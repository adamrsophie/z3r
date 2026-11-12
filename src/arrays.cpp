// Arrays (multi-index, map, default, extensionality), sets, lambdas,
// pseudo-Boolean constraints, recursive functions, special relations and
// remaining arithmetic and bit-vector constructors.
#include "z3r.h"

using namespace z3r;

namespace {

std::vector<Z3_sort> sort_list(const CtxPtr& ctx, Rcpp::List sorts, AstVec& keep) {
  std::vector<Z3_sort> out;
  for (R_xlen_t i = 0; i < sorts.size(); ++i) {
    const Ast& s = get_sort(sorts[i]);
    same_ctx(ctx, s.ctx());
    keep.push_back(s);
    out.push_back(as_sort(s));
  }
  return out;
}

Ast typed(const CtxPtr& ctx, SEXP x, Z3_sort want, const std::string& what) {
  Ast a = lift(ctx, x, want);
  Z3_context c = ctx->c;
  if (Z3_get_sort_kind(c, want) == Z3_REAL_SORT && sort_kind(a) == Z3_INT_SORT) a = to_real(a);
  if (!Z3_is_eq_sort(c, Z3_get_sort(c, a), want)) {
    fail(what + " has sort " + sort_name(ctx, Z3_get_sort(c, a)) + " but " +
         sort_name(ctx, want) + " is required");
  }
  return a;
}

unsigned array_arity(Z3_context c, Z3_sort s) {
#if Z3R_AT_LEAST(4, 15, 0)
  return Z3_get_array_arity(c, s);
#elif Z3R_AT_LEAST(4, 8, 15)
  // Count index sorts until the lookup reports an error.
  unsigned n = 0;
  for (;;) {
    Z3_get_array_sort_domain_n(c, s, n);
    if (Z3_get_error_code(c) != Z3_OK) {
      Z3_set_error(c, Z3_OK);
      break;
    }
    ++n;
  }
  return n;
#else
  (void)s;
  (void)c;
  return 1;
#endif
}

Z3_sort array_domain(Z3_context c, Z3_sort s, unsigned i) {
#if Z3R_AT_LEAST(4, 8, 15)
  return Z3_get_array_sort_domain_n(c, s, i);
#else
  (void)i;
  return Z3_get_array_sort_domain(c, s);
#endif
}

}  // namespace

// [[Rcpp::export(rng = false)]]
SEXP cpp_array_sort_n(Rcpp::List domain, SEXP range_) {
  const Ast& r = get_sort(range_);
  const CtxPtr& ctx = r.ctx();
  AstVec keep;
  std::vector<Z3_sort> d = sort_list(ctx, domain, keep);
  if (d.empty()) fail("an array needs at least one index sort");
  return wrap_sort(own_ast(ctx, sort_ast(Z3_mk_array_sort_n(ctx->c, static_cast<unsigned>(d.size()), d.data(),
                                                            as_sort(r)))));
}

// Index sorts and range of an array sort (any arity).
// [[Rcpp::export(rng = false)]]
SEXP cpp_array_sort_info(SEXP x) {
  Ast s = is_kind(x, Kind::Sort) ? get_sort(x) : sort_of(get_expr(x));
  const CtxPtr& ctx = s.ctx();
  Z3_context c = ctx->c;
  if (Z3_get_sort_kind(c, as_sort(s)) != Z3_ARRAY_SORT) fail("not an array");
  unsigned n = array_arity(c, as_sort(s));
  Rcpp::List dom(n);
  for (unsigned i = 0; i < n; ++i) dom[i] = wrap_sort(own_ast(ctx, sort_ast(array_domain(c, as_sort(s), i))));
  Ast range = own_ast(ctx, sort_ast(Z3_get_array_sort_range(c, as_sort(s))));
  return Rcpp::List::create(Rcpp::Named("domain") = dom, Rcpp::Named("range") = wrap_sort(range));
}

// select/store with any number of indices.
// [[Rcpp::export(rng = false)]]
SEXP cpp_array_access(SEXP a_, Rcpp::List idx, SEXP value, bool store) {
  const Ast& a = get_expr(a_);
  const CtxPtr& ctx = a.ctx();
  Z3_context c = ctx->c;
  Z3_sort s = Z3_get_sort(c, a);
  if (Z3_get_sort_kind(c, s) != Z3_ARRAY_SORT) {
    fail(std::string(store ? "z3_store()" : "z3_select()") + " needs an array, got " + sort_name(ctx, s));
  }
  unsigned n = array_arity(c, s);
  if (static_cast<unsigned>(idx.size()) != n) {
    fail("the array has " + std::to_string(n) + " index(es), got " +
         std::to_string(static_cast<long long>(idx.size())));
  }
  AstVec iv;
  for (unsigned i = 0; i < n; ++i) iv.push_back(typed(ctx, idx[i], array_domain(c, s, i), "index"));
  std::vector<Z3_ast> r = raw(iv);
  if (!store) return wrap_expr(own_ast(ctx, Z3_mk_select_n(c, a, n, r.data())));
  Ast v = typed(ctx, value, Z3_get_array_sort_range(c, s), "the stored value");
  return wrap_expr(own_ast(ctx, Z3_mk_store_n(c, a, n, r.data(), v)));
}

// Applies f pointwise to arrays: result[i] = f(a1[i], ..., an[i]).
// [[Rcpp::export(rng = false)]]
SEXP cpp_array_map(SEXP decl_, Rcpp::List arrays) {
  const Ast& d = get_decl(decl_);
  const CtxPtr& ctx = d.ctx();
  AstVec v = expr_list(ctx, arrays, "arrays");
  if (v.empty()) fail("z3_array_map() needs at least one array");
  std::vector<Z3_ast> r = raw(v);
  return wrap_expr(own_ast(ctx, Z3_mk_map(ctx->c, as_decl(d), static_cast<unsigned>(r.size()), r.data())));
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_array_unary(std::string op, SEXP a_, SEXP b_) {
  const Ast& a = get_expr(a_);
  const CtxPtr& ctx = a.ctx();
  if (sort_kind(a) != Z3_ARRAY_SORT) fail("expected an array");
  if (op == "default") return wrap_expr(own_ast(ctx, Z3_mk_array_default(ctx->c, a)));
  const Ast& b = get_expr(b_);
  same_ctx(ctx, b.ctx());
  return wrap_expr(own_ast(ctx, Z3_mk_array_ext(ctx->c, a, b)));
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_as_array(SEXP decl_) {
  const Ast& d = get_decl(decl_);
  return wrap_expr(own_ast(d.ctx(), Z3_mk_as_array(d.c(), as_decl(d))));
}

// Lambda over constants: an array-valued function of the bound variables.
// [[Rcpp::export(rng = false)]]
SEXP cpp_lambda(Rcpp::List vars, SEXP body_) {
  const Ast& body = get_expr(body_);
  const CtxPtr& ctx = body.ctx();
  Z3_context c = ctx->c;
  if (vars.size() == 0) fail("a lambda needs at least one bound variable");
  AstVec keep = expr_list(ctx, vars, "bound variables");
  std::vector<Z3_app> bound;
  for (const Ast& v : keep) {
    if (!Z3_is_app(c, v) || Z3_get_app_num_args(c, Z3_to_app(c, v)) != 0 ||
        Z3_get_decl_kind(c, Z3_get_app_decl(c, Z3_to_app(c, v))) != Z3_OP_UNINTERPRETED) {
      fail("bound variables must be constants such as those made by z3_int()");
    }
    bound.push_back(Z3_to_app(c, v));
  }
  return wrap_expr(own_ast(ctx, Z3_mk_lambda_const(c, static_cast<unsigned>(bound.size()), bound.data(), body)));
}

// ---------------------------------------------------------------------------
// Sets (arrays to Bool)

// [[Rcpp::export(rng = false)]]
SEXP cpp_set_sort(SEXP elem_) {
  const Ast& e = get_sort(elem_);
  return wrap_sort(own_ast(e.ctx(), sort_ast(Z3_mk_set_sort(e.c(), as_sort(e)))));
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_set_constant(bool full, SEXP elem_) {
  const Ast& e = get_sort(elem_);
  return wrap_expr(own_ast(e.ctx(), full ? Z3_mk_full_set(e.c(), as_sort(e)) : Z3_mk_empty_set(e.c(), as_sort(e))));
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_set_op(std::string op, SEXP args) {
  CtxPtr ctx = ctx_from_list(args);
  Z3_context c = ctx->c;
  R_xlen_t n = Rf_xlength(args);
  auto set_arg = [&](R_xlen_t i) {
    Ast a = lift(ctx, VECTOR_ELT(args, i), nullptr);
    Z3_sort s = Z3_get_sort(c, a);
    if (Z3_get_sort_kind(c, s) != Z3_ARRAY_SORT ||
        Z3_get_sort_kind(c, Z3_get_array_sort_range(c, s)) != Z3_BOOL_SORT) {
      fail("'" + op + "' needs sets (arrays to Bool), got " + sort_name(ctx, s));
    }
    return a;
  };
  if (op == "union" || op == "intersect") {
    if (n < 1) fail("'" + op + "' needs at least one set");
    AstVec v;
    for (R_xlen_t i = 0; i < n; ++i) v.push_back(set_arg(i));
    std::vector<Z3_ast> r = raw(v);
    return wrap_expr(own_ast(ctx, (op == "union" ? Z3_mk_set_union : Z3_mk_set_intersect)(
                                      c, static_cast<unsigned>(r.size()), r.data())));
  }
  if (op == "complement") return wrap_expr(own_ast(ctx, Z3_mk_set_complement(c, set_arg(0))));
  if (op == "difference") return wrap_expr(own_ast(ctx, Z3_mk_set_difference(c, set_arg(0), set_arg(1))));
  if (op == "subset") return wrap_expr(own_ast(ctx, Z3_mk_set_subset(c, set_arg(0), set_arg(1))));
  // add, del, member: (set, element) for add/del, (element, set) for member
  bool member = op == "member";
  Ast s = set_arg(member ? 1 : 0);
  Ast e = typed(ctx, VECTOR_ELT(args, member ? 0 : 1), Z3_get_array_sort_domain(c, Z3_get_sort(c, s)), "the element");
  if (member) return wrap_expr(own_ast(ctx, Z3_mk_set_member(c, e, s)));
  return wrap_expr(own_ast(ctx, op == "add" ? Z3_mk_set_add(c, s, e) : Z3_mk_set_del(c, s, e)));
}

// ---------------------------------------------------------------------------
// Pseudo-Boolean and cardinality constraints

// [[Rcpp::export(rng = false)]]
SEXP cpp_pb(std::string op, Rcpp::List lits, Rcpp::IntegerVector coeffs, int k) {
  CtxPtr ctx = ctx_from_list(lits);
  Z3_context c = ctx->c;
  AstVec v;
  for (R_xlen_t i = 0; i < lits.size(); ++i) {
    Ast a = lift(ctx, lits[i], Z3_mk_bool_sort(c));
    if (!is_bool(a)) fail("pseudo-Boolean constraints need Boolean literals");
    v.push_back(a);
  }
  std::vector<Z3_ast> r = raw(v);
  unsigned n = static_cast<unsigned>(r.size());
  if (op == "atmost" || op == "atleast") {
    if (k < 0) fail("k must be non-negative");
    return wrap_expr(own_ast(ctx, (op == "atmost" ? Z3_mk_atmost : Z3_mk_atleast)(c, n, r.data(),
                                                                                  static_cast<unsigned>(k))));
  }
  if (static_cast<unsigned>(coeffs.size()) != n) fail("one coefficient per literal is needed");
  std::vector<int> cf(coeffs.begin(), coeffs.end());
  typedef Z3_ast (*PbFn)(Z3_context, unsigned, Z3_ast const[], int const[], int);
  PbFn f = op == "pble" ? Z3_mk_pble : op == "pbge" ? Z3_mk_pbge : Z3_mk_pbeq;
  return wrap_expr(own_ast(ctx, f(c, n, r.data(), cf.data(), k)));
}

// ---------------------------------------------------------------------------
// Recursive functions

// [[Rcpp::export(rng = false)]]
SEXP cpp_mk_rec_func_decl(SEXP ctx_, SEXP name_, Rcpp::List domain, SEXP range_) {
  CtxPtr ctx = get_ctx(ctx_);
  std::string name = as_string(name_, "name");
  AstVec keep;
  std::vector<Z3_sort> d = sort_list(ctx, domain, keep);
  const Ast& r = get_sort(range_);
  same_ctx(ctx, r.ctx());
  Z3_func_decl f = Z3_mk_rec_func_decl(ctx->c, mk_symbol(ctx, name), static_cast<unsigned>(d.size()),
                                       d.empty() ? nullptr : d.data(), as_sort(r));
  check(ctx);
  return wrap_decl(own_ast(ctx, decl_ast(f)));
}

// Defines f(args) = body; args are constants standing for the parameters.
// [[Rcpp::export(rng = false)]]
void cpp_add_rec_def(SEXP decl_, Rcpp::List args, SEXP body_) {
  const Ast& d = get_decl(decl_);
  const CtxPtr& ctx = d.ctx();
  Z3_context c = ctx->c;
  Z3_func_decl f = as_decl(d);
  AstVec a = expr_list(ctx, args, "parameters");
  if (a.size() != Z3_get_arity(c, f)) fail("the definition needs one parameter per argument");
  for (unsigned i = 0; i < a.size(); ++i) {
    if (!Z3_is_eq_sort(c, Z3_get_sort(c, a[i]), Z3_get_domain(c, f, i))) fail("a parameter has the wrong sort");
  }
  Ast body = typed(ctx, body_, Z3_get_range(c, f), "the body");
  std::vector<Z3_ast> r = raw(a);
  Z3_add_rec_def(c, f, static_cast<unsigned>(r.size()), r.empty() ? nullptr : r.data(), body);
  check(ctx);
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_mk_fresh_func_decl(SEXP ctx_, SEXP prefix_, Rcpp::List domain, SEXP range_) {
  CtxPtr ctx = get_ctx(ctx_);
  std::string prefix = as_string(prefix_, "prefix");
  AstVec keep;
  std::vector<Z3_sort> d = sort_list(ctx, domain, keep);
  const Ast& r = get_sort(range_);
  same_ctx(ctx, r.ctx());
  Z3_func_decl f = Z3_mk_fresh_func_decl(ctx->c, prefix.c_str(), static_cast<unsigned>(d.size()),
                                         d.empty() ? nullptr : d.data(), as_sort(r));
  check(ctx);
  return wrap_decl(own_ast(ctx, decl_ast(f)));
}

// ---------------------------------------------------------------------------
// Special relations

// [[Rcpp::export(rng = false)]]
SEXP cpp_special_relation(std::string kind, SEXP sort_, int id) {
  const Ast& s = get_sort(sort_);
  if (id < 0) fail("id must be a non-negative integer");
  Z3_context c = s.c();
  unsigned u = static_cast<unsigned>(id);
  Z3_func_decl f = kind == "linear" ? Z3_mk_linear_order(c, as_sort(s), u)
                   : kind == "partial" ? Z3_mk_partial_order(c, as_sort(s), u)
                   : kind == "piecewise" ? Z3_mk_piecewise_linear_order(c, as_sort(s), u)
                                         : Z3_mk_tree_order(c, as_sort(s), u);
  check(s.ctx());
  return wrap_decl(own_ast(s.ctx(), decl_ast(f)));
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_transitive_closure(SEXP decl_) {
  const Ast& d = get_decl(decl_);
  Z3_func_decl f = Z3_mk_transitive_closure(d.c(), as_decl(d));
  check(d.ctx());
  return wrap_decl(own_ast(d.ctx(), decl_ast(f)));
}

// ---------------------------------------------------------------------------
// Miscellaneous constructors

// [[Rcpp::export(rng = false)]]
SEXP cpp_misc_op(std::string op, SEXP args, Rcpp::IntegerVector params) {
  CtxPtr ctx = ctx_from_list(args);
  Z3_context c = ctx->c;
  if (op == "divides") {
    Ast a = lift(ctx, VECTOR_ELT(args, 0), Z3_mk_int_sort(c));
    Ast b = lift(ctx, VECTOR_ELT(args, 1), Z3_mk_int_sort(c));
    if (sort_kind(a) != Z3_INT_SORT || sort_kind(b) != Z3_INT_SORT) fail("z3_divides() needs Int arguments");
    return wrap_expr(own_ast(ctx, Z3_mk_divides(c, a, b)));
  }
  if (op == "bvneg_no_overflow") {
    Ast a = lift(ctx, VECTOR_ELT(args, 0), nullptr);
    if (sort_kind(a) != Z3_BV_SORT) fail("expected a bit-vector");
    return wrap_expr(own_ast(ctx, Z3_mk_bvneg_no_overflow(c, a)));
  }
  if (op == "bit2bool") {
#if Z3R_AT_LEAST(4, 8, 16)
    Ast a = lift(ctx, VECTOR_ELT(args, 0), nullptr);
    if (sort_kind(a) != Z3_BV_SORT) fail("expected a bit-vector");
    unsigned w = Z3_get_bv_sort_size(c, Z3_get_sort(c, a));
    if (params.size() != 1 || params[0] < 0 || static_cast<unsigned>(params[0]) >= w) {
      fail("the bit index must be between 0 and width - 1");
    }
    return wrap_expr(own_ast(ctx, Z3_mk_bit2bool(c, static_cast<unsigned>(params[0]), a)));
#else
    needs_version("z3_bit2bool()", "4.8.16");
#endif
  }
  fail("unknown operation '" + op + "'");
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_finite_domain_sort(SEXP ctx_, SEXP name_, double size) {
  CtxPtr ctx = get_ctx(ctx_);
  std::string name = as_string(name_, "name");
  if (ISNAN(size) || size < 1 || size != std::floor(size) || size > 9007199254740992.0) {
    fail("size must be a positive whole number");
  }
  Z3_sort s = Z3_mk_finite_domain_sort(ctx->c, mk_symbol(ctx, name), static_cast<uint64_t>(size));
  return wrap_sort(own_ast(ctx, sort_ast(s)));
}

// [[Rcpp::export(rng = false)]]
double cpp_finite_domain_size(SEXP sort_) {
  const Ast& s = get_sort(sort_);
  uint64_t n = 0;
  if (!Z3_get_finite_domain_sort_size(s.c(), as_sort(s), &n)) fail("not a finite domain sort");
  check(s.ctx());
  return static_cast<double>(n);
}
