// Algebraic datatypes.
#include "z3r.h"

using namespace z3r;

namespace {

// Z3_constructor and Z3_constructor_list are not reference counted; they
// are deleted explicitly once the datatype sorts exist. This guard deletes
// them on every exit path, including errors.
struct ConstructorGuard {
  Z3_context c;
  std::vector<Z3_constructor> constructors;
  std::vector<Z3_constructor_list> lists;
  explicit ConstructorGuard(Z3_context ctx) : c(ctx) {}
  ~ConstructorGuard() {
    for (Z3_constructor_list l : lists) Z3_del_constructor_list(c, l);
    for (Z3_constructor k : constructors) Z3_del_constructor(c, k);
  }
};

// Builds the Z3 constructors of one datatype from its R specification: a
// list of constructors, each list(name, recognizer, fields, sorts, refs).
// A field's sort is either a z3_sort or, for a reference to a datatype of
// the same declaration, NULL with refs[i] its index.
std::vector<Z3_constructor> make_constructors(const CtxPtr& ctx, Rcpp::List spec,
                                              ConstructorGuard& guard, AstVec& keep) {
  std::vector<Z3_constructor> out;
  for (R_xlen_t k = 0; k < spec.size(); ++k) {
    Rcpp::List con(spec[k]);
    std::string name = Rcpp::as<std::string>(con["name"]);
    std::string recognizer = Rcpp::as<std::string>(con["recognizer"]);
    Rcpp::CharacterVector fields = con["fields"];
    Rcpp::List sorts = con["sorts"];
    Rcpp::IntegerVector refs = con["refs"];
    unsigned nf = static_cast<unsigned>(fields.size());
    std::vector<Z3_symbol> field_syms(nf);
    std::vector<Z3_sort> field_sorts(nf, nullptr);
    std::vector<unsigned> sort_refs(nf, 0);
    for (unsigned i = 0; i < nf; ++i) {
      field_syms[i] = mk_symbol(ctx, Rcpp::as<std::string>(fields[i]));
      SEXP srt = sorts[i];
      if (Rf_isNull(srt)) {
        sort_refs[i] = static_cast<unsigned>(refs[i]);
      } else {
        const Ast& s = get_sort(srt);
        same_ctx(ctx, s.ctx());
        keep.push_back(s);
        field_sorts[i] = as_sort(s);
      }
    }
    Z3_constructor cons = Z3_mk_constructor(ctx->c, mk_symbol(ctx, name), mk_symbol(ctx, recognizer), nf,
                                            nf ? field_syms.data() : nullptr,
                                            nf ? field_sorts.data() : nullptr,
                                            nf ? sort_refs.data() : nullptr);
    check(ctx);
    guard.constructors.push_back(cons);
    out.push_back(cons);
  }
  return out;
}

// Name of a new sort: a single non-empty string.
Z3_symbol sort_symbol(const CtxPtr& ctx, SEXP name_) {
  std::string name = as_string(name_, "name");
  if (name.empty()) fail("name must not be empty");
  return mk_symbol(ctx, name);
}

// Element i of a character vector as UTF-8; fails on NA.
std::string utf8_elt(SEXP x, R_xlen_t i, const char* what) {
  SEXP s = STRING_ELT(x, i);
  if (s == NA_STRING) fail(std::string(what) + " must not be missing");
  return std::string(Rf_translateCharUTF8(s));
}

// Declarations of one constructor of a new datatype.
struct Parts {
  Ast cons, rec;
  AstVec acc;
};

// The sort and, per constructor, its declaration, recognizer and accessors,
// in the shape of cpp_datatype_parts(). Every handle is owned before this
// allocates R objects.
SEXP wrap_datatype(const Ast& sort, const std::vector<Parts>& parts) {
  Rcpp::List out(parts.size());
  for (size_t k = 0; k < parts.size(); ++k) {
    Rcpp::List acc(parts[k].acc.size());
    for (size_t a = 0; a < parts[k].acc.size(); ++a) acc[a] = wrap_decl(parts[k].acc[a]);
    Rcpp::List con(3);
    con[0] = wrap_decl(parts[k].cons);
    con[1] = wrap_decl(parts[k].rec);
    con[2] = acc;
    con.names() = Rcpp::CharacterVector::create("constructor", "recognizer", "accessors");
    out[k] = con;
  }
  Rcpp::List res(2);
  res[0] = wrap_sort(sort);
  res[1] = out;
  res.names() = Rcpp::CharacterVector::create("sort", "parts");
  return res;
}

}  // namespace

// Constructors, recognizers and accessors of a datatype sort.
// [[Rcpp::export(rng = false)]]
SEXP cpp_datatype_parts(SEXP sort_) {
  const Ast& s = get_sort(sort_);
  const CtxPtr& ctx = s.ctx();
  Z3_context c = ctx->c;
  Z3_sort srt = as_sort(s);
  if (Z3_get_sort_kind(c, srt) != Z3_DATATYPE_SORT) fail("not a datatype sort");
  unsigned n = Z3_get_datatype_sort_num_constructors(c, srt);
  check(ctx);
  Rcpp::List cons(n);
  for (unsigned k = 0; k < n; ++k) {
    Ast decl = own_ast(ctx, decl_ast(Z3_get_datatype_sort_constructor(c, srt, k)));
    Ast rec = own_ast(ctx, decl_ast(Z3_get_datatype_sort_recognizer(c, srt, k)));
    unsigned na = Z3_get_arity(c, as_decl(decl));
    Rcpp::List acc(na);
    for (unsigned a = 0; a < na; ++a) {
      acc[a] = wrap_decl(own_ast(ctx, decl_ast(Z3_get_datatype_sort_constructor_accessor(c, srt, k, a))));
    }
    cons[k] = Rcpp::List::create(Rcpp::Named("constructor") = wrap_decl(decl),
                                 Rcpp::Named("recognizer") = wrap_decl(rec),
                                 Rcpp::Named("accessors") = acc);
  }
  return cons;
}

// Declares one or more (mutually recursive) datatypes. Returns their sorts.
// [[Rcpp::export(rng = false)]]
SEXP cpp_mk_datatypes(SEXP ctx_, Rcpp::CharacterVector names, Rcpp::List specs) {
  CtxPtr ctx = get_ctx(ctx_);
  Z3_context c = ctx->c;
  unsigned n = static_cast<unsigned>(names.size());
  if (n == 0 || specs.size() != names.size()) fail("internal error: bad datatype specification");
  ConstructorGuard guard(c);
  AstVec keep;
  std::vector<Z3_symbol> sort_names(n);
  for (unsigned i = 0; i < n; ++i) {
    sort_names[i] = mk_symbol(ctx, Rcpp::as<std::string>(names[i]));
    std::vector<Z3_constructor> cons = make_constructors(ctx, specs[i], guard, keep);
    if (cons.empty()) fail("a datatype needs at least one constructor");
    Z3_constructor_list l = Z3_mk_constructor_list(c, static_cast<unsigned>(cons.size()), cons.data());
    check(ctx);
    guard.lists.push_back(l);
  }
  std::vector<Z3_sort> sorts(n, nullptr);
  Z3_mk_datatypes(c, n, sort_names.data(), sorts.data(), guard.lists.data());
  check(ctx);
  Rcpp::List out(n);
  for (unsigned i = 0; i < n; ++i) out[i] = wrap_sort(own_ast(ctx, sort_ast(sorts[i])));
  return out;
}

// Declares a single datatype, whose fields may refer to it (reference index
// 0). The declarations are read back from the constructor objects.
// [[Rcpp::export(rng = false)]]
SEXP cpp_mk_datatype(SEXP ctx_, SEXP name_, Rcpp::List spec) {
  CtxPtr ctx = get_ctx(ctx_);
  Z3_context c = ctx->c;
  Z3_symbol name = sort_symbol(ctx, name_);
  ConstructorGuard guard(c);
  AstVec keep;
  std::vector<Z3_constructor> cons = make_constructors(ctx, spec, guard, keep);
  if (cons.empty()) fail("a datatype needs at least one constructor");
  Ast sort = own_ast(ctx, sort_ast(Z3_mk_datatype(c, name, static_cast<unsigned>(cons.size()), cons.data())));
  // Each query replaces Z3's record of recent results, which alone holds
  // the declarations it returns: own them before the next call.
  std::vector<Parts> parts(cons.size());
  for (size_t k = 0; k < cons.size(); ++k) {
#if Z3R_AT_LEAST(4, 12, 3)
    unsigned nf = Z3_constructor_num_fields(c, cons[k]);
    check(ctx);
#else
    Rcpp::List con(spec[static_cast<R_xlen_t>(k)]);
    Rcpp::CharacterVector fields = con["fields"];
    unsigned nf = static_cast<unsigned>(fields.size());
#endif
    Z3_func_decl decl = nullptr, tester = nullptr;
    std::vector<Z3_func_decl> acc(nf, nullptr);
    Z3_query_constructor(c, cons[k], nf, &decl, &tester, nf ? acc.data() : nullptr);
    parts[k].cons = own_ast(ctx, decl_ast(decl));
    parts[k].rec = own_ast(ctx, decl_ast(tester));
    for (Z3_func_decl a : acc) parts[k].acc.push_back(own_ast(ctx, decl_ast(a)));
  }
  return wrap_datatype(sort, parts);
}

// Tuple sort: one constructor named like the sort, with recognizer
// is_<name> and the given fields (no recursive references).
// [[Rcpp::export(rng = false)]]
SEXP cpp_mk_tuple_sort(SEXP ctx_, SEXP name_, Rcpp::CharacterVector fields, Rcpp::List sorts) {
  CtxPtr ctx = get_ctx(ctx_);
  Z3_context c = ctx->c;
  Z3_symbol name = sort_symbol(ctx, name_);
  unsigned n = static_cast<unsigned>(fields.size());
  if (static_cast<unsigned>(sorts.size()) != n) fail("internal error: bad tuple specification");
  std::vector<Z3_symbol> syms(n);
  std::vector<Z3_sort> ss(n);
  AstVec keep;
  for (unsigned i = 0; i < n; ++i) {
    syms[i] = mk_symbol(ctx, utf8_elt(fields, i, "field names"));
    const Ast& s = get_sort(sorts[i]);
    same_ctx(ctx, s.ctx());
    keep.push_back(s);
    ss[i] = as_sort(s);
  }
  Z3_func_decl mk = nullptr;
  std::vector<Z3_func_decl> proj(n, nullptr);
  Z3_sort raw_sort = Z3_mk_tuple_sort(c, name, n, n ? syms.data() : nullptr, n ? ss.data() : nullptr, &mk,
                                      n ? proj.data() : nullptr);
  Ast sort = own_ast(ctx, sort_ast(raw_sort));
  std::vector<Parts> parts(1);
  parts[0].cons = own_ast(ctx, decl_ast(mk));
  for (Z3_func_decl a : proj) parts[0].acc.push_back(own_ast(ctx, decl_ast(a)));
  parts[0].rec = own_ast(ctx, decl_ast(Z3_get_datatype_sort_recognizer(c, as_sort(sort), 0)));
  return wrap_datatype(sort, parts);
}

// Enumeration sort: one constant per value, with recognizers is_<value>.
// [[Rcpp::export(rng = false)]]
SEXP cpp_mk_enumeration_sort(SEXP ctx_, SEXP name_, Rcpp::CharacterVector values) {
  CtxPtr ctx = get_ctx(ctx_);
  Z3_context c = ctx->c;
  Z3_symbol name = sort_symbol(ctx, name_);
  unsigned n = static_cast<unsigned>(values.size());
  if (n == 0) fail("an enumeration needs at least one value");
  std::vector<Z3_symbol> syms(n);
  for (unsigned i = 0; i < n; ++i) {
    std::string v = utf8_elt(values, i, "values");
    if (v.empty()) fail("values must not be empty");
    syms[i] = mk_symbol(ctx, v);
  }
  std::vector<Z3_func_decl> consts(n, nullptr), testers(n, nullptr);
  Ast sort = own_ast(ctx, sort_ast(Z3_mk_enumeration_sort(c, name, n, syms.data(), consts.data(), testers.data())));
  std::vector<Parts> parts(n);
  for (unsigned i = 0; i < n; ++i) {
    parts[i].cons = own_ast(ctx, decl_ast(consts[i]));
    parts[i].rec = own_ast(ctx, decl_ast(testers[i]));
  }
  return wrap_datatype(sort, parts);
}

// List sort with constructors nil and cons(head, tail).
// [[Rcpp::export(rng = false)]]
SEXP cpp_mk_list_sort(SEXP ctx_, SEXP name_, SEXP element_) {
  CtxPtr ctx = get_ctx(ctx_);
  Z3_context c = ctx->c;
  Z3_symbol name = sort_symbol(ctx, name_);
  const Ast& elem = get_sort(element_);
  same_ctx(ctx, elem.ctx());
  Z3_func_decl nil = nullptr, is_nil = nullptr, cons = nullptr, is_cons = nullptr, head = nullptr, tail = nullptr;
  Ast sort = own_ast(ctx, sort_ast(Z3_mk_list_sort(c, name, as_sort(elem), &nil, &is_nil, &cons, &is_cons,
                                                   &head, &tail)));
  std::vector<Parts> parts(2);
  parts[0].cons = own_ast(ctx, decl_ast(nil));
  parts[0].rec = own_ast(ctx, decl_ast(is_nil));
  parts[1].cons = own_ast(ctx, decl_ast(cons));
  parts[1].rec = own_ast(ctx, decl_ast(is_cons));
  parts[1].acc.push_back(own_ast(ctx, decl_ast(head)));
  parts[1].acc.push_back(own_ast(ctx, decl_ast(tail)));
  return wrap_datatype(sort, parts);
}

// Constructor and field accessors of a tuple sort (a datatype with one
// constructor and no recursive fields).
// [[Rcpp::export(rng = false)]]
SEXP cpp_tuple_parts(SEXP sort_) {
  const Ast& s = get_sort(sort_);
  const CtxPtr& ctx = s.ctx();
  Z3_context c = ctx->c;
  Z3_sort srt = as_sort(s);
  const char* not_tuple = "not a tuple sort: a tuple is a datatype with one constructor and no recursive fields";
  if (Z3_get_sort_kind(c, srt) != Z3_DATATYPE_SORT) fail(not_tuple);
  unsigned ncons = Z3_get_datatype_sort_num_constructors(c, srt);
  check(ctx);
  if (ncons != 1) fail(not_tuple);
  Z3_func_decl raw_mk = Z3_get_tuple_sort_mk_decl(c, srt);
  if (Z3_get_error_code(c) != Z3_OK || !raw_mk) {
    Z3_set_error(c, Z3_OK);
    fail(not_tuple);
  }
  Ast mk = own_ast(ctx, decl_ast(raw_mk));
  unsigned n = Z3_get_tuple_sort_num_fields(c, srt);
  check(ctx);
  AstVec fields;
  for (unsigned i = 0; i < n; ++i) fields.push_back(own_ast(ctx, decl_ast(Z3_get_tuple_sort_field_decl(c, srt, i))));
  Rcpp::List acc(n);
  for (unsigned i = 0; i < n; ++i) acc[i] = wrap_decl(fields[i]);
  Rcpp::List out(2);
  out[0] = wrap_decl(mk);
  out[1] = acc;
  out.names() = Rcpp::CharacterVector::create("constructor", "accessors");
  return out;
}

// Type variable for polymorphic datatypes (Z3 4.12.3 and later).
// [[Rcpp::export(rng = false)]]
SEXP cpp_mk_type_variable(SEXP ctx_, SEXP name_) {
#if Z3R_AT_LEAST(4, 12, 3)
  CtxPtr ctx = get_ctx(ctx_);
  std::string name = as_string(name_, "name");
  return wrap_sort(own_ast(ctx, sort_ast(Z3_mk_type_variable(ctx->c, mk_symbol(ctx, name)))));
#else
  (void)ctx_; (void)name_;
  needs_version("z3_type_variable()", "4.12.3");
#endif
}

// Polymorphic datatype over type variables (Z3 4.15.4 and later).
// [[Rcpp::export(rng = false)]]
SEXP cpp_mk_polymorphic_datatype(SEXP ctx_, SEXP name_, Rcpp::List params, Rcpp::List spec) {
#if Z3R_AT_LEAST(4, 15, 4)
  CtxPtr ctx = get_ctx(ctx_);
  std::string name = as_string(name_, "name");
  ConstructorGuard guard(ctx->c);
  AstVec keep;
  std::vector<Z3_sort> ps;
  for (R_xlen_t i = 0; i < params.size(); ++i) {
    const Ast& s = get_sort(params[i]);
    same_ctx(ctx, s.ctx());
    keep.push_back(s);
    ps.push_back(as_sort(s));
  }
  std::vector<Z3_constructor> cons = make_constructors(ctx, spec, guard, keep);
  if (cons.empty()) fail("a datatype needs at least one constructor");
  Z3_sort srt = Z3_mk_polymorphic_datatype(ctx->c, mk_symbol(ctx, name), static_cast<unsigned>(ps.size()),
                                           ps.empty() ? nullptr : ps.data(),
                                           static_cast<unsigned>(cons.size()), cons.data());
  return wrap_sort(own_ast(ctx, sort_ast(srt)));
#else
  (void)ctx_; (void)name_; (void)params; (void)spec;
  needs_version("polymorphic datatypes", "4.15.4");
#endif
}

// Instance of a declared (polymorphic) datatype at concrete sorts.
// [[Rcpp::export(rng = false)]]
SEXP cpp_datatype_instance(SEXP ctx_, SEXP name_, Rcpp::List params) {
#if Z3R_AT_LEAST(4, 15, 4)
  CtxPtr ctx = get_ctx(ctx_);
  std::string name = as_string(name_, "name");
  AstVec keep;
  std::vector<Z3_sort> ps;
  for (R_xlen_t i = 0; i < params.size(); ++i) {
    const Ast& s = get_sort(params[i]);
    same_ctx(ctx, s.ctx());
    keep.push_back(s);
    ps.push_back(as_sort(s));
  }
  Z3_sort srt = Z3_mk_datatype_sort(ctx->c, mk_symbol(ctx, name), static_cast<unsigned>(ps.size()),
                                    ps.empty() ? nullptr : ps.data());
  return wrap_sort(own_ast(ctx, sort_ast(srt)));
#else
  (void)ctx_; (void)name_; (void)params;
  needs_version("z3_datatype_instance()", "4.15.4");
#endif
}

// Copy of t with the field read by `accessor` replaced by value.
// [[Rcpp::export(rng = false)]]
SEXP cpp_datatype_update(SEXP accessor_, SEXP t_, SEXP value_) {
  const Ast& acc = get_decl(accessor_);
  const CtxPtr& ctx = acc.ctx();
  Z3_context c = ctx->c;
  Ast t = lift(ctx, t_, Z3_get_domain(c, as_decl(acc), 0));
  Ast v = lift(ctx, value_, Z3_get_range(c, as_decl(acc)));
  if (!Z3_is_eq_sort(c, Z3_get_sort(c, t), Z3_get_domain(c, as_decl(acc), 0))) {
    fail("the value to update is not of the datatype this field belongs to");
  }
  if (!Z3_is_eq_sort(c, Z3_get_sort(c, v), Z3_get_range(c, as_decl(acc)))) {
    fail("the new field value has the wrong sort");
  }
  return wrap_expr(own_ast(ctx, Z3_datatype_update_field(c, as_decl(acc), t, v)));
}

// [[Rcpp::export(rng = false)]]
bool cpp_is_recursive_datatype(SEXP sort_) {
#if Z3R_AT_LEAST(4, 15, 0)
  const Ast& s = get_sort(sort_);
  bool r = Z3_is_recursive_datatype_sort(s.c(), as_sort(s));
  check(s.ctx());
  return r;
#else
  (void)sort_;
  needs_version("z3_is_recursive_datatype()", "4.15.0");
#endif
}
