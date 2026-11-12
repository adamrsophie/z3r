// Theories added in Z3 5.x: complex numbers, finite fields, finite sets.
#include "z3r.h"

using namespace z3r;

#define Z3R_V5_COMPLEX Z3R_AT_LEAST(5, 3, 2)
#define Z3R_V5_FF Z3R_AT_LEAST(5, 3, 0)
#define Z3R_V5_FSET Z3R_AT_LEAST(5, 0, 0)

// [[Rcpp::export(rng = false)]]
SEXP cpp_v5_sort(SEXP ctx_, std::string what, SEXP arg) {
  CtxPtr ctx = get_ctx(ctx_);
  Z3_context c = ctx->c;
  (void)c;
  if (what == "complex") {
#if Z3R_V5_COMPLEX
    return wrap_sort(own_ast(ctx, sort_ast(Z3_mk_complex_sort(c))));
#else
    needs_version("z3_complex_sort()", "5.3.2");
#endif
  }
  if (what == "finite_field") {
#if Z3R_V5_FF
    std::string p = as_string(arg, "prime");
    return wrap_sort(own_ast(ctx, sort_ast(Z3_mk_finite_field_sort(c, p.c_str()))));
#else
    (void)arg;
    needs_version("z3_ff_sort()", "5.3.0");
#endif
  }
#if Z3R_V5_FSET
  const Ast& e = get_sort(arg);
  same_ctx(ctx, e.ctx());
  return wrap_sort(own_ast(ctx, sort_ast(Z3_mk_finite_set_sort(c, as_sort(e)))));
#else
  (void)arg;
  needs_version("z3_finite_set_sort()", "5.0.0");
#endif
}

// Size of a finite field (decimal string) or element sort of a finite set.
// [[Rcpp::export(rng = false)]]
SEXP cpp_v5_sort_info(SEXP sort_) {
  const Ast& s = get_sort(sort_);
  Z3_context c = s.c();
#if Z3R_V5_FSET
  if (Z3_is_finite_set_sort(c, as_sort(s))) {
    return wrap_sort(own_ast(s.ctx(), sort_ast(Z3_get_finite_set_sort_basis(c, as_sort(s)))));
  }
#endif
#if Z3R_V5_FF
  if (Z3_get_sort_kind(c, as_sort(s)) == Z3_FINITE_FIELD_SORT) {
    std::string n = str(Z3_get_finite_field_sort_size(c, as_sort(s)));
    check(s.ctx());
    return Rcpp::wrap(n);
  }
#endif
  (void)c;
  fail("not a finite field or finite set sort (or this Z3 is too old for them)");
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_v5_op(std::string op, SEXP args) {
  CtxPtr ctx = ctx_from_list(args);
  Z3_context c = ctx->c;
  R_xlen_t n = Rf_xlength(args);
  auto arg = [&](R_xlen_t i, Z3_sort hint = nullptr) { return lift(ctx, VECTOR_ELT(args, i), hint); };
  (void)c;
  (void)n;
  // Complex numbers
  if (op == "complex" || op == "real_part" || op == "imag_part") {
#if Z3R_V5_COMPLEX
    if (op == "complex") {
      Ast re = arg(0, Z3_mk_real_sort(c)), im = arg(1, Z3_mk_real_sort(c));
      if (!is_arith(re) || !is_arith(im)) fail("the parts of a complex number must be Int or Real");
      if (sort_kind(re) == Z3_INT_SORT) re = to_real(re);
      if (sort_kind(im) == Z3_INT_SORT) im = to_real(im);
      return wrap_expr(own_ast(ctx, Z3_mk_complex(c, re, im)));
    }
    Ast z = arg(0);
    return wrap_expr(own_ast(ctx, op == "real_part" ? Z3_mk_real_part(c, z) : Z3_mk_imag_part(c, z)));
#else
    (void)arg;
    needs_version("complex numbers", "5.3.2");
#endif
  }
  // Finite fields
  if (op == "ff_add" || op == "ff_mul" || op == "ff_bitsum" || op == "ff_neg") {
#if Z3R_V5_FF
    if (op == "ff_neg") return wrap_expr(own_ast(ctx, Z3_mk_ff_neg(c, arg(0))));
    AstVec v;
    Z3_sort hint = nullptr;
    for (R_xlen_t i = 0; i < n; ++i) {
      SEXP a = VECTOR_ELT(args, i);
      if (is_kind(a, Kind::Expr)) { hint = Z3_get_sort(c, get_expr(a)); break; }
    }
    for (R_xlen_t i = 0; i < n; ++i) v.push_back(arg(i, hint));
    std::vector<Z3_ast> r = raw(v);
    Z3_ast out = op == "ff_add" ? Z3_mk_ff_add(c, static_cast<unsigned>(r.size()), r.data())
                 : op == "ff_mul" ? Z3_mk_ff_mul(c, static_cast<unsigned>(r.size()), r.data())
                                  : Z3_mk_ff_bitsum(c, static_cast<unsigned>(r.size()), r.data());
    return wrap_expr(own_ast(ctx, out));
#else
    (void)arg;
    needs_version("finite fields", "5.3.0");
#endif
  }
  // Finite sets
#if Z3R_V5_FSET
  if (op == "fs_singleton") return wrap_expr(own_ast(ctx, Z3_mk_finite_set_singleton(c, arg(0))));
  if (op == "fs_size") return wrap_expr(own_ast(ctx, Z3_mk_finite_set_size(c, arg(0))));
  if (op == "fs_range") {
    Ast lo = arg(0, Z3_mk_int_sort(c)), hi = arg(1, Z3_mk_int_sort(c));
    return wrap_expr(own_ast(ctx, Z3_mk_finite_set_range(c, lo, hi)));
  }
  if (op == "fs_member") {
    Ast s = arg(1);
    Ast e = arg(0, Z3_get_finite_set_sort_basis(c, Z3_get_sort(c, s)));
    return wrap_expr(own_ast(ctx, Z3_mk_finite_set_member(c, e, s)));
  }
  if (op == "fs_map" || op == "fs_filter") {
    Ast f = arg(0), s = arg(1);
    return wrap_expr(own_ast(ctx, op == "fs_map" ? Z3_mk_finite_set_map(c, f, s) : Z3_mk_finite_set_filter(c, f, s)));
  }
  typedef Z3_ast (*Bin)(Z3_context, Z3_ast, Z3_ast);
  Bin f = op == "fs_union" ? Z3_mk_finite_set_union : op == "fs_intersect" ? Z3_mk_finite_set_intersect
          : op == "fs_difference" ? Z3_mk_finite_set_difference : op == "fs_subset" ? Z3_mk_finite_set_subset
                                                                                    : nullptr;
  if (f) return wrap_expr(own_ast(ctx, f(c, arg(0), arg(1))));
#else
  if (op.rfind("fs_", 0) == 0) needs_version("finite sets", "5.0.0");
#endif
  fail("unknown operation '" + op + "'");
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_fs_empty(SEXP sort_) {
#if Z3R_V5_FSET
  const Ast& s = get_sort(sort_);
  return wrap_expr(own_ast(s.ctx(), Z3_mk_finite_set_empty(s.c(), as_sort(s))));
#else
  (void)sort_;
  needs_version("finite sets", "5.0.0");
#endif
}
