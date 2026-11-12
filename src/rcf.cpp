// Real closed field numbers, algebraic number arithmetic and polynomials.
#include "z3r.h"

using namespace z3r;

namespace {

// Z3_rcf_num values are not reference counted; each is owned by exactly one
// holder and deleted with Z3_rcf_del.
struct RcfHolder {
  CtxPtr ctx;
  Z3_rcf_num n;
  RcfHolder(CtxPtr c, Z3_rcf_num v) : ctx(std::move(c)), n(v) {}
  ~RcfHolder() {
    if (n) Z3_rcf_del(ctx->c, n);
  }
  RcfHolder(const RcfHolder&) = delete;
  RcfHolder& operator=(const RcfHolder&) = delete;
};

RcfHolder* rcf(SEXP x) { return get_xptr<RcfHolder>(x, Kind::Rcf); }

// Z3 represents the number zero by a null Z3_rcf_num, so null is valid.
SEXP wrap_rcf(const CtxPtr& ctx, Z3_rcf_num v) {
  // Take ownership first so the number is freed if the error check throws.
  std::unique_ptr<RcfHolder> h(new RcfHolder(ctx, v));
  check(ctx);
  return make_xptr(h.release(), Kind::Rcf);
}

}  // namespace

namespace z3r {
CtxPtr rcf_ctx(SEXP x) { return rcf(x)->ctx; }
}  // namespace z3r

// [[Rcpp::export(rng = false)]]
SEXP cpp_rcf_mk(SEXP ctx_, std::string what, SEXP value_) {
  CtxPtr ctx = get_ctx(ctx_);
  Z3_context c = ctx->c;
  if (what == "rational") {
    std::string v = as_string(value_, "value");
    return wrap_rcf(ctx, Z3_rcf_mk_rational(c, v.c_str()));
  }
  if (what == "pi") return wrap_rcf(ctx, Z3_rcf_mk_pi(c));
  if (what == "e") return wrap_rcf(ctx, Z3_rcf_mk_e(c));
  return wrap_rcf(ctx, Z3_rcf_mk_infinitesimal(c));
}

// Roots of the polynomial a[0] + a[1] x + ... + a[n-1] x^(n-1).
// [[Rcpp::export(rng = false)]]
SEXP cpp_rcf_roots(Rcpp::List coeffs) {
  if (coeffs.size() == 0) fail("at least one coefficient is needed");
  CtxPtr ctx = rcf(coeffs[0])->ctx;
  std::vector<Z3_rcf_num> a;
  for (R_xlen_t i = 0; i < coeffs.size(); ++i) {
    RcfHolder* h = rcf(coeffs[i]);
    same_ctx(ctx, h->ctx);
    a.push_back(h->n);
  }
  std::vector<Z3_rcf_num> roots(a.size(), nullptr);
  unsigned n = Z3_rcf_mk_roots(ctx->c, static_cast<unsigned>(a.size()), a.data(), roots.data());
  // Own every returned root before checking for errors.
  std::vector<std::unique_ptr<RcfHolder>> owned;
  for (unsigned i = 0; i < n; ++i) owned.emplace_back(new RcfHolder(ctx, roots[i]));
  check(ctx);
  Rcpp::List out(n);
  for (unsigned i = 0; i < n; ++i) out[i] = make_xptr(owned[i].release(), Kind::Rcf);
  return out;
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_rcf_op(std::string op, SEXP a_, SEXP b_, int k) {
  RcfHolder* a = rcf(a_);
  const CtxPtr& ctx = a->ctx;
  Z3_context c = ctx->c;
  if (op == "neg") return wrap_rcf(ctx, Z3_rcf_neg(c, a->n));
  if (op == "inv") return wrap_rcf(ctx, Z3_rcf_inv(c, a->n));
  if (op == "power") {
    if (k < 0) fail("the exponent must be a non-negative integer");
    return wrap_rcf(ctx, Z3_rcf_power(c, a->n, static_cast<unsigned>(k)));
  }
  RcfHolder* b = rcf(b_);
  same_ctx(ctx, b->ctx);
  if (op == "+") return wrap_rcf(ctx, Z3_rcf_add(c, a->n, b->n));
  if (op == "-") return wrap_rcf(ctx, Z3_rcf_sub(c, a->n, b->n));
  if (op == "*") return wrap_rcf(ctx, Z3_rcf_mul(c, a->n, b->n));
  if (op == "/") return wrap_rcf(ctx, Z3_rcf_div(c, a->n, b->n));
  bool r;
  if (op == "<") r = Z3_rcf_lt(c, a->n, b->n);
  else if (op == ">") r = Z3_rcf_gt(c, a->n, b->n);
  else if (op == "<=") r = Z3_rcf_le(c, a->n, b->n);
  else if (op == ">=") r = Z3_rcf_ge(c, a->n, b->n);
  else if (op == "==") r = Z3_rcf_eq(c, a->n, b->n);
  else if (op == "!=") r = Z3_rcf_neq(c, a->n, b->n);
  else fail("unsupported operation on real closed field numbers");
  check(ctx);
  return Rcpp::wrap(r);
}

// [[Rcpp::export(rng = false)]]
std::string cpp_rcf_string(SEXP a_, bool compact, int decimals) {
  RcfHolder* a = rcf(a_);
  std::string out = decimals >= 0
      ? str(Z3_rcf_num_to_decimal_string(a->ctx->c, a->n, static_cast<unsigned>(decimals)))
      : str(Z3_rcf_num_to_string(a->ctx->c, a->n, compact, false));
  check(a->ctx);
  return out;
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_rcf_fraction(SEXP a_) {
  RcfHolder* a = rcf(a_);
  Z3_rcf_num n = nullptr, d = nullptr;
  Z3_rcf_get_numerator_denominator(a->ctx->c, a->n, &n, &d);
  std::unique_ptr<RcfHolder> hn(new RcfHolder(a->ctx, n)), hd(new RcfHolder(a->ctx, d));
  check(a->ctx);
  SEXP num = PROTECT(make_xptr(hn.release(), Kind::Rcf));
  SEXP den = PROTECT(make_xptr(hd.release(), Kind::Rcf));
  SEXP out = Rcpp::List::create(Rcpp::Named("numerator") = num, Rcpp::Named("denominator") = den);
  UNPROTECT(2);
  return out;
}

// Classification and extension details (Z3 4.12.3 and later).
// [[Rcpp::export(rng = false)]]
SEXP cpp_rcf_info(SEXP a_) {
#if Z3R_AT_LEAST(4, 12, 3)
  RcfHolder* a = rcf(a_);
  Z3_context c = a->ctx->c;
  std::string kind = Z3_rcf_is_rational(c, a->n) ? "rational"
                     : Z3_rcf_is_algebraic(c, a->n) ? "algebraic"
                     : Z3_rcf_is_infinitesimal(c, a->n) ? "infinitesimal"
                     : Z3_rcf_is_transcendental(c, a->n) ? "transcendental" : "other";
  Rcpp::List out = Rcpp::List::create(Rcpp::Named("kind") = kind);
  if (kind != "rational") {
    out["extension_index"] = static_cast<int>(Z3_rcf_extension_index(c, a->n));
  }
  if (kind == "transcendental") out["name"] = symbol_string(a->ctx, Z3_rcf_transcendental_name(c, a->n));
  if (kind == "infinitesimal") out["name"] = symbol_string(a->ctx, Z3_rcf_infinitesimal_name(c, a->n));
  if (kind == "algebraic") {
    unsigned nc = Z3_rcf_num_coefficients(c, a->n);
    Rcpp::List coeffs(nc);
    for (unsigned i = 0; i < nc; ++i) coeffs[i] = wrap_rcf(a->ctx, Z3_rcf_coefficient(c, a->n, i));
    out["coefficients"] = coeffs;
    // The flags are int before Z3 4.15.5 and bool from then on.
#if Z3R_AT_LEAST(4, 15, 5)
    bool lo_inf, lo_open, hi_inf, hi_open;
#else
    int lo_inf, lo_open, hi_inf, hi_open;
#endif
    Z3_rcf_num lo = nullptr, hi = nullptr;
    Z3_rcf_interval(c, a->n, &lo_inf, &lo_open, &lo, &hi_inf, &hi_open, &hi);
    std::unique_ptr<RcfHolder> hlo(new RcfHolder(a->ctx, lo)), hhi(new RcfHolder(a->ctx, hi));
    check(a->ctx);
    Rcpp::List iv = Rcpp::List::create(Rcpp::Named("lower_infinite") = lo_inf != 0,
                                       Rcpp::Named("lower_open") = lo_open != 0,
                                       Rcpp::Named("upper_infinite") = hi_inf != 0,
                                       Rcpp::Named("upper_open") = hi_open != 0);
    iv["lower"] = lo_inf ? R_NilValue : make_xptr(hlo.release(), Kind::Rcf);
    iv["upper"] = hi_inf ? R_NilValue : make_xptr(hhi.release(), Kind::Rcf);
    out["interval"] = iv;
    unsigned ns = Z3_rcf_num_sign_conditions(c, a->n);
    Rcpp::List conds(ns);
    for (unsigned i = 0; i < ns; ++i) {
      unsigned nk = Z3_rcf_num_sign_condition_coefficients(c, a->n, i);
      Rcpp::List cc(nk);
      for (unsigned j = 0; j < nk; ++j) cc[j] = wrap_rcf(a->ctx, Z3_rcf_sign_condition_coefficient(c, a->n, i, j));
      conds[i] = Rcpp::List::create(Rcpp::Named("sign") = Z3_rcf_sign_condition_sign(c, a->n, i),
                                    Rcpp::Named("coefficients") = cc);
    }
    out["sign_conditions"] = conds;
  }
  check(a->ctx);
  return out;
#else
  (void)a_;
  needs_version("z3_rcf_info()", "4.12.3");
#endif
}

// ---------------------------------------------------------------------------
// Algebraic numbers as Z3 expressions (z3_algebraic.h)

// [[Rcpp::export(rng = false)]]
SEXP cpp_algebraic_op(std::string op, SEXP a_, SEXP b_, int k) {
  const Ast& a = get_expr(a_);
  const CtxPtr& ctx = a.ctx();
  Z3_context c = ctx->c;
  auto need_value = [&](const Ast& x) {
    if (!Z3_algebraic_is_value(c, x)) fail("expected a numeral or an algebraic number");
  };
  need_value(a);
  if (op == "is_pos") return Rcpp::wrap(Z3_algebraic_is_pos(c, a));
  if (op == "is_neg") return Rcpp::wrap(Z3_algebraic_is_neg(c, a));
  if (op == "is_zero") return Rcpp::wrap(Z3_algebraic_is_zero(c, a));
  if (op == "sign") return Rcpp::wrap(Z3_algebraic_sign(c, a));
  if (op == "root" || op == "power") {
    if (k < 1 && op == "root") fail("the root index must be a positive integer");
    if (k < 0) fail("the exponent must be a non-negative integer");
    Z3_ast r = op == "root" ? Z3_algebraic_root(c, a, static_cast<unsigned>(k))
                            : Z3_algebraic_power(c, a, static_cast<unsigned>(k));
    return wrap_expr(own_ast(ctx, r));
  }
  Ast b = lift(ctx, b_, Z3_mk_real_sort(c));
  need_value(b);
  if (op == "+") return wrap_expr(own_ast(ctx, Z3_algebraic_add(c, a, b)));
  if (op == "-") return wrap_expr(own_ast(ctx, Z3_algebraic_sub(c, a, b)));
  if (op == "*") return wrap_expr(own_ast(ctx, Z3_algebraic_mul(c, a, b)));
  if (op == "/") {
    if (Z3_algebraic_is_zero(c, b)) fail("division by zero");
    return wrap_expr(own_ast(ctx, Z3_algebraic_div(c, a, b)));
  }
  bool r;
  if (op == "<") r = Z3_algebraic_lt(c, a, b);
  else if (op == ">") r = Z3_algebraic_gt(c, a, b);
  else if (op == "<=") r = Z3_algebraic_le(c, a, b);
  else if (op == ">=") r = Z3_algebraic_ge(c, a, b);
  else if (op == "==") r = Z3_algebraic_eq(c, a, b);
  else if (op == "!=") r = Z3_algebraic_neq(c, a, b);
  else fail("unsupported algebraic operation");
  check(ctx);
  return Rcpp::wrap(r);
}

// Real roots of polynomial p in its last variable after substituting the
// values a for the others (all variables are bound variables of p, see the
// Z3 documentation of Z3_algebraic_roots).
// [[Rcpp::export(rng = false)]]
SEXP cpp_algebraic_roots(SEXP p_, Rcpp::List values, bool eval) {
  const Ast& p = get_expr(p_);
  const CtxPtr& ctx = p.ctx();
  Z3_context c = ctx->c;
  AstVec v;
  for (R_xlen_t i = 0; i < values.size(); ++i) {
    Ast a = lift(ctx, values[i], Z3_mk_real_sort(c));
    if (!Z3_algebraic_is_value(c, a)) fail("values must be numerals or algebraic numbers");
    v.push_back(a);
  }
  std::vector<Z3_ast> r = raw(v);
  if (eval) {
    int s = Z3_algebraic_eval(c, p, static_cast<unsigned>(r.size()), r.empty() ? nullptr : r.data());
    check(ctx);
    return Rcpp::wrap(s);
  }
  return wrap_ast_vector(ctx, Z3_algebraic_roots(c, p, static_cast<unsigned>(r.size()),
                                                 r.empty() ? nullptr : r.data()));
}

// Defining polynomial (as coefficients) and root index of an algebraic number.
// [[Rcpp::export(rng = false)]]
SEXP cpp_algebraic_poly(SEXP a_) {
  const Ast& a = get_expr(a_);
  const CtxPtr& ctx = a.ctx();
  Z3_context c = ctx->c;
  if (!Z3_is_algebraic_number(c, a)) fail("expected an irrational algebraic number");
  SEXP coeffs = PROTECT(wrap_ast_vector(ctx, Z3_algebraic_get_poly(c, a)));
  unsigned i = Z3_algebraic_get_i(c, a);
  check(ctx);
  SEXP out = Rcpp::List::create(Rcpp::Named("coefficients") = coeffs, Rcpp::Named("index") = static_cast<int>(i));
  UNPROTECT(1);
  return out;
}

// Subresultants of polynomials p and q with respect to variable x.
// [[Rcpp::export(rng = false)]]
SEXP cpp_polynomial_subresultants(SEXP p_, SEXP q_, SEXP x_) {
  const Ast& p = get_expr(p_);
  const CtxPtr& ctx = p.ctx();
  Ast q = lift(ctx, q_, nullptr);
  const Ast& x = get_expr(x_);
  same_ctx(ctx, x.ctx());
  return wrap_ast_vector(ctx, Z3_polynomial_subresultants(ctx->c, p, q, x));
}
