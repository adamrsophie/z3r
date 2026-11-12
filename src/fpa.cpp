// IEEE 754 floating point theory.
#include "z3r.h"

#include <cmath>
#include <cstring>

using namespace z3r;

namespace z3r {

bool is_fp(const Ast& e) { return sort_kind(e) == Z3_FLOATING_POINT_SORT; }
bool is_rm(const Ast& e) { return sort_kind(e) == Z3_ROUNDING_MODE_SORT; }

// Exact R double for a floating point numeral: the value is widened to
// Float64 (exact when the format fits in a double) and read through its IEEE
// bit pattern. Returns false when the format is wider than a double and the
// value does not survive the round trip.
bool fp_to_double(const Ast& a, double* out) {
  const CtxPtr& ctx = a.ctx();
  Z3_context c = ctx->c;
  if (Z3_fpa_is_numeral_nan(c, a)) {
    *out = R_NaN;
    return true;
  }
  Z3_sort s = Z3_get_sort(c, a);
  unsigned eb = Z3_fpa_get_ebits(c, s), sb = Z3_fpa_get_sbits(c, s);
  check(ctx);
  Ast rne = own_ast(ctx, Z3_mk_fpa_rne(c));
  Z3_sort d = Z3_mk_fpa_sort_double(c);
  Ast as_double = (eb == 11 && sb == 53) ? a : own_ast(ctx, Z3_mk_fpa_to_fp_float(c, rne, a, d));
  Ast ieee = own_ast(ctx, Z3_mk_fpa_to_ieee_bv(c, as_double));
  Ast bits = own_ast(ctx, Z3_simplify(c, ieee));
  uint64_t u = 0;
  if (!Z3_get_numeral_uint64(c, bits, &u)) fail("could not read the floating point value");
  check(ctx);
  std::memcpy(out, &u, sizeof(double));
  if (eb <= 11 && sb <= 53) return true;
  Ast back = own_ast(ctx, Z3_mk_fpa_to_fp_float(c, rne, as_double, s));
  Ast same = own_ast(ctx, Z3_simplify(c, own_ast(ctx, Z3_mk_eq(c, back, a))));
  return Z3_get_bool_value(c, same) == Z3_L_TRUE;
}

// FP numeral for an R double, refusing values the format cannot represent.
Ast fp_literal(const CtxPtr& ctx, double v, bool is_na, Z3_sort sort) {
  Z3_context c = ctx->c;
  if (is_na) fail("NA cannot be used in a Z3 expression");
  if (std::isnan(v)) return own_ast(ctx, Z3_mk_fpa_nan(c, sort));
  if (std::isinf(v)) return own_ast(ctx, Z3_mk_fpa_inf(c, sort, v < 0));
  Ast a = own_ast(ctx, Z3_mk_fpa_numeral_double(c, v, sort));
  double back = 0;
  bool exact = fp_to_double(a, &back) && back == v && std::signbit(back) == std::signbit(v);
  if (!exact) {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.17g", v);
    fail(std::string("the double ") + buf + " is not exactly representable in " + sort_name(ctx, sort) +
         "; round it explicitly with z3_fp_val(ctx, value, sort, rounding)");
  }
  return a;
}

// Arithmetic with R's operators on floating point uses round-to-nearest,
// ties-to-even, as R's own double arithmetic does.
Ast fp_binary(const CtxPtr& ctx, const std::string& op, const Ast& a, const Ast& b) {
  Z3_context c = ctx->c;
  if (!is_fp(a) || !is_fp(b) || !Z3_is_eq_sort(c, Z3_get_sort(c, a), Z3_get_sort(c, b))) {
    fail("'" + op + "' needs floating point arguments of the same format, got " +
         sort_name(ctx, Z3_get_sort(c, a)) + " and " + sort_name(ctx, Z3_get_sort(c, b)) +
         "; convert with z3_fp_to_fp()");
  }
  Ast rne = own_ast(ctx, Z3_mk_fpa_rne(c));
  if (op == "+") return own_ast(ctx, Z3_mk_fpa_add(c, rne, a, b));
  if (op == "-") return own_ast(ctx, Z3_mk_fpa_sub(c, rne, a, b));
  if (op == "*") return own_ast(ctx, Z3_mk_fpa_mul(c, rne, a, b));
  if (op == "/") return own_ast(ctx, Z3_mk_fpa_div(c, rne, a, b));
  if (op == "%%") return own_ast(ctx, Z3_mk_fpa_rem(c, a, b));
  if (op == "<") return own_ast(ctx, Z3_mk_fpa_lt(c, a, b));
  if (op == "<=") return own_ast(ctx, Z3_mk_fpa_leq(c, a, b));
  if (op == ">") return own_ast(ctx, Z3_mk_fpa_gt(c, a, b));
  if (op == ">=") return own_ast(ctx, Z3_mk_fpa_geq(c, a, b));
  fail("'" + op + "' is not defined for floating point; see ?z3_fp");
}

}  // namespace z3r

// [[Rcpp::export(rng = false)]]
SEXP cpp_fp_sort(SEXP ctx_, int ebits, int sbits) {
  CtxPtr ctx = get_ctx(ctx_);
  if (ebits < 2 || sbits < 2) fail("floating point formats need at least 2 exponent and 2 significand bits");
  return wrap_sort(own_ast(ctx, sort_ast(Z3_mk_fpa_sort(ctx->c, static_cast<unsigned>(ebits),
                                                        static_cast<unsigned>(sbits)))));
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_rm_sort(SEXP ctx_) {
  CtxPtr ctx = get_ctx(ctx_);
  return wrap_sort(own_ast(ctx, sort_ast(Z3_mk_fpa_rounding_mode_sort(ctx->c))));
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_fp_sort_info(SEXP x) {
  Ast s = is_kind(x, Kind::Sort) ? get_sort(x) : sort_of(get_expr(x));
  Z3_context c = s.c();
  if (Z3_get_sort_kind(c, as_sort(s)) != Z3_FLOATING_POINT_SORT) fail("not a floating point sort");
  unsigned eb = Z3_fpa_get_ebits(c, as_sort(s)), sb = Z3_fpa_get_sbits(c, as_sort(s));
  check(s.ctx());
  return Rcpp::IntegerVector::create(Rcpp::Named("ebits") = static_cast<int>(eb),
                                     Rcpp::Named("sbits") = static_cast<int>(sb));
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_rounding_mode(SEXP ctx_, std::string mode) {
  CtxPtr ctx = get_ctx(ctx_);
  Z3_context c = ctx->c;
  Z3_ast r;
  if (mode == "RNE") r = Z3_mk_fpa_rne(c);
  else if (mode == "RNA") r = Z3_mk_fpa_rna(c);
  else if (mode == "RTP") r = Z3_mk_fpa_rtp(c);
  else if (mode == "RTN") r = Z3_mk_fpa_rtn(c);
  else if (mode == "RTZ") r = Z3_mk_fpa_rtz(c);
  else fail("rounding must be one of \"RNE\", \"RNA\", \"RTP\", \"RTN\", \"RTZ\"");
  return wrap_expr(own_ast(ctx, r));
}

// Special values and numerals of a floating point sort.
// [[Rcpp::export(rng = false)]]
SEXP cpp_fp_special(std::string what, SEXP sort_, bool negative) {
  const Ast& s = get_sort(sort_);
  Z3_context c = s.c();
  if (Z3_get_sort_kind(c, as_sort(s)) != Z3_FLOATING_POINT_SORT) fail("not a floating point sort");
  Z3_ast r = what == "nan" ? Z3_mk_fpa_nan(c, as_sort(s))
             : what == "inf" ? Z3_mk_fpa_inf(c, as_sort(s), negative)
                             : Z3_mk_fpa_zero(c, as_sort(s), negative);
  return wrap_expr(own_ast(s.ctx(), r));
}

// Numeral from a double, rounded with rm when the format cannot hold it.
// [[Rcpp::export(rng = false)]]
SEXP cpp_fp_from_double(SEXP ctx_, double v, SEXP sort_, SEXP rm_) {
  CtxPtr ctx = get_ctx(ctx_);
  const Ast& s = get_sort(sort_);
  same_ctx(ctx, s.ctx());
  Z3_context c = ctx->c;
  if (ISNA(v)) fail("NA cannot be used in a Z3 expression");
  if (Rf_isNull(rm_)) return wrap_expr(fp_literal(ctx, v, false, as_sort(s)));
  const Ast& rm = get_expr(rm_);
  if (!is_rm(rm)) fail("rounding must be a rounding mode");
  // Exact in Float64, then rounded into the target format.
  Ast d = std::isnan(v) ? own_ast(ctx, Z3_mk_fpa_nan(c, Z3_mk_fpa_sort_double(c)))
                        : own_ast(ctx, Z3_mk_fpa_numeral_double(c, v, Z3_mk_fpa_sort_double(c)));
  Ast r = own_ast(ctx, Z3_mk_fpa_to_fp_float(c, rm, d, as_sort(s)));
  return wrap_expr(own_ast(ctx, Z3_simplify(c, r)));
}

// fp(sign, exponent, significand) from bit-vectors.
// [[Rcpp::export(rng = false)]]
SEXP cpp_fp_from_bits(SEXP sgn_, SEXP exp_, SEXP sig_) {
  const Ast& sgn = get_expr(sgn_);
  const CtxPtr& ctx = sgn.ctx();
  const Ast& e = get_expr(exp_);
  const Ast& m = get_expr(sig_);
  same_ctx(ctx, e.ctx());
  same_ctx(ctx, m.ctx());
  return wrap_expr(own_ast(ctx, Z3_mk_fpa_fp(ctx->c, sgn, e, m)));
}

// Floating point operations. args holds expressions or R values; the
// rounding mode, when needed, comes first.
// [[Rcpp::export(rng = false)]]
SEXP cpp_fp_op(std::string op, SEXP args, SEXP sort_, int size) {
  CtxPtr ctx = ctx_from_list(args);
  Z3_context c = ctx->c;
  R_xlen_t n = Rf_xlength(args);
  Z3_sort fp_hint = nullptr;
  for (R_xlen_t i = 0; i < n; ++i) {
    SEXP a = VECTOR_ELT(args, i);
    if (is_kind(a, Kind::Expr) && is_fp(get_expr(a))) {
      fp_hint = Z3_get_sort(c, get_expr(a));
      break;
    }
  }
  auto arg = [&](R_xlen_t i) -> Ast {
    SEXP a = VECTOR_ELT(args, i);
    if (!is_kind(a, Kind::Expr) && TYPEOF(a) == REALSXP && Rf_xlength(a) == 1 && fp_hint) {
      return fp_literal(ctx, REAL(a)[0], ISNA(REAL(a)[0]), fp_hint);
    }
    return lift(ctx, a, fp_hint);
  };
  auto fp = [&](R_xlen_t i) {
    Ast a = arg(i);
    if (!is_fp(a)) fail("'" + op + "' needs floating point arguments, got " + sort_name(ctx, Z3_get_sort(c, a)));
    return a;
  };
  auto rm = [&](R_xlen_t i) {
    Ast a = arg(i);
    if (!is_rm(a)) fail("'" + op + "' needs a rounding mode as its first argument");
    return a;
  };
  auto same = [&](const Ast& a, const Ast& b) {
    if (!Z3_is_eq_sort(c, Z3_get_sort(c, a), Z3_get_sort(c, b))) {
      fail("'" + op + "' needs floating point arguments of the same format");
    }
  };
  auto target = [&]() -> Z3_sort {
    const Ast& s = get_sort(sort_);
    same_ctx(ctx, s.ctx());
    return as_sort(s);
  };
  typedef Z3_ast (*Un)(Z3_context, Z3_ast);
  typedef Z3_ast (*Bin)(Z3_context, Z3_ast, Z3_ast);
  typedef Z3_ast (*RmBin)(Z3_context, Z3_ast, Z3_ast, Z3_ast);
  struct U { const char* name; Un f; };
  static const U unary[] = {{"abs", Z3_mk_fpa_abs}, {"neg", Z3_mk_fpa_neg}, {"is_normal", Z3_mk_fpa_is_normal},
                            {"is_subnormal", Z3_mk_fpa_is_subnormal}, {"is_zero", Z3_mk_fpa_is_zero},
                            {"is_infinite", Z3_mk_fpa_is_infinite}, {"is_nan", Z3_mk_fpa_is_nan},
                            {"is_negative", Z3_mk_fpa_is_negative}, {"is_positive", Z3_mk_fpa_is_positive},
                            {"to_real", Z3_mk_fpa_to_real}, {"to_ieee_bv", Z3_mk_fpa_to_ieee_bv}};
  for (const U& u : unary) {
    if (op == u.name) return wrap_expr(own_ast(ctx, u.f(c, fp(0))));
  }
  struct B { const char* name; Bin f; };
  static const B binary[] = {{"rem", Z3_mk_fpa_rem}, {"min", Z3_mk_fpa_min}, {"max", Z3_mk_fpa_max},
                             {"leq", Z3_mk_fpa_leq}, {"lt", Z3_mk_fpa_lt}, {"geq", Z3_mk_fpa_geq},
                             {"gt", Z3_mk_fpa_gt}, {"eq", Z3_mk_fpa_eq}};
  for (const B& b : binary) {
    if (op == b.name) {
      Ast x = fp(0), y = fp(1);
      same(x, y);
      return wrap_expr(own_ast(ctx, b.f(c, x, y)));
    }
  }
  struct R { const char* name; RmBin f; };
  static const R rmbin[] = {{"add", Z3_mk_fpa_add}, {"sub", Z3_mk_fpa_sub}, {"mul", Z3_mk_fpa_mul},
                            {"div", Z3_mk_fpa_div}};
  for (const R& r : rmbin) {
    if (op == r.name) {
      Ast m = rm(0), x = fp(1), y = fp(2);
      same(x, y);
      return wrap_expr(own_ast(ctx, r.f(c, m, x, y)));
    }
  }
  if (op == "fma") {
    Ast m = rm(0), x = fp(1), y = fp(2), z = fp(3);
    same(x, y);
    same(x, z);
    return wrap_expr(own_ast(ctx, Z3_mk_fpa_fma(c, m, x, y, z)));
  }
  if (op == "sqrt") return wrap_expr(own_ast(ctx, Z3_mk_fpa_sqrt(c, rm(0), fp(1))));
  if (op == "round_to_integral") return wrap_expr(own_ast(ctx, Z3_mk_fpa_round_to_integral(c, rm(0), fp(1))));
  if (op == "to_ubv" || op == "to_sbv") {
    if (size < 1) fail("the bit-vector width must be positive");
    Ast m = rm(0), x = fp(1);
    return wrap_expr(own_ast(ctx, (op == "to_ubv" ? Z3_mk_fpa_to_ubv : Z3_mk_fpa_to_sbv)(
                                      c, m, x, static_cast<unsigned>(size))));
  }
  // Conversions to a floating point sort
  if (op == "from_ieee_bv") {
    Ast b = arg(0);
    if (sort_kind(b) != Z3_BV_SORT) fail("expected a bit-vector");
    return wrap_expr(own_ast(ctx, Z3_mk_fpa_to_fp_bv(c, b, target())));
  }
  if (op == "to_fp") {
    Ast m = rm(0);
    Ast x = lift(ctx, VECTOR_ELT(args, 1), nullptr);
    Z3_sort_kind k = sort_kind(x);
    Z3_sort t = target();
    if (k == Z3_FLOATING_POINT_SORT) return wrap_expr(own_ast(ctx, Z3_mk_fpa_to_fp_float(c, m, x, t)));
    if (k == Z3_REAL_SORT) return wrap_expr(own_ast(ctx, Z3_mk_fpa_to_fp_real(c, m, x, t)));
    if (k == Z3_INT_SORT) return wrap_expr(own_ast(ctx, Z3_mk_fpa_to_fp_real(c, m, to_real(x), t)));
    fail("z3_fp_to_fp() converts floating point, Real or Int values; use z3_fp_from_bv() for bit-vectors");
  }
  if (op == "from_signed" || op == "from_unsigned") {
    Ast m = rm(0);
    Ast b = arg(1);
    if (sort_kind(b) != Z3_BV_SORT) fail("expected a bit-vector");
    return wrap_expr(own_ast(ctx, (op == "from_signed" ? Z3_mk_fpa_to_fp_signed : Z3_mk_fpa_to_fp_unsigned)(
                                      c, m, b, target())));
  }
  if (op == "from_int_real") {
    Ast m = rm(0);
    Ast e = lift(ctx, VECTOR_ELT(args, 1), Z3_mk_int_sort(c));
    Ast s = lift(ctx, VECTOR_ELT(args, 2), Z3_mk_real_sort(c));
    if (sort_kind(s) == Z3_INT_SORT) s = to_real(s);
    return wrap_expr(own_ast(ctx, Z3_mk_fpa_to_fp_int_real(c, m, e, s, target())));
  }
  fail("unknown floating point operation '" + op + "'");
}

// Parts of a floating point numeral.
// [[Rcpp::export(rng = false)]]
SEXP cpp_fp_numeral_parts(SEXP x, bool biased) {
  const Ast& a = get_expr(x);
  const CtxPtr& ctx = a.ctx();
  Z3_context c = ctx->c;
  if (!is_fp(a) || !Z3_is_numeral_ast(c, a)) fail("not a floating point numeral");
  std::string cls = Z3_fpa_is_numeral_nan(c, a) ? "nan"
                    : Z3_fpa_is_numeral_inf(c, a) ? "infinite"
                    : Z3_fpa_is_numeral_zero(c, a) ? "zero"
                    : Z3_fpa_is_numeral_subnormal(c, a) ? "subnormal" : "normal";
  bool negative = Z3_fpa_is_numeral_negative(c, a);
  Rcpp::List out = Rcpp::List::create(Rcpp::Named("class") = cls, Rcpp::Named("negative") = negative);
  if (cls != "nan" && cls != "infinite") {
    out["significand"] = str(Z3_fpa_get_numeral_significand_string(c, a));
    out["exponent"] = str(Z3_fpa_get_numeral_exponent_string(c, a, biased));
  }
  if (cls != "nan") {
    out["sign_bv"] = wrap_expr(own_ast(ctx, Z3_fpa_get_numeral_sign_bv(c, a)));
    out["exponent_bv"] = wrap_expr(own_ast(ctx, Z3_fpa_get_numeral_exponent_bv(c, a, biased)));
    out["significand_bv"] = wrap_expr(own_ast(ctx, Z3_fpa_get_numeral_significand_bv(c, a)));
  }
  check(ctx);
  return out;
}
