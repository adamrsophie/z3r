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
