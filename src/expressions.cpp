// Constants, numerals, literal conversion, sort checking and operators.
#include "z3r.h"

#include <set>

#include <cerrno>
#include <climits>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace z3r {

static const double TWO53 = 9007199254740992.0;  // 2^53

bool is_bool(const Ast& e) { return sort_kind(e) == Z3_BOOL_SORT; }
bool is_arith(const Ast& e) {
  Z3_sort_kind k = sort_kind(e);
  return k == Z3_INT_SORT || k == Z3_REAL_SORT;
}
static bool is_int(const Ast& e) { return sort_kind(e) == Z3_INT_SORT; }
static bool is_real(const Ast& e) { return sort_kind(e) == Z3_REAL_SORT; }
static bool is_bv(const Ast& e) { return sort_kind(e) == Z3_BV_SORT; }
static bool is_string(const Ast& e) {
  if (sort_kind(e) != Z3_SEQ_SORT) return false;
  bool s = Z3_is_string_sort(e.c(), Z3_get_sort(e.c(), e));
  check(e.ctx());
  return s;
}
static unsigned bv_width(const Ast& e) {
  unsigned w = Z3_get_bv_sort_size(e.c(), Z3_get_sort(e.c(), e));
  check(e.ctx());
  return w;
}

static std::string describe(const Ast& e) {
  return sort_name(e.ctx(), Z3_get_sort(e.c(), e));
}

// Numeral of a machine integer, made without decimal strings. The caller
// has checked that `v` fits `sort`.
static Ast machine_numeral(const CtxPtr& ctx, int64_t v, Z3_sort sort) {
  Z3_context c = ctx->c;
  Z3_ast r;
  if (v >= 0) {
    r = v <= static_cast<int64_t>(UINT_MAX) ? Z3_mk_unsigned_int(c, static_cast<unsigned>(v), sort)
                      : Z3_mk_unsigned_int64(c, static_cast<uint64_t>(v), sort);
  } else {
    r = v >= INT_MIN ? Z3_mk_int(c, static_cast<int>(v), sort) : Z3_mk_int64(c, v, sort);
  }
  return own_ast(ctx, r);
}

Ast to_real(const Ast& e) {
  if (is_real(e)) return e;
  if (!is_int(e)) fail("cannot convert an expression of sort " + describe(e) + " to Real");
  Z3_context c = e.c();
  if (Z3_is_numeral_ast(c, e)) {
    // Keep literals readable: 2 becomes 2.0 rather than (to_real 2).
    Z3_sort real = Z3_mk_real_sort(c);
    int64_t n = 0, d = 1;
    if (Z3_get_numeral_small(c, e, &n, &d)) return machine_numeral(e.ctx(), n, real);
    check(e.ctx());
    std::string v = str(Z3_get_numeral_string(c, e));
    return own_ast(e.ctx(), Z3_mk_numeral(c, v.c_str(), real));
  }
  return own_ast(e.ctx(), Z3_mk_int2real(c, e));
}

// Complex numbers (Z3 5.3.2): Int and Real embed exactly.
static bool is_complex(const Ast& e) {
#if Z3R_AT_LEAST(5, 3, 2)
  return sort_kind(e) == Z3_COMPLEX_SORT;
#else
  (void)e;
  return false;
#endif
}
static bool is_numeric(const Ast& e) { return is_arith(e) || is_complex(e); }
static Ast to_complex(const Ast& e) {
#if Z3R_AT_LEAST(5, 3, 2)
  if (is_complex(e)) return e;
  Ast re = to_real(e);
  Ast zero = own_ast(e.ctx(), Z3_mk_numeral(e.c(), "0", Z3_mk_real_sort(e.c())));
  return own_ast(e.ctx(), Z3_mk_complex(e.c(), re, zero));
#else
  return e;
#endif
}

Ast coerce_sort(const Ast& v, Z3_sort sort, const std::string& what) {
  Z3_context c = v.c();
  Ast out = v;
  if (Z3_get_sort_kind(c, sort) == Z3_REAL_SORT && sort_kind(out) == Z3_INT_SORT) out = to_real(out);
  if (!Z3_is_eq_sort(c, sort, Z3_get_sort(c, out))) {
    fail(what + " has the wrong sort: " + sort_name(v.ctx(), Z3_get_sort(c, out)) + " where " +
         sort_name(v.ctx(), sort) + " is needed");
  }
  return out;
}

namespace {
void collect_free_vars(Z3_context c, Z3_ast a, unsigned depth, std::vector<Z3_sort>& out,
                       std::set<std::pair<unsigned, unsigned>>& seen) {
  if (!seen.insert(std::make_pair(Z3_get_ast_id(c, a), depth)).second) return;
  switch (Z3_get_ast_kind(c, a)) {
    case Z3_VAR_AST: {
      unsigned i = Z3_get_index_value(c, a);
      if (i >= depth) {
        unsigned k = i - depth;
        if (out.size() <= k) out.resize(k + 1, nullptr);
        Z3_sort srt = Z3_get_sort(c, a);
        if (out[k] && !Z3_is_eq_sort(c, out[k], srt)) {
          fail("bound variable " + std::to_string(k) + " occurs with two different sorts");
        }
        out[k] = srt;
      }
      break;
    }
    case Z3_APP_AST: {
      Z3_app app = Z3_to_app(c, a);
      unsigned n = Z3_get_app_num_args(c, app);
      for (unsigned k = 0; k < n; ++k) collect_free_vars(c, Z3_get_app_arg(c, app, k), depth, out, seen);
      break;
    }
    case Z3_QUANTIFIER_AST:
      collect_free_vars(c, Z3_get_quantifier_body(c, a), depth + Z3_get_quantifier_num_bound(c, a), out, seen);
      break;
    default:
      break;
  }
}
}  // namespace

std::vector<Z3_sort> free_var_sorts(Z3_context c, Z3_ast a) {
  std::vector<Z3_sort> out;
  std::set<std::pair<unsigned, unsigned>> seen;
  collect_free_vars(c, a, 0, out, seen);
  return out;
}

// Context of the expressions in an argument list. All of them must share it:
// callers take sort hints for R literals from any of them before lifting.
CtxPtr ctx_from_list(SEXP args) {
  R_xlen_t n = Rf_xlength(args);
  CtxPtr ctx;
  for (R_xlen_t i = 0; i < n; ++i) {
    SEXP a = VECTOR_ELT(args, i);
    if (!is_kind(a, Kind::Expr)) continue;
    const CtxPtr& c = get_expr(a).ctx();
    if (!ctx) ctx = c;
    else same_ctx(ctx, c);
  }
  if (!ctx) fail("at least one argument must be a Z3 expression (z3_expr)");
  return ctx;
}

// ---------------------------------------------------------------------------
// Numerals

static Ast mk_numeral(const CtxPtr& ctx, const std::string& s, Z3_sort sort) {
  return own_ast(ctx, Z3_mk_numeral(ctx->c, s.c_str(), sort));
}

// Shortest decimal string that converts back to the same double.
static std::string shortest_decimal(double v) {
  char buf[64];
  for (int prec = 1; prec <= 17; ++prec) {
    std::snprintf(buf, sizeof(buf), "%.*g", prec, v);
    if (std::strtod(buf, nullptr) == v) break;
  }
  return std::string(buf);
}

// Decimal digits of 2^k, computed exactly.
static std::string pow2_digits(unsigned k) {
  std::string d = "1";  // least significant digit first
  for (unsigned i = 0; i < k; ++i) {
    int carry = 0;
    for (char& ch : d) {
      int v = (ch - '0') * 2 + carry;
      ch = static_cast<char>('0' + v % 10);
      carry = v / 10;
    }
    if (carry) d.push_back(static_cast<char>('0' + carry));
  }
  return std::string(d.rbegin(), d.rend());
}

// Compares non-negative decimal digit strings without leading zeros.
static int cmp_digits(const std::string& a, const std::string& b) {
  if (a.size() != b.size()) return a.size() < b.size() ? -1 : 1;
  return a.compare(b) < 0 ? -1 : a.compare(b) > 0 ? 1 : 0;
}

static Ast bv_numeral(const CtxPtr& ctx, std::string digits, bool negative, Z3_sort sort) {
  unsigned w = Z3_get_bv_sort_size(ctx->c, sort);
  check(ctx);
  size_t nz = digits.find_first_not_of('0');
  digits = nz == std::string::npos ? "0" : digits.substr(nz);
  // Accept the unsigned range [0, 2^w) and the signed range [-2^(w-1), 0).
  bool ok = negative ? cmp_digits(digits, pow2_digits(w - 1)) <= 0
                     : cmp_digits(digits, pow2_digits(w)) < 0;
  if (!ok) {
    fail("the value " + std::string(negative ? "-" : "") + digits +
         " does not fit in a bit-vector of width " + std::to_string(w));
  }
  return mk_numeral(ctx, (negative ? "-" : "") + digits, sort);
}

// Numeral for an exact integer given as decimal digits, in the sort that
// matches `hint` (Int when there is no arithmetic hint).
static Ast int_literal(const CtxPtr& ctx, const std::string& s, Z3_sort hint) {
  if (hint) {
    Z3_sort_kind k = Z3_get_sort_kind(ctx->c, hint);
    if (k == Z3_REAL_SORT) return mk_numeral(ctx, s, hint);
    if (k == Z3_BV_SORT) {
      bool neg = !s.empty() && s[0] == '-';
      return bv_numeral(ctx, neg ? s.substr(1) : s, neg, hint);
    }
#if Z3R_AT_LEAST(5, 3, 0)
    if (k == Z3_FINITE_FIELD_SORT) return mk_numeral(ctx, s, hint);
#endif
    if (k == Z3_FINITE_DOMAIN_SORT) {
      // Elements are 0, ..., size - 1.
      uint64_t size = 0;
      Z3_get_finite_domain_sort_size(ctx->c, hint, &size);
      check(ctx);
      bool neg = !s.empty() && s[0] == '-';
      errno = 0;
      unsigned long long v = neg ? 0 : std::strtoull(s.c_str(), nullptr, 10);
      if (neg || errno == ERANGE || v >= size) {
        fail("element " + s + " is outside the finite domain (0 to " + std::to_string(size - 1) + ")");
      }
      return mk_numeral(ctx, s, hint);
    }
  }
  return mk_numeral(ctx, s, Z3_mk_int_sort(ctx->c));
}

// The same for a machine integer, made without decimal strings.
static Ast int_literal(const CtxPtr& ctx, int64_t v, Z3_sort hint) {
  Z3_context c = ctx->c;
  if (hint) {
    Z3_sort_kind k = Z3_get_sort_kind(c, hint);
    if (k == Z3_REAL_SORT) return machine_numeral(ctx, v, hint);
    if (k == Z3_BV_SORT) {
      unsigned w = Z3_get_bv_sort_size(c, hint);
      check(ctx);
      // Accept the unsigned range [0, 2^w) and the signed range [-2^(w-1), 0).
      bool ok = w >= 64 || (v >= 0 ? static_cast<uint64_t>(v) < (UINT64_C(1) << w)
                                   : v >= -static_cast<int64_t>(UINT64_C(1) << (w - 1)));
      if (!ok) {
        fail("the value " + std::to_string(static_cast<long long>(v)) +
             " does not fit in a bit-vector of width " + std::to_string(w));
      }
      return machine_numeral(ctx, v, hint);
    }
#if Z3R_AT_LEAST(5, 3, 0)
    if (k == Z3_FINITE_FIELD_SORT) return mk_numeral(ctx, std::to_string(static_cast<long long>(v)), hint);
#endif
    if (k == Z3_FINITE_DOMAIN_SORT) {
      // Elements are 0, ..., size - 1.
      uint64_t size = 0;
      Z3_get_finite_domain_sort_size(c, hint, &size);
      check(ctx);
      if (v < 0 || static_cast<uint64_t>(v) >= size) {
        fail("element " + std::to_string(static_cast<long long>(v)) + " is outside the finite domain (0 to " +
             std::to_string(size - 1) + ")");
      }
      return machine_numeral(ctx, v, hint);
    }
  }
  return machine_numeral(ctx, v, Z3_mk_int_sort(c));
}

// Code points of a UTF-8 string, so that string literals are exact.
static std::vector<unsigned> code_points(const char* utf8) {
  std::vector<unsigned> cps;
  const unsigned char* p = reinterpret_cast<const unsigned char*>(utf8);
  while (*p) {
    unsigned cp;
    int extra;
    if (*p < 0x80) { cp = *p; extra = 0; }
    else if ((*p & 0xE0) == 0xC0) { cp = *p & 0x1F; extra = 1; }
    else if ((*p & 0xF0) == 0xE0) { cp = *p & 0x0F; extra = 2; }
    else if ((*p & 0xF8) == 0xF0) { cp = *p & 0x07; extra = 3; }
    else fail("invalid UTF-8 in string literal");
    ++p;
    for (int i = 0; i < extra; ++i, ++p) {
      if ((*p & 0xC0) != 0x80) fail("invalid UTF-8 in string literal");
      cp = (cp << 6) | (*p & 0x3F);
    }
    // Reject overlong encodings and surrogates, which would change the text.
    static const unsigned min_cp[] = {0, 0x80, 0x800, 0x10000};
    if (cp < min_cp[extra] || (cp >= 0xD800 && cp <= 0xDFFF)) fail("invalid UTF-8 in string literal");
    // Z3's characters are the code points 0 to 0x2FFFF.
    if (cp > 0x2FFFF) {
      char buf[16];
      std::snprintf(buf, sizeof(buf), "U+%X", cp);
      fail(std::string("the character ") + buf + " is beyond U+2FFFF, the largest character Z3 strings can hold");
    }
    cps.push_back(cp);
  }
  return cps;
}

// Z3_mk_string would interpret backslash escapes; code points do not.
static Ast mk_string_literal(const CtxPtr& ctx, SEXP chr) {
  std::vector<unsigned> cps = code_points(Rf_translateCharUTF8(chr));
  return own_ast(ctx, Z3_mk_u32string(ctx->c, static_cast<unsigned>(cps.size()),
                                      cps.empty() ? nullptr : cps.data()));
}

static std::string gmp_string(SEXP x) {
  Rcpp::Function as_character("as.character");
  Rcpp::CharacterVector s = as_character(x);
  if (s.size() != 1 || Rcpp::CharacterVector::is_na(s[0])) {
    fail("gmp values used in Z3 expressions must be single non-missing numbers");
  }
  return Rcpp::as<std::string>(s[0]);
}

Ast lift(const CtxPtr& ctx, SEXP x, Z3_sort hint) {
  if (is_kind(x, Kind::Expr)) {
    const Ast& e = get_expr(x);
    same_ctx(ctx, e.ctx());
    return e;
  }
  if (TYPEOF(x) == EXTPTRSXP) fail("expected a Z3 expression, not another kind of Z3 object");
  if (Rf_inherits(x, "bigz")) {
    return int_literal(ctx, gmp_string(x), hint);
  }
  if (Rf_inherits(x, "bigq")) {
    std::string s = gmp_string(x);
    if (s.find('/') == std::string::npos) {
      return int_literal(ctx, s, hint);
    }
    return mk_numeral(ctx, s, Z3_mk_real_sort(ctx->c));
  }
  if (Rf_inherits(x, "integer64")) {
    // bit64 stores 64-bit integers in the bits of a double.
    if (Rf_xlength(x) != 1 || TYPEOF(x) != REALSXP) fail("integer64 values must have length 1");
    int64_t v;
    std::memcpy(&v, &REAL(x)[0], sizeof v);
    if (v == INT64_MIN) fail("NA cannot be used in a Z3 expression");
    return int_literal(ctx, v, hint);
  }
  if (Rf_isFactor(x)) fail("factors cannot be used in Z3 expressions");
  if (Rf_xlength(x) != 1) {
    fail("R values combined with Z3 expressions must have length 1 (got length " +
         std::to_string(static_cast<long long>(Rf_xlength(x))) +
         "); z3r expressions are scalars, so build element-wise constraints with "
         "lapply() or Map()");
  }
  switch (TYPEOF(x)) {
    case LGLSXP: {
      int v = LOGICAL(x)[0];
      if (v == NA_LOGICAL) fail("NA cannot be used in a Z3 expression");
      return own_ast(ctx, v ? Z3_mk_true(ctx->c) : Z3_mk_false(ctx->c));
    }
    case INTSXP: {
      int v = INTEGER(x)[0];
      if (v == NA_INTEGER) fail("NA cannot be used in a Z3 expression");
      if (hint && Z3_get_sort_kind(ctx->c, hint) == Z3_FLOATING_POINT_SORT) {
        return fp_literal(ctx, static_cast<double>(v), false, hint);
      }
      return int_literal(ctx, static_cast<int64_t>(v), hint);
    }
    case REALSXP: {
      double v = REAL(x)[0];
      if (hint && Z3_get_sort_kind(ctx->c, hint) == Z3_FLOATING_POINT_SORT) {
        return fp_literal(ctx, v, ISNA(v), hint);
      }
      if (ISNAN(v)) fail("NA and NaN cannot be used in a Z3 expression");
      if (!std::isfinite(v)) fail("Inf cannot be used in a Z3 expression");
      if (v == std::floor(v)) {
        if (std::fabs(v) > TWO53) {
          fail("the double " + shortest_decimal(v) +
               " is beyond 2^53, so R may already have rounded it; give large "
               "integers exactly as strings with z3_int_val() or as gmp::bigz");
        }
        return int_literal(ctx, static_cast<int64_t>(v), hint);
      }
      Rcpp::Function getopt("getOption");
      std::string policy = Rcpp::as<std::string>(getopt("z3r.doubles", "decimal"));
      if (policy == "error") {
        fail("the non-integer double " + shortest_decimal(v) +
             " cannot be used because option z3r.doubles is \"error\"; use "
             "z3_real_val() with an exact decimal or fraction string");
      } else if (policy != "decimal") {
        fail("option z3r.doubles must be \"decimal\" or \"error\"");
      }
      if (hint && Z3_get_sort_kind(ctx->c, hint) == Z3_BV_SORT) {
        fail("bit-vector literals must be whole numbers");
      }
      return mk_numeral(ctx, shortest_decimal(v), Z3_mk_real_sort(ctx->c));
    }
    case CPLXSXP: {
#if Z3R_AT_LEAST(5, 3, 2)
      Rcomplex v = COMPLEX(x)[0];
      Z3_sort real = Z3_mk_real_sort(ctx->c);
      SEXP re_r = PROTECT(Rf_ScalarReal(v.r));
      SEXP im_r = PROTECT(Rf_ScalarReal(v.i));
      Ast re, im;
      try {
        re = to_real(lift(ctx, re_r, real));
        im = to_real(lift(ctx, im_r, real));
      } catch (...) {
        UNPROTECT(2);
        throw;
      }
      UNPROTECT(2);
      return own_ast(ctx, Z3_mk_complex(ctx->c, re, im));
#else
      needs_version("R complex numbers in Z3 expressions", "5.3.2");
#endif
    }
    case STRSXP: {
      SEXP chr = STRING_ELT(x, 0);
      if (chr == NA_STRING) fail("NA cannot be used in a Z3 expression");
      if (hint) {
        Z3_sort_kind k = Z3_get_sort_kind(ctx->c, hint);
        if (k == Z3_INT_SORT || k == Z3_REAL_SORT || k == Z3_BV_SORT) {
          fail("character values are Z3 string literals; to give an exact number "
               "use z3_int_val(), z3_real_val() or z3_bv_val()");
        }
      }
      return mk_string_literal(ctx, chr);
    }
    default:
      fail(std::string("values of R type '") + Rf_type2char(TYPEOF(x)) +
           "' cannot be used in a Z3 expression");
  }
}

// ---------------------------------------------------------------------------
// Sort checking

static std::string op_label(const std::string& op) { return "'" + op + "'"; }

static void need_bool(const Ast& e, const std::string& op) {
  if (!is_bool(e)) {
    fail(op_label(op) + " needs Bool arguments, got " + describe(e));
  }
}

static void need_arith(const Ast& e, const std::string& op) {
  if (!is_arith(e)) {
    std::string hint;
    if (is_bv(e)) hint = "; bit-vectors use the z3_bv*() functions";
    if (is_bool(e)) hint = "; use z3_ite(p, 1, 0) to count Booleans";
    fail(op_label(op) + " needs Int or Real arguments, got " + describe(e) + hint);
  }
}

static void need_bv(const Ast& e, const std::string& op) {
  if (!is_bv(e)) fail(op_label(op) + " needs bit-vector arguments, got " + describe(e));
}

static void need_string(const Ast& e, const std::string& op) {
  if (!is_string(e)) fail(op_label(op) + " needs String arguments, got " + describe(e));
}

static bool is_seq(const Ast& e) { return sort_kind(e) == Z3_SEQ_SORT; }

// Strings are sequences of characters; these operations accept any sequence.
static void need_seq(const Ast& e, const std::string& op) {
  if (!is_seq(e)) fail(op_label(op) + " needs String or sequence arguments, got " + describe(e));
}

static void need_same_bv(const Ast& a, const Ast& b, const std::string& op) {
  need_bv(a, op);
  need_bv(b, op);
  if (bv_width(a) != bv_width(b)) {
    fail(op_label(op) + " needs bit-vectors of equal width, got " + describe(a) +
         " and " + describe(b) + "; use z3_zero_ext() or z3_sign_ext() to widen");
  }
}

// Promotes an Int/Real pair to a common sort (Int is exactly embedded in Real).
static void unify_arith(Ast& a, Ast& b, const std::string& op) {
  need_arith(a, op);
  need_arith(b, op);
  if (is_real(a) != is_real(b)) {
    a = to_real(a);
    b = to_real(b);
  }
}

// Makes two operands of an equality or ite comparable.
static void unify_any(Ast& a, Ast& b, const std::string& op) {
  if ((is_complex(a) || is_complex(b)) && is_numeric(a) && is_numeric(b)) {
    a = to_complex(a);
    b = to_complex(b);
    return;
  }
  if (is_arith(a) && is_arith(b)) {
    unify_arith(a, b, op);
    return;
  }
  if (is_bv(a) && is_bv(b)) {
    need_same_bv(a, b, op);
    return;
  }
  bool same = Z3_is_eq_sort(a.c(), Z3_get_sort(a.c(), a), Z3_get_sort(b.c(), b));
  check(a.ctx());
  if (!same) {
    fail(op_label(op) + " needs arguments of the same sort, got " + describe(a) +
         " and " + describe(b));
  }
}

// Lifts two operands; each R literal takes the sort of the other operand.
static void lift_pair(const CtxPtr& ctx, SEXP x, SEXP y, Ast& a, Ast& b) {
  if (is_kind(x, Kind::Expr)) {
    a = lift(ctx, x, nullptr);
    b = lift(ctx, y, Z3_get_sort(a.c(), a));
  } else {
    b = lift(ctx, y, nullptr);
    a = lift(ctx, x, Z3_get_sort(b.c(), b));
  }
}

// Lifts a list whose literals take the sort of the first expression.
static AstVec lift_all(const CtxPtr& ctx, SEXP args) {
  R_xlen_t n = Rf_xlength(args);
  Z3_sort hint = nullptr;
  Ast first;
  for (R_xlen_t i = 0; i < n; ++i) {
    if (is_kind(VECTOR_ELT(args, i), Kind::Expr)) {
      first = get_expr(VECTOR_ELT(args, i));
      hint = Z3_get_sort(first.c(), first);
      break;
    }
  }
  AstVec out;
  out.reserve(n);
  for (R_xlen_t i = 0; i < n; ++i) out.push_back(lift(ctx, VECTOR_ELT(args, i), hint));
  return out;
}

static void unify_all_arith(AstVec& v, const std::string& op) {
  bool any_real = false;
  for (const Ast& e : v) {
    need_arith(e, op);
    if (is_real(e)) any_real = true;
  }
  if (any_real) {
    for (Ast& e : v) e = to_real(e);
  }
}

static void unify_all(AstVec& v, const std::string& op) {
  if (v.empty()) return;
  bool all_arith = true;
  for (const Ast& e : v) all_arith = all_arith && is_arith(e);
  if (all_arith) {
    unify_all_arith(v, op);
    return;
  }
  for (size_t i = 1; i < v.size(); ++i) {
    Ast a = v[0];
    unify_any(a, v[i], op);
  }
}

static unsigned as_uint(int v, const char* what) {
  if (v < 0) fail(std::string(what) + " must be a non-negative integer");
  return static_cast<unsigned>(v);
}

// ---------------------------------------------------------------------------
// Operators

typedef Z3_ast (*NaryFn)(Z3_context, unsigned, const Z3_ast[]);
typedef Z3_ast (*BinFn)(Z3_context, Z3_ast, Z3_ast);
typedef Z3_ast (*UnFn)(Z3_context, Z3_ast);

static Ast nary(const CtxPtr& ctx, NaryFn f, const AstVec& v) {
  std::vector<Z3_ast> r = raw(v);
  return own_ast(ctx, f(ctx->c, static_cast<unsigned>(r.size()), r.data()));
}

static Ast power(const CtxPtr& ctx, Ast a, SEXP exponent_r, Ast b) {
  need_arith(a, "^");
  need_arith(b, "^");
  // A small non-negative integer literal exponent is expanded into a product
  // so that the result keeps the base's sort (Z3's power operator is Real).
  bool literal = !is_kind(exponent_r, Kind::Expr) &&
                 (TYPEOF(exponent_r) == INTSXP || TYPEOF(exponent_r) == REALSXP);
  if (literal) {
    double k = Rf_asReal(exponent_r);
    if (k >= 0 && k == std::floor(k) && k <= 64) {
      if (k == 0) return own_ast(ctx, Z3_mk_numeral(ctx->c, "1", Z3_get_sort(a.c(), a)));
      AstVec f(static_cast<size_t>(k), a);
      return f.size() == 1 ? a : nary(ctx, Z3_mk_mul, f);
    }
  }
  a = to_real(a);
  b = to_real(b);
  return own_ast(ctx, Z3_mk_power(ctx->c, a, b));
}

static Ast binary(const CtxPtr& ctx, const std::string& op, SEXP x, SEXP y) {
  Ast a, b;
  lift_pair(ctx, x, y, a, b);
  Z3_context c = ctx->c;

  if (op == "==" || op == "!=") {
    unify_any(a, b, op);
    Ast eq = own_ast(ctx, Z3_mk_eq(c, a, b));
    return op == "==" ? eq : own_ast(ctx, Z3_mk_not(c, eq));
  }
  if (op == "&" || op == "|") {
    if (is_bv(a) || is_bv(b)) {
      need_same_bv(a, b, op);
      return own_ast(ctx, op == "&" ? Z3_mk_bvand(c, a, b) : Z3_mk_bvor(c, a, b));
    }
    need_bool(a, op);
    need_bool(b, op);
    Z3_ast r[2] = {a, b};
    return own_ast(ctx, op == "&" ? Z3_mk_and(c, 2, r) : Z3_mk_or(c, 2, r));
  }
  if (is_fp(a) || is_fp(b)) return fp_binary(ctx, op, a, b);
  if (is_bv(a) || is_bv(b)) {
    need_same_bv(a, b, op);
    if (op == "+") return own_ast(ctx, Z3_mk_bvadd(c, a, b));
    if (op == "-") return own_ast(ctx, Z3_mk_bvsub(c, a, b));
    if (op == "*") return own_ast(ctx, Z3_mk_bvmul(c, a, b));
    fail(op_label(op) + " is not defined for bit-vectors because signed and "
         "unsigned semantics differ; use z3_bvult(), z3_bvslt(), z3_bvudiv(), "
         "z3_bvsdiv() and related functions");
  }
  if (is_complex(a) || is_complex(b)) {
    if (!is_numeric(a) || !is_numeric(b)) {
      fail(op_label(op) + " needs Int, Real or Complex arguments, got " + describe(a) + " and " + describe(b));
    }
    a = to_complex(a);
    b = to_complex(b);
    Z3_ast r[2] = {a, b};
    if (op == "+") return own_ast(ctx, Z3_mk_add(c, 2, r));
    if (op == "-") return own_ast(ctx, Z3_mk_sub(c, 2, r));
    if (op == "*") return own_ast(ctx, Z3_mk_mul(c, 2, r));
    if (op == "/") return own_ast(ctx, Z3_mk_div(c, a, b));
    fail(op_label(op) + " is not defined for complex numbers (only +, -, *, / and equality are)");
  }
  if (op == "^") return power(ctx, a, y, b);
  if (op == "%/%" || op == "%%") {
    need_arith(a, op);
    need_arith(b, op);
    if (!is_int(a) || !is_int(b)) fail(op_label(op) + " needs Int arguments");
    return own_ast(ctx, op == "%/%" ? Z3_mk_div(c, a, b) : Z3_mk_mod(c, a, b));
  }
  unify_arith(a, b, op);
  if (op == "+") { Z3_ast r[2] = {a, b}; return own_ast(ctx, Z3_mk_add(c, 2, r)); }
  if (op == "-") { Z3_ast r[2] = {a, b}; return own_ast(ctx, Z3_mk_sub(c, 2, r)); }
  if (op == "*") { Z3_ast r[2] = {a, b}; return own_ast(ctx, Z3_mk_mul(c, 2, r)); }
  if (op == "/") {
    // Division in R is real division, also for integers.
    a = to_real(a);
    b = to_real(b);
    return own_ast(ctx, Z3_mk_div(c, a, b));
  }
  if (op == "<") return own_ast(ctx, Z3_mk_lt(c, a, b));
  if (op == "<=") return own_ast(ctx, Z3_mk_le(c, a, b));
  if (op == ">") return own_ast(ctx, Z3_mk_gt(c, a, b));
  if (op == ">=") return own_ast(ctx, Z3_mk_ge(c, a, b));
  fail("operator " + op_label(op) + " is not supported for Z3 expressions");
}

static Ast unary(const CtxPtr& ctx, const std::string& op, const Ast& a) {
  Z3_context c = ctx->c;
  if (op == "!") {
    if (is_bv(a)) return own_ast(ctx, Z3_mk_bvnot(c, a));
    need_bool(a, op);
    return own_ast(ctx, Z3_mk_not(c, a));
  }
  if (op == "-") {
    if (is_bv(a)) return own_ast(ctx, Z3_mk_bvneg(c, a));
    if (is_fp(a)) return own_ast(ctx, Z3_mk_fpa_neg(c, a));
    if (!is_complex(a)) need_arith(a, op);
    return own_ast(ctx, Z3_mk_unary_minus(c, a));
  }
  if (op == "+") {
    if (!is_bv(a) && !is_fp(a) && !is_complex(a)) need_arith(a, op);
    return a;
  }
  fail("unary operator " + op_label(op) + " is not supported for Z3 expressions");
}

}  // namespace z3r

using namespace z3r;

// ---------------------------------------------------------------------------
// Exported constructors

// A constant's name: a string, or a whole number from 0 to 2^30 - 1 for an
// integer symbol, which Z3 prints as k!n.
static Z3_symbol const_symbol(const CtxPtr& ctx, SEXP name) {
  if ((TYPEOF(name) == INTSXP || TYPEOF(name) == REALSXP) && !Rf_isObject(name)) {
    double v = Rf_xlength(name) == 1 ? Rf_asReal(name) : NA_REAL;
    if (ISNAN(v) || v != std::floor(v) || v < 0 || v > 1073741823.0) {
      fail("a numeric name must be a single whole number from 0 to 2^30 - 1");
    }
    Z3_symbol sym = Z3_mk_int_symbol(ctx->c, static_cast<int>(v));
    check(ctx);
    return sym;
  }
  return mk_symbol(ctx, as_string(name, "name"));
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_mk_const(SEXP ctx_, SEXP name_, SEXP sort_) {
  CtxPtr ctx = get_ctx(ctx_);
  const Ast& s = get_sort(sort_);
  same_ctx(ctx, s.ctx());
  return wrap_expr(own_ast(ctx, Z3_mk_const(ctx->c, const_symbol(ctx, name_), as_sort(s))));
}

// Fresh constant with a unique name derived from `prefix`.
// [[Rcpp::export(rng = false)]]
SEXP cpp_mk_fresh_const(SEXP ctx_, SEXP prefix_, SEXP sort_) {
  CtxPtr ctx = get_ctx(ctx_);
  const Ast& s = get_sort(sort_);
  same_ctx(ctx, s.ctx());
  std::string prefix = as_string(prefix_, "prefix");
  return wrap_expr(own_ast(ctx, Z3_mk_fresh_const(ctx->c, prefix.c_str(), as_sort(s))));
}

// Numeral from a string already validated in R.
// [[Rcpp::export(rng = false)]]
SEXP cpp_mk_numeral(SEXP ctx_, SEXP value_, SEXP sort_) {
  CtxPtr ctx = get_ctx(ctx_);
  const Ast& s = get_sort(sort_);
  same_ctx(ctx, s.ctx());
  std::string value = as_string(value_, "value");
  return wrap_expr(mk_numeral(ctx, value, as_sort(s)));
}

// Bit-vector numeral with range checking.
// [[Rcpp::export(rng = false)]]
SEXP cpp_mk_bv_numeral(SEXP ctx_, SEXP value_, int width) {
  CtxPtr ctx = get_ctx(ctx_);
  if (width < 1) fail("bit-vector width must be a positive integer");
  Z3_sort s = Z3_mk_bv_sort(ctx->c, static_cast<unsigned>(width));
  check(ctx);
  std::string v = as_string(value_, "value");
  bool neg = !v.empty() && v[0] == '-';
  std::string digits = neg ? v.substr(1) : v;
  return wrap_expr(bv_numeral(ctx, digits, neg, s));
}

// An R whole number as an exact 64-bit integer: an integer, a double of
// absolute value at most 2^53, or a bit64::integer64.
static int64_t whole_int64(SEXP x, const char* what) {
  if (Rf_xlength(x) == 1) {
    if (Rf_inherits(x, "integer64")) {
      int64_t v = INT64_MIN;
      if (TYPEOF(x) == REALSXP) std::memcpy(&v, &REAL(x)[0], sizeof v);
      if (v != INT64_MIN) return v;
    } else if (TYPEOF(x) == INTSXP && !Rf_isObject(x)) {
      if (INTEGER(x)[0] != NA_INTEGER) return INTEGER(x)[0];
    } else if (TYPEOF(x) == REALSXP && !Rf_isObject(x)) {
      double v = REAL(x)[0];
      if (std::isfinite(v) && v == std::floor(v) && std::fabs(v) <= TWO53) return static_cast<int64_t>(v);
    }
  }
  fail(std::string(what) + " must be a single whole number: an integer, a double of absolute value at "
       "most 2^53, or a bit64::integer64; give larger values as a string such as \"1/3\"");
}

// The rational num / den, built from machine integers.
// [[Rcpp::export(rng = false)]]
SEXP cpp_mk_real_ratio(SEXP ctx_, SEXP num_, SEXP den_) {
  CtxPtr ctx = get_ctx(ctx_);
  int64_t n = whole_int64(num_, "value"), d = whole_int64(den_, "den");
  if (d == 0) fail("the denominator must not be zero");
  // Both are above INT64_MIN (integer64 NA), so negating is safe.
  if (d < 0) {
    n = -n;
    d = -d;
  }
  Z3_context c = ctx->c;
  if (n >= -INT_MAX && n <= INT_MAX && d <= INT_MAX) {
    return wrap_expr(own_ast(ctx, Z3_mk_real(c, static_cast<int>(n), static_cast<int>(d))));
  }
#if Z3R_AT_LEAST(4, 12, 2)
  return wrap_expr(own_ast(ctx, Z3_mk_real_int64(c, n, d)));
#else
  return wrap_expr(mk_numeral(ctx, std::to_string(static_cast<long long>(n)) + "/" +
                                       std::to_string(static_cast<long long>(d)),
                              Z3_mk_real_sort(c)));
#endif
}

// String literal from SMT-LIB text, whose \u{...} escapes Z3 decodes. Z3
// reads one character per byte, so characters beyond ASCII are passed as
// escapes too.
// [[Rcpp::export(rng = false)]]
SEXP cpp_mk_string_escaped(SEXP ctx_, SEXP value_) {
  CtxPtr ctx = get_ctx(ctx_);
  std::string value = as_string(value_, "value");
  std::string text;
  for (unsigned cp : code_points(value.c_str())) {
    if (cp < 0x80) {
      text += static_cast<char>(cp);
    } else {
      char buf[16];
      std::snprintf(buf, sizeof(buf), "\\u{%x}", cp);
      text += buf;
    }
  }
  return wrap_expr(own_ast(ctx, Z3_mk_string(ctx->c, text.c_str())));
}

// String literal with one character per byte (code points 0 to 255).
// [[Rcpp::export(rng = false)]]
SEXP cpp_mk_lstring(SEXP ctx_, Rcpp::RawVector bytes) {
  CtxPtr ctx = get_ctx(ctx_);
  if (bytes.size() > static_cast<R_xlen_t>(UINT_MAX)) fail("the raw vector is too long for a Z3 string");
  const char* p = bytes.size() ? reinterpret_cast<const char*>(RAW(bytes)) : "";
  return wrap_expr(own_ast(ctx, Z3_mk_lstring(ctx->c, static_cast<unsigned>(bytes.size()), p)));
}

// R scalar to Z3 value of a given sort (or natural sort when sort is NULL).
// [[Rcpp::export(rng = false)]]
SEXP cpp_lift(SEXP ctx_, SEXP x, SEXP sort_) {
  CtxPtr ctx = get_ctx(ctx_);
  Z3_sort hint = nullptr;
  if (!Rf_isNull(sort_)) {
    const Ast& s = get_sort(sort_);
    same_ctx(ctx, s.ctx());
    hint = as_sort(s);
  }
  Ast a = lift(ctx, x, hint);
  if (hint) {
    Z3_sort_kind want = Z3_get_sort_kind(ctx->c, hint);
    if (want == Z3_REAL_SORT && is_int(a)) a = to_real(a);
    bool same = Z3_is_eq_sort(ctx->c, Z3_get_sort(ctx->c, a), hint);
    check(ctx);
    if (!same) {
      fail("cannot convert this value to sort " + sort_name(ctx, hint) +
           " (it has sort " + describe(a) + ")");
    }
  }
  return wrap_expr(a);
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_binary(std::string op, SEXP x, SEXP y) {
  CtxPtr ctx = is_kind(x, Kind::Expr) ? get_expr(x).ctx() : get_expr(y).ctx();
  return wrap_expr(binary(ctx, op, x, y));
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_unary(std::string op, SEXP x) {
  const Ast& a = get_expr(x);
  return wrap_expr(unary(a.ctx(), op, a));
}

// N-ary and named operations. `args` is a list of expressions or R scalars;
// `params` holds integer parameters (widths, indices).
// [[Rcpp::export(rng = false)]]
SEXP cpp_op(std::string op, SEXP args, Rcpp::IntegerVector params) {
  CtxPtr ctx = ctx_from_list(args);
  Z3_context c = ctx->c;
  R_xlen_t n = Rf_xlength(args);
  auto nargs = [&](R_xlen_t k) {
    if (n != k) fail("'" + op + "' takes " + std::to_string(static_cast<long long>(k)) + " arguments");
  };
  auto nparams = [&](R_xlen_t k) {
    if (params.size() != k) fail("internal error: wrong parameter count for " + op);
  };

  // Logical connectives
  if (op == "and" || op == "or") {
    AstVec v = lift_all(ctx, args);
    for (const Ast& e : v) need_bool(e, op);
    if (v.empty()) return wrap_expr(own_ast(ctx, op == "and" ? Z3_mk_true(c) : Z3_mk_false(c)));
    return wrap_expr(nary(ctx, op == "and" ? Z3_mk_and : Z3_mk_or, v));
  }
  if (op == "not") {
    nargs(1);
    Ast a = lift(ctx, VECTOR_ELT(args, 0), nullptr);
    need_bool(a, op);
    return wrap_expr(own_ast(ctx, Z3_mk_not(c, a)));
  }
  if (op == "implies" || op == "xor" || op == "iff") {
    nargs(2);
    AstVec v = lift_all(ctx, args);
    need_bool(v[0], op);
    need_bool(v[1], op);
    BinFn f = op == "implies" ? Z3_mk_implies : op == "xor" ? Z3_mk_xor : Z3_mk_iff;
    return wrap_expr(own_ast(ctx, f(c, v[0], v[1])));
  }
  if (op == "ite") {
    nargs(3);
    Ast cond = lift(ctx, VECTOR_ELT(args, 0), nullptr);
    need_bool(cond, op);
    Ast a, b;
    lift_pair(ctx, VECTOR_ELT(args, 1), VECTOR_ELT(args, 2), a, b);
    unify_any(a, b, op);
    return wrap_expr(own_ast(ctx, Z3_mk_ite(c, cond, a, b)));
  }
  if (op == "distinct") {
    AstVec v = lift_all(ctx, args);
    if (v.size() < 2) fail("z3_distinct() needs at least two arguments");
    unify_all(v, op);
    return wrap_expr(nary(ctx, Z3_mk_distinct, v));
  }

  // Arithmetic
  if (op == "sum" || op == "prod") {
    AstVec v = lift_all(ctx, args);
    if (v.empty()) fail("'" + op + "' needs at least one argument");
    bool all_bv = true;
    for (const Ast& e : v) all_bv = all_bv && is_bv(e);
    if (all_bv) {
      Ast acc = v[0];
      for (size_t i = 1; i < v.size(); ++i) {
        need_same_bv(acc, v[i], op);
        acc = own_ast(ctx, op == "sum" ? Z3_mk_bvadd(c, acc, v[i]) : Z3_mk_bvmul(c, acc, v[i]));
      }
      return wrap_expr(acc);
    }
    bool any_complex = false;
    for (const Ast& e : v) any_complex = any_complex || is_complex(e);
    if (any_complex) {
      for (Ast& e : v) {
        if (!is_numeric(e)) fail("'" + op + "' needs Int, Real or Complex arguments, got " + describe(e));
        e = to_complex(e);
      }
    } else {
      unify_all_arith(v, op);
    }
    if (v.size() == 1) return wrap_expr(v[0]);
    return wrap_expr(nary(ctx, op == "sum" ? Z3_mk_add : Z3_mk_mul, v));
  }
  if (op == "div" || op == "mod" || op == "rem") {
    nargs(2);
    AstVec v = lift_all(ctx, args);
    for (const Ast& e : v) {
      if (!is_int(e)) fail("'" + op + "' needs Int arguments, got " + describe(e));
    }
    BinFn f = op == "div" ? Z3_mk_div : op == "mod" ? Z3_mk_mod : Z3_mk_rem;
    return wrap_expr(own_ast(ctx, f(c, v[0], v[1])));
  }
  if (op == "abs") {
    nargs(1);
    Ast a = lift(ctx, VECTOR_ELT(args, 0), nullptr);
    if (is_fp(a)) return wrap_expr(own_ast(ctx, Z3_mk_fpa_abs(c, a)));
    need_arith(a, op);
#if Z3R_AT_LEAST(4, 13, 2)
    return wrap_expr(own_ast(ctx, Z3_mk_abs(c, a)));
#else
    Ast zero = own_ast(ctx, Z3_mk_numeral(c, "0", Z3_get_sort(c, a)));
    Ast neg = own_ast(ctx, Z3_mk_lt(c, a, zero));
    Ast minus = own_ast(ctx, Z3_mk_unary_minus(c, a));
    return wrap_expr(own_ast(ctx, Z3_mk_ite(c, neg, minus, a)));
#endif
  }
  if (op == "to_real") {
    nargs(1);
    Ast a = lift(ctx, VECTOR_ELT(args, 0), nullptr);
    need_arith(a, op);
    return wrap_expr(to_real(a));
  }
  if (op == "to_int" || op == "is_int") {
    nargs(1);
    Ast a = lift(ctx, VECTOR_ELT(args, 0), nullptr);
    need_arith(a, op);
    a = to_real(a);
    return wrap_expr(own_ast(ctx, op == "to_int" ? Z3_mk_real2int(c, a) : Z3_mk_is_int(c, a)));
  }

  // Bit-vectors
  struct BvBin { const char* name; BinFn f; };
  static const BvBin bv_bins[] = {
      {"bvand", Z3_mk_bvand},   {"bvor", Z3_mk_bvor},     {"bvxor", Z3_mk_bvxor},
      {"bvnand", Z3_mk_bvnand}, {"bvnor", Z3_mk_bvnor},   {"bvxnor", Z3_mk_bvxnor},
      {"bvadd", Z3_mk_bvadd},   {"bvsub", Z3_mk_bvsub},   {"bvmul", Z3_mk_bvmul},
      {"bvudiv", Z3_mk_bvudiv}, {"bvsdiv", Z3_mk_bvsdiv}, {"bvurem", Z3_mk_bvurem},
      {"bvsrem", Z3_mk_bvsrem}, {"bvsmod", Z3_mk_bvsmod}, {"bvshl", Z3_mk_bvshl},
      {"bvlshr", Z3_mk_bvlshr}, {"bvashr", Z3_mk_bvashr}, {"bvult", Z3_mk_bvult},
      {"bvule", Z3_mk_bvule},   {"bvugt", Z3_mk_bvugt},   {"bvuge", Z3_mk_bvuge},
      {"bvslt", Z3_mk_bvslt},   {"bvsle", Z3_mk_bvsle},   {"bvsgt", Z3_mk_bvsgt},
      {"bvsge", Z3_mk_bvsge},   {"bvsdiv_no_overflow", Z3_mk_bvsdiv_no_overflow},
      {"rotate_left_by", Z3_mk_ext_rotate_left}, {"rotate_right_by", Z3_mk_ext_rotate_right}};
  for (const BvBin& b : bv_bins) {
    if (op == b.name) {
      nargs(2);
      AstVec v = lift_all(ctx, args);
      need_same_bv(v[0], v[1], op);
      return wrap_expr(own_ast(ctx, b.f(c, v[0], v[1])));
    }
  }
  if (op == "bvadd_no_overflow" || op == "bvmul_no_overflow" ||
      op == "bvsub_no_underflow") {
    nargs(2);
    nparams(1);
    AstVec v = lift_all(ctx, args);
    need_same_bv(v[0], v[1], op);
    bool is_signed = params[0] != 0;
    if (op == "bvadd_no_overflow") return wrap_expr(own_ast(ctx, Z3_mk_bvadd_no_overflow(c, v[0], v[1], is_signed)));
    if (op == "bvmul_no_overflow") return wrap_expr(own_ast(ctx, Z3_mk_bvmul_no_overflow(c, v[0], v[1], is_signed)));
    return wrap_expr(own_ast(ctx, Z3_mk_bvsub_no_underflow(c, v[0], v[1], is_signed)));
  }
  if (op == "bvadd_no_underflow" || op == "bvmul_no_underflow" ||
      op == "bvsub_no_overflow") {
    nargs(2);
    AstVec v = lift_all(ctx, args);
    need_same_bv(v[0], v[1], op);
    BinFn f = op == "bvadd_no_underflow" ? Z3_mk_bvadd_no_underflow
              : op == "bvmul_no_underflow" ? Z3_mk_bvmul_no_underflow
                                           : Z3_mk_bvsub_no_overflow;
    return wrap_expr(own_ast(ctx, f(c, v[0], v[1])));
  }
  if (op == "bvnot" || op == "bvneg" || op == "bvredand" || op == "bvredor") {
    nargs(1);
    Ast a = lift(ctx, VECTOR_ELT(args, 0), nullptr);
    need_bv(a, op);
    UnFn f = op == "bvnot" ? Z3_mk_bvnot : op == "bvneg" ? Z3_mk_bvneg
             : op == "bvredand" ? Z3_mk_bvredand : Z3_mk_bvredor;
    return wrap_expr(own_ast(ctx, f(c, a)));
  }
  if (op == "concat") {
    AstVec v = lift_all(ctx, args);
    if (v.empty()) fail("z3_concat() needs at least one argument");
    bool all_bv = true, all_str = true;
    for (const Ast& e : v) {
      all_bv = all_bv && is_bv(e);
      all_str = all_str && is_seq(e) && Z3_is_eq_sort(c, Z3_get_sort(c, e), Z3_get_sort(c, v[0]));
    }
    if (all_str) {
      if (v.size() == 1) return wrap_expr(v[0]);
      return wrap_expr(nary(ctx, Z3_mk_seq_concat, v));
    }
    if (!all_bv) fail("z3_concat() needs all bit-vector arguments or sequences of one sort");
    Ast acc = v[0];
    for (size_t i = 1; i < v.size(); ++i) acc = own_ast(ctx, Z3_mk_concat(c, acc, v[i]));
    return wrap_expr(acc);
  }
  if (op == "extract") {
    nargs(1);
    nparams(2);
    Ast a = lift(ctx, VECTOR_ELT(args, 0), nullptr);
    need_bv(a, op);
    unsigned hi = as_uint(params[0], "high"), lo = as_uint(params[1], "low");
    if (hi < lo || hi >= bv_width(a)) {
      fail("z3_extract() needs width > high >= low >= 0 (width " +
           std::to_string(bv_width(a)) + ")");
    }
    return wrap_expr(own_ast(ctx, Z3_mk_extract(c, hi, lo, a)));
  }
  if (op == "zero_ext" || op == "sign_ext" || op == "rotate_left" ||
      op == "rotate_right" || op == "repeat") {
    nargs(1);
    nparams(1);
    Ast a = lift(ctx, VECTOR_ELT(args, 0), nullptr);
    need_bv(a, op);
    unsigned k = as_uint(params[0], "the count");
    if (op == "zero_ext") return wrap_expr(own_ast(ctx, Z3_mk_zero_ext(c, k, a)));
    if (op == "sign_ext") return wrap_expr(own_ast(ctx, Z3_mk_sign_ext(c, k, a)));
    if (op == "rotate_left") return wrap_expr(own_ast(ctx, Z3_mk_rotate_left(c, k, a)));
    if (op == "rotate_right") return wrap_expr(own_ast(ctx, Z3_mk_rotate_right(c, k, a)));
    if (k == 0) fail("z3_repeat() needs a positive count");
    return wrap_expr(own_ast(ctx, Z3_mk_repeat(c, k, a)));
  }
  if (op == "bv2int") {
    nargs(1);
    nparams(1);
    Ast a = lift(ctx, VECTOR_ELT(args, 0), nullptr);
    need_bv(a, op);
    return wrap_expr(own_ast(ctx, Z3_mk_bv2int(c, a, params[0] != 0)));
  }
  if (op == "int2bv") {
    nargs(1);
    nparams(1);
    Ast a = lift(ctx, VECTOR_ELT(args, 0), nullptr);
    if (!is_int(a)) fail("z3_int2bv() needs an Int argument, got " + describe(a));
    if (params[0] < 1) fail("bit-vector width must be a positive integer");
    return wrap_expr(own_ast(ctx, Z3_mk_int2bv(c, static_cast<unsigned>(params[0]), a)));
  }

  // Arrays
  if (op == "select") {
    nargs(2);
    Ast a = lift(ctx, VECTOR_ELT(args, 0), nullptr);
    if (sort_kind(a) != Z3_ARRAY_SORT) fail("z3_select() needs an array, got " + describe(a));
    Z3_sort dom = Z3_get_array_sort_domain(c, Z3_get_sort(c, a));
    Ast i = lift(ctx, VECTOR_ELT(args, 1), dom);
    if (Z3_get_sort_kind(c, dom) == Z3_REAL_SORT && is_int(i)) i = to_real(i);
    if (!Z3_is_eq_sort(c, Z3_get_sort(c, i), dom)) {
      fail("array index has sort " + describe(i) + " but the array domain is " + sort_name(ctx, dom));
    }
    return wrap_expr(own_ast(ctx, Z3_mk_select(c, a, i)));
  }
  if (op == "store") {
    nargs(3);
    Ast a = lift(ctx, VECTOR_ELT(args, 0), nullptr);
    if (sort_kind(a) != Z3_ARRAY_SORT) fail("z3_store() needs an array, got " + describe(a));
    Z3_sort dom = Z3_get_array_sort_domain(c, Z3_get_sort(c, a));
    Z3_sort ran = Z3_get_array_sort_range(c, Z3_get_sort(c, a));
    Ast i = lift(ctx, VECTOR_ELT(args, 1), dom);
    Ast v = lift(ctx, VECTOR_ELT(args, 2), ran);
    if (Z3_get_sort_kind(c, dom) == Z3_REAL_SORT && is_int(i)) i = to_real(i);
    if (Z3_get_sort_kind(c, ran) == Z3_REAL_SORT && is_int(v)) v = to_real(v);
    if (!Z3_is_eq_sort(c, Z3_get_sort(c, i), dom)) {
      fail("array index has sort " + describe(i) + " but the array domain is " + sort_name(ctx, dom));
    }
    if (!Z3_is_eq_sort(c, Z3_get_sort(c, v), ran)) {
      fail("stored value has sort " + describe(v) + " but the array range is " + sort_name(ctx, ran));
    }
    return wrap_expr(own_ast(ctx, Z3_mk_store(c, a, i, v)));
  }

  // Strings
  if (op == "length") {
    nargs(1);
    Ast a = lift(ctx, VECTOR_ELT(args, 0), nullptr);
    need_seq(a, op);
    return wrap_expr(own_ast(ctx, Z3_mk_seq_length(c, a)));
  }
  if (op == "contains" || op == "prefixof" || op == "suffixof" || op == "str_lt" ||
      op == "str_le" || op == "str_at") {
    nargs(2);
    if (op == "str_at") {
      Ast s = lift(ctx, VECTOR_ELT(args, 0), nullptr);
      need_seq(s, op);
      Ast i = lift(ctx, VECTOR_ELT(args, 1), Z3_mk_int_sort(c));
      if (!is_int(i)) fail("string positions must be Int");
      return wrap_expr(own_ast(ctx, Z3_mk_seq_at(c, s, i)));
    }
    AstVec v = lift_all(ctx, args);
    bool lex = op == "str_lt" || op == "str_le";
    if (lex) {
      need_string(v[0], op);
      need_string(v[1], op);
    } else {
      need_seq(v[0], op);
      need_seq(v[1], op);
      if (!Z3_is_eq_sort(c, Z3_get_sort(c, v[0]), Z3_get_sort(c, v[1]))) {
        fail(op_label(op) + " needs sequences of the same sort");
      }
    }
    BinFn f = op == "contains" ? Z3_mk_seq_contains : op == "prefixof" ? Z3_mk_seq_prefix
              : op == "suffixof" ? Z3_mk_seq_suffix : op == "str_lt" ? Z3_mk_str_lt
                                                                     : Z3_mk_str_le;
    return wrap_expr(own_ast(ctx, f(c, v[0], v[1])));
  }
  if (op == "substr" || op == "indexof" || op == "replace") {
    nargs(3);
    Ast s = lift(ctx, VECTOR_ELT(args, 0), nullptr);
    need_seq(s, op);
    if (op == "replace") {
      Ast a = lift(ctx, VECTOR_ELT(args, 1), Z3_get_sort(c, s));
      Ast b = lift(ctx, VECTOR_ELT(args, 2), Z3_get_sort(c, s));
      need_seq(a, op);
      need_seq(b, op);
      return wrap_expr(own_ast(ctx, Z3_mk_seq_replace(c, s, a, b)));
    }
    Z3_sort I = Z3_mk_int_sort(c);
    if (op == "substr") {
      Ast off = lift(ctx, VECTOR_ELT(args, 1), I);
      Ast len = lift(ctx, VECTOR_ELT(args, 2), I);
      if (!is_int(off) || !is_int(len)) fail("z3_substr() needs Int offset and length");
      return wrap_expr(own_ast(ctx, Z3_mk_seq_extract(c, s, off, len)));
    }
    Ast sub = lift(ctx, VECTOR_ELT(args, 1), Z3_get_sort(c, s));
    need_seq(sub, op);
    Ast off = lift(ctx, VECTOR_ELT(args, 2), I);
    if (!is_int(off)) fail("z3_indexof() needs an Int offset");
    return wrap_expr(own_ast(ctx, Z3_mk_seq_index(c, s, sub, off)));
  }
  if (op == "str_to_int" || op == "int_to_str") {
    nargs(1);
    Ast a = lift(ctx, VECTOR_ELT(args, 0), nullptr);
    if (op == "str_to_int") {
      need_string(a, op);
      return wrap_expr(own_ast(ctx, Z3_mk_str_to_int(c, a)));
    }
    if (!is_int(a)) fail("z3_int_to_str() needs an Int argument, got " + describe(a));
    return wrap_expr(own_ast(ctx, Z3_mk_int_to_str(c, a)));
  }

  fail("unknown operation '" + op + "'");
}

// Constant array: every index maps to `value`.
// [[Rcpp::export(rng = false)]]
SEXP cpp_const_array(SEXP domain_, SEXP value) {
  const Ast& d = get_sort(domain_);
  Ast v = lift(d.ctx(), value, nullptr);
  return wrap_expr(own_ast(d.ctx(), Z3_mk_const_array(d.c(), as_sort(d), v)));
}

// ---------------------------------------------------------------------------
// Inspection

// [[Rcpp::export(rng = false)]]
Rcpp::String cpp_expr_to_string(SEXP x) { return to_string(get_expr(x)); }

// [[Rcpp::export(rng = false)]]
SEXP cpp_simplify(SEXP x) {
  const Ast& a = get_expr(x);
  return wrap_expr(own_ast(a.ctx(), Z3_simplify(a.c(), a)));
}

// [[Rcpp::export(rng = false)]]
bool cpp_expr_equal(SEXP x, SEXP y) {
  const Ast& a = get_expr(x);
  const Ast& b = get_expr(y);
  if (a.ctx().get() != b.ctx().get()) return false;
  return Z3_is_eq_ast(a.c(), a, b);
}

// [[Rcpp::export(rng = false)]]
bool cpp_is_const(SEXP x) {
  const Ast& a = get_expr(x);
  Z3_context c = a.c();
  if (Z3_get_ast_kind(c, a) != Z3_APP_AST) return false;
  // A constant applies an uninterpreted declaration of no arguments.
  Z3_func_decl d = Z3_get_app_decl(c, Z3_to_app(c, a));
  return Z3_get_domain_size(c, d) == 0 && Z3_get_decl_kind(c, d) == Z3_OP_UNINTERPRETED;
}

// An integer numeral within R's integer range, or NA. INT_MIN is R's
// NA_integer_, so it is out of range.
// [[Rcpp::export(rng = false)]]
int cpp_numeral_int(SEXP x) {
  const Ast& a = get_expr(x);
  int v = 0;
  bool ok = Z3_is_numeral_ast(a.c(), a) && Z3_get_numeral_int(a.c(), a, &v);
  check(a.ctx());
  return ok && v != NA_INTEGER ? v : NA_INTEGER;
}

static void need_string_value(const Ast& a) {
  bool lit = Z3_is_string(a.c(), a);
  check(a.ctx());
  if (!lit) fail("x must be a string value; evaluate a symbolic string in a model with z3_eval() first");
}

// A string value in SMT-LIB form: characters outside printable ASCII are
// written as \u{...} escapes.
// [[Rcpp::export(rng = false)]]
std::string cpp_string_escaped(SEXP x) {
  const Ast& a = get_expr(x);
  need_string_value(a);
  std::string out = str(Z3_get_string(a.c(), a));
  check(a.ctx());
  return out;
}

// The characters of a string value as bytes. Z3 writes characters 1 to 255
// as single bytes and the others, and a backslash before 'u', as \u{hex}
// escapes, so every "\u{" starts an escape (Z3 4.8.13 writes U+0000 as
// "\u{}").
// [[Rcpp::export(rng = false)]]
Rcpp::RawVector cpp_string_bytes(SEXP x) {
  const Ast& a = get_expr(x);
  need_string_value(a);
  unsigned len = 0;
  Z3_char_ptr p = Z3_get_lstring(a.c(), a, &len);
  check(a.ctx());
  // Copy at once: Z3 reuses the buffer in the next call.
  std::string buf = len && p ? std::string(p, len) : std::string();
  std::vector<unsigned char> out;
  out.reserve(buf.size());
  for (size_t i = 0; i < buf.size(); ++i) {
    if (buf.compare(i, 3, "\\u{") == 0) {
      size_t close = buf.find('}', i + 3);
      if (close == std::string::npos) fail("Z3 returned a malformed string escape");
      unsigned long cp = std::strtoul(buf.substr(i + 3, close - i - 3).c_str(), nullptr, 16);
      if (cp > 255) {
        char hex[16];
        std::snprintf(hex, sizeof(hex), "U+%04lX", cp);
        fail(std::string("the string contains the character ") + hex +
             ", which does not fit in a byte; use z3_value() or z3_string_escaped()");
      }
      out.push_back(static_cast<unsigned char>(cp));
      i = close;
    } else {
      out.push_back(static_cast<unsigned char>(buf[i]));
    }
  }
  return Rcpp::RawVector(out.begin(), out.end());
}

// Description of a value expression for conversion to R. Returns a list
// with `kind` and exact representations; R code decides the R type.
// A double near a rational or algebraic numeral, rounded from enough of its
// decimal digits (25 significant ones). Z3_get_numeral_double divides two
// separately rounded doubles, which loses accuracy and overflows to NaN for
// large numerators and denominators.
static double approx_double(const CtxPtr& ctx, Z3_ast a) {
  Z3_context c = ctx->c;
  for (unsigned prec = 40;; prec *= 2) {
    std::string d = str(Z3_get_numeral_decimal_string(c, a, prec));
    check(ctx);
    bool inexact = !d.empty() && d.back() == '?';
    if (inexact) d.pop_back();
    size_t first = d.find_first_of("123456789");
    size_t sig = 0;
    if (first != std::string::npos) {
      sig = d.size() - first;
      size_t point = d.find('.');
      if (point != std::string::npos && point > first) --sig;
    }
    if (!inexact || sig >= 25 || prec >= 1280) return std::strtod(d.c_str(), nullptr);
  }
}

// Code points as an R string (UTF-8). R strings cannot hold U+0000, so a
// value containing it gets its own kind and stays a Z3 expression.
static SEXP code_points_value(const std::vector<unsigned>& cps, const char* kind) {
  using Rcpp::Named;
  std::string out;
  for (unsigned cp : cps) {
    if (cp == 0) return Rcpp::List::create(Named("kind") = "nul_string");
    if (cp < 0x80) out += static_cast<char>(cp);
    else if (cp < 0x800) { out += static_cast<char>(0xC0 | (cp >> 6)); out += static_cast<char>(0x80 | (cp & 0x3F)); }
    else if (cp < 0x10000) { out += static_cast<char>(0xE0 | (cp >> 12)); out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F)); out += static_cast<char>(0x80 | (cp & 0x3F)); }
    else { out += static_cast<char>(0xF0 | (cp >> 18)); out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F)); out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F)); out += static_cast<char>(0x80 | (cp & 0x3F)); }
  }
  Rcpp::CharacterVector s(1);
  s[0] = Rcpp::String(out, CE_UTF8);
  return Rcpp::List::create(Named("kind") = kind, Named("value") = s);
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_value_info(SEXP x, int digits) {
  const Ast& a = get_expr(x);
  const CtxPtr& ctx = a.ctx();
  Z3_context c = a.c();
  using Rcpp::Named;
  Z3_sort_kind k = sort_kind(a);

  if (k == Z3_BOOL_SORT) {
    Z3_lbool v = Z3_get_bool_value(c, a);
    check(ctx);
    if (v == Z3_L_UNDEF) return Rcpp::List::create(Named("kind") = "symbolic");
    return Rcpp::List::create(Named("kind") = "bool", Named("value") = (v == Z3_L_TRUE));
  }
  if (k == Z3_INT_SORT || k == Z3_REAL_SORT || k == Z3_BV_SORT) {
    if (Z3_is_algebraic_number(c, a)) {
      std::string dec = str(Z3_get_numeral_decimal_string(c, a, static_cast<unsigned>(digits)));
      check(ctx);
      double approx = approx_double(ctx, a);
      return Rcpp::List::create(Named("kind") = "algebraic", Named("decimal") = dec,
                                Named("approx") = approx, Named("text") = to_string(a));
    }
    if (!Z3_is_numeral_ast(c, a)) return Rcpp::List::create(Named("kind") = "symbolic");
    std::string exact;
    bool exact_double;
    double value, approx;
    // Fast path: a numeral whose numerator and denominator fit machine
    // integers is read without decimal strings. Bit-vector numerals are
    // unsigned.
    int64_t n = 0, d = 1;
    unsigned u = 0;
    bool small = k == Z3_BV_SORT && Z3_get_numeral_uint(c, a, &u);
    if (small) n = u;
    else small = Z3_get_numeral_rational_int64(c, a, &n, &d);
    check(ctx);
    if (small) {
      exact = std::to_string(static_cast<long long>(n));
      if (d != 1) exact += "/" + std::to_string(static_cast<long long>(d));
      // Exact double when |num| <= 2^53 and the denominator is a power of two.
      exact_double = n >= -(INT64_C(1) << 53) && n <= (INT64_C(1) << 53) && (d & (d - 1)) == 0;
      int dexp = 0;
      while (exact_double && (INT64_C(1) << dexp) < d) ++dexp;
      value = exact_double ? std::ldexp(static_cast<double>(n), -dexp) : NA_REAL;
      approx = exact_double ? value : d == 1 ? std::strtod(exact.c_str(), nullptr) : approx_double(ctx, a);
    } else {
      exact = str(Z3_get_numeral_string(c, a));
      check(ctx);
      // Exact double when |num| <= 2^53 and the denominator is a power of two.
      Ast num = k == Z3_REAL_SORT ? own_ast(ctx, Z3_get_numerator(c, a)) : a;
      Ast den = k == Z3_REAL_SORT ? own_ast(ctx, Z3_get_denominator(c, a)) : Ast();
      int64_t n64 = 0;
      bool fits = Z3_get_numeral_int64(c, num, &n64);
      if (!fits && k == Z3_BV_SORT) {
        uint64_t u64 = 0;
        fits = Z3_get_numeral_uint64(c, num, &u64) && u64 <= (1ULL << 53);
        n64 = static_cast<int64_t>(u64);
      }
      check(ctx);
      // The denominator must be a power of two, 2^dexp; dexp up to 1023 is
      // checked through the exact decimal digits of the power.
      int dexp = 0;
      if (fits && den.get()) {
        double dv = Z3_get_numeral_double(c, den);
        std::string ds = str(Z3_get_numeral_string(c, den));
        check(ctx);
        int e = 0;
        fits = std::isfinite(dv) && std::frexp(dv, &e) == 0.5;
        if (fits) {
          char buf[400];
          std::snprintf(buf, sizeof(buf), "%.0f", dv);
          fits = ds == buf;
          dexp = e - 1;
        }
      }
      exact_double = fits && n64 <= (1LL << 53) && n64 >= -(1LL << 53);
      value = exact_double ? std::ldexp(static_cast<double>(n64), -dexp) : NA_REAL;
      approx = exact_double ? value : k == Z3_BV_SORT ? std::strtod(exact.c_str(), nullptr) : approx_double(ctx, a);
    }
    std::string kind = k == Z3_INT_SORT ? "int" : k == Z3_REAL_SORT ? "real" : "bv";
    return Rcpp::List::create(Named("kind") = kind, Named("exact") = exact,
                              Named("exact_double") = exact_double, Named("value") = value,
                              Named("approx") = approx);
  }
  if (k == Z3_FLOATING_POINT_SORT) {
    if (!Z3_is_numeral_ast(c, a)) return Rcpp::List::create(Named("kind") = "symbolic");
    double v = 0;
    bool exact = fp_to_double(a, &v);
    return Rcpp::List::create(Named("kind") = "fp", Named("value") = v, Named("exact_double") = exact,
                              Named("text") = to_string(a));
  }
  if (k == Z3_ROUNDING_MODE_SORT) {
    if (!Z3_is_app(c, a) || Z3_get_app_num_args(c, Z3_to_app(c, a)) != 0 ||
        Z3_get_decl_kind(c, Z3_get_app_decl(c, Z3_to_app(c, a))) == Z3_OP_UNINTERPRETED) {
      return Rcpp::List::create(Named("kind") = "symbolic");
    }
    return Rcpp::List::create(Named("kind") = "rm", Named("value") = to_string(a));
  }
  if (k == Z3_SEQ_SORT && is_string(a)) {
    if (!Z3_is_string(c, a)) return Rcpp::List::create(Named("kind") = "symbolic");
    unsigned len = Z3_get_string_length(c, a);
    std::vector<unsigned> cps(len);
    if (len) Z3_get_string_contents(c, a, len, cps.data());
    check(ctx);
    return code_points_value(cps, "string");
  }
  if (k == Z3_CHAR_SORT) {
    // A character literal is an application of the char decl with the code
    // point as its integer parameter.
    if (!Z3_is_app(c, a) || Z3_get_app_num_args(c, Z3_to_app(c, a)) != 0) {
      return Rcpp::List::create(Named("kind") = "symbolic");
    }
    Z3_func_decl d = Z3_get_app_decl(c, Z3_to_app(c, a));
    if (Z3_get_decl_kind(c, d) == Z3_OP_UNINTERPRETED || Z3_get_decl_num_parameters(c, d) != 1 ||
        Z3_get_decl_parameter_kind(c, d, 0) != Z3_PARAMETER_INT) {
      check(ctx);
      return Rcpp::List::create(Named("kind") = "symbolic");
    }
    int cp = Z3_get_decl_int_parameter(c, d, 0);
    check(ctx);
    return code_points_value(std::vector<unsigned>(1, static_cast<unsigned>(cp)), "char");
  }
#if Z3R_AT_LEAST(5, 3, 0)
  if (k == Z3_FINITE_FIELD_SORT) {
    if (!Z3_is_numeral_ast(c, a)) return Rcpp::List::create(Named("kind") = "symbolic");
    std::string exact = str(Z3_get_numeral_string(c, a));
    check(ctx);
    double v = std::strtod(exact.c_str(), nullptr);
    bool exact_double = exact.size() < 16 || (exact.size() == 16 && exact <= "9007199254740992");
    return Rcpp::List::create(Named("kind") = "ff", Named("exact") = exact, Named("exact_double") = exact_double,
                              Named("value") = exact_double ? v : NA_REAL, Named("approx") = v);
  }
#endif
#if Z3R_AT_LEAST(5, 3, 2)
  if (k == Z3_COMPLEX_SORT) {
    // A value is (complex.make re im) with numeral parts.
    Ast re = own_ast(ctx, Z3_simplify(c, own_ast(ctx, Z3_mk_real_part(c, a))));
    Ast im = own_ast(ctx, Z3_simplify(c, own_ast(ctx, Z3_mk_imag_part(c, a))));
    if (!Z3_is_numeral_ast(c, re) || !Z3_is_numeral_ast(c, im)) {
      check(ctx);
      return Rcpp::List::create(Named("kind") = "symbolic");
    }
    return Rcpp::List::create(Named("kind") = "complex", Named("re") = wrap_expr(re), Named("im") = wrap_expr(im));
  }
#endif
  return Rcpp::List::create(Named("kind") = "other");
}

// [[Rcpp::export(rng = false)]]
std::string cpp_decimal_string(SEXP x, int digits) {
  const Ast& a = get_expr(x);
  std::string out = str(Z3_get_numeral_decimal_string(a.c(), a, static_cast<unsigned>(digits)));
  check(a.ctx());
  return out;
}
