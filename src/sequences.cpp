// Sequences, regular expressions and characters.
#include "z3r.h"

using namespace z3r;

namespace {

Z3_sort sort_of_ast(const Ast& a) { return Z3_get_sort(a.c(), a); }

bool is_seq(const Ast& a) {
  return Z3_get_sort_kind(a.c(), sort_of_ast(a)) == Z3_SEQ_SORT;
}
bool is_re(const Ast& a) { return Z3_get_sort_kind(a.c(), sort_of_ast(a)) == Z3_RE_SORT; }
bool is_char(const Ast& a) {
  return Z3_get_sort_kind(a.c(), sort_of_ast(a)) == Z3_CHAR_SORT;
}

void need(bool ok, const std::string& op, const char* what, const Ast& a) {
  if (!ok) {
    fail("'" + op + "' needs " + what + ", got " + sort_name(a.ctx(), sort_of_ast(a)));
  }
}

// Lifts argument i; R character values become string literals.
Ast arg(const CtxPtr& ctx, SEXP args, R_xlen_t i, Z3_sort hint = nullptr) {
  return lift(ctx, VECTOR_ELT(args, i), hint);
}

// A regular expression argument: strings are converted with str.to_re.
Ast re_arg(const CtxPtr& ctx, SEXP args, R_xlen_t i, const std::string& op) {
  Ast a = arg(ctx, args, i);
  if (is_seq(a)) return own_ast(ctx, Z3_mk_seq_to_re(ctx->c, a));
  need(is_re(a), op, "a regular expression or a sequence", a);
  return a;
}

typedef Z3_ast (*NaryFn)(Z3_context, unsigned, const Z3_ast[]);

}  // namespace

// [[Rcpp::export(rng = false)]]
SEXP cpp_seq_sort(SEXP elem_) {
  const Ast& e = get_sort(elem_);
  return wrap_sort(own_ast(e.ctx(), sort_ast(Z3_mk_seq_sort(e.c(), as_sort(e)))));
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_re_sort(SEXP seq_) {
  const Ast& s = get_sort(seq_);
  if (Z3_get_sort_kind(s.c(), as_sort(s)) != Z3_SEQ_SORT) fail("regular expressions are over sequence sorts");
  return wrap_sort(own_ast(s.ctx(), sort_ast(Z3_mk_re_sort(s.c(), as_sort(s)))));
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_char_sort(SEXP ctx_) {
  CtxPtr ctx = get_ctx(ctx_);
  return wrap_sort(own_ast(ctx, sort_ast(Z3_mk_char_sort(ctx->c))));
}

// Element sort of a sequence sort or regular expression sort.
// [[Rcpp::export(rng = false)]]
SEXP cpp_sort_basis(SEXP sort_) {
  const Ast& s = get_sort(sort_);
  Z3_context c = s.c();
  Z3_sort_kind k = Z3_get_sort_kind(c, as_sort(s));
  Z3_sort b;
  if (k == Z3_SEQ_SORT) b = Z3_get_seq_sort_basis(c, as_sort(s));
  else if (k == Z3_RE_SORT) b = Z3_get_re_sort_basis(c, as_sort(s));
  else fail("not a sequence or regular expression sort");
  return wrap_sort(own_ast(s.ctx(), sort_ast(b)));
}

// Constants of sequence and regular expression sorts.
// [[Rcpp::export(rng = false)]]
SEXP cpp_seq_constant(std::string what, SEXP sort_) {
  const Ast& s = get_sort(sort_);
  Z3_context c = s.c();
  Z3_sort srt = as_sort(s);
  Z3_sort_kind k = Z3_get_sort_kind(c, srt);
  if (what == "empty") {
    if (k != Z3_SEQ_SORT) fail("z3_seq_empty() needs a sequence sort");
    return wrap_expr(own_ast(s.ctx(), Z3_mk_seq_empty(c, srt)));
  }
  if (k != Z3_RE_SORT) fail("this regular expression constant needs a regular expression sort");
  if (what == "re_empty") return wrap_expr(own_ast(s.ctx(), Z3_mk_re_empty(c, srt)));
  if (what == "re_full") return wrap_expr(own_ast(s.ctx(), Z3_mk_re_full(c, srt)));
  return wrap_expr(own_ast(s.ctx(), Z3_mk_re_allchar(c, srt)));
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_char_val(SEXP ctx_, double code) {
#if Z3R_AT_LEAST(4, 8, 15)
  CtxPtr ctx = get_ctx(ctx_);
  if (ISNAN(code) || code < 0 || code > 0x2FFFF || code != std::floor(code)) {
    fail("a character code must be a whole number between 0 and 196607");
  }
  return wrap_expr(own_ast(ctx, Z3_mk_char(ctx->c, static_cast<unsigned>(code))));
#else
  (void)ctx_; (void)code;
  needs_version("z3_char_val()", "4.8.15");
#endif
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_seq_op(std::string op, SEXP args, Rcpp::IntegerVector params) {
  CtxPtr ctx = ctx_from_list(args);
  Z3_context c = ctx->c;
  R_xlen_t n = Rf_xlength(args);
  auto nargs = [&](R_xlen_t k) {
    if (n != k) fail("'" + op + "' takes " + std::to_string(static_cast<long long>(k)) + " arguments");
  };
  Z3_sort I = Z3_mk_int_sort(c);

  // Sequences
  if (op == "unit") {
    nargs(1);
    return wrap_expr(own_ast(ctx, Z3_mk_seq_unit(c, arg(ctx, args, 0))));
  }
  if (op == "nth") {
    nargs(2);
    Ast s = arg(ctx, args, 0);
    need(is_seq(s), op, "a sequence", s);
    Ast i = arg(ctx, args, 1, I);
    return wrap_expr(own_ast(ctx, Z3_mk_seq_nth(c, s, i)));
  }
  if (op == "last_indexof") {
    nargs(2);
    Ast s = arg(ctx, args, 0);
    need(is_seq(s), op, "a sequence", s);
    Ast t = arg(ctx, args, 1, sort_of_ast(s));
    return wrap_expr(own_ast(ctx, Z3_mk_seq_last_index(c, s, t)));
  }
  if (op == "replace_all" || op == "replace_re" || op == "replace_re_all") {
#if Z3R_AT_LEAST(4, 15, 5)
    nargs(3);
    Ast s = arg(ctx, args, 0);
    need(is_seq(s), op, "a sequence", s);
    Ast dst = arg(ctx, args, 2, sort_of_ast(s));
    if (op == "replace_all") {
      Ast src = arg(ctx, args, 1, sort_of_ast(s));
      return wrap_expr(own_ast(ctx, Z3_mk_seq_replace_all(c, s, src, dst)));
    }
    Ast re = re_arg(ctx, args, 1, op);
    return wrap_expr(own_ast(ctx, op == "replace_re" ? Z3_mk_seq_replace_re(c, s, re, dst)
                                                     : Z3_mk_seq_replace_re_all(c, s, re, dst)));
#else
    needs_version(("z3_" + op + "()").c_str(), "4.15.5");
#endif
  }
  if (op == "map" || op == "mapi" || op == "foldl" || op == "foldli") {
#if Z3R_AT_LEAST(4, 13, 2)
    if (op == "map") { nargs(2); return wrap_expr(own_ast(ctx, Z3_mk_seq_map(c, arg(ctx, args, 0), arg(ctx, args, 1)))); }
    if (op == "mapi") {
      nargs(3);
      return wrap_expr(own_ast(ctx, Z3_mk_seq_mapi(c, arg(ctx, args, 0), arg(ctx, args, 1, I), arg(ctx, args, 2))));
    }
    if (op == "foldl") {
      nargs(3);
      return wrap_expr(own_ast(ctx, Z3_mk_seq_foldl(c, arg(ctx, args, 0), arg(ctx, args, 1), arg(ctx, args, 2))));
    }
    nargs(4);
    return wrap_expr(own_ast(ctx, Z3_mk_seq_foldli(c, arg(ctx, args, 0), arg(ctx, args, 1, I), arg(ctx, args, 2),
                                                   arg(ctx, args, 3))));
#else
    needs_version(("z3_seq_" + op + "()").c_str(), "4.13.2");
#endif
  }
  if (op == "seq_power") {
#if Z3R_AT_LEAST(5, 3, 0)
    nargs(2);
    Ast s = arg(ctx, args, 0);
    need(is_seq(s), op, "a sequence", s);
    return wrap_expr(own_ast(ctx, Z3_mk_seq_power(c, s, arg(ctx, args, 1, I))));
#else
    needs_version("z3_seq_power()", "5.3.0");
#endif
  }
  if (op == "in_re") {
    nargs(2);
    Ast s = arg(ctx, args, 0);
    need(is_seq(s), op, "a sequence", s);
    return wrap_expr(own_ast(ctx, Z3_mk_seq_in_re(c, s, re_arg(ctx, args, 1, op))));
  }

  // Regular expressions
  if (op == "to_re") {
    nargs(1);
    Ast s = arg(ctx, args, 0);
    need(is_seq(s), op, "a sequence", s);
    return wrap_expr(own_ast(ctx, Z3_mk_seq_to_re(c, s)));
  }
  typedef Z3_ast (*UnFn)(Z3_context, Z3_ast);
  struct ReUn { const char* name; UnFn f; };
  static const ReUn re_un[] = {{"star", Z3_mk_re_star}, {"plus", Z3_mk_re_plus},
                               {"option", Z3_mk_re_option}, {"complement", Z3_mk_re_complement}};
  for (const ReUn& r : re_un) {
    if (op == r.name) {
      nargs(1);
      return wrap_expr(own_ast(ctx, r.f(c, re_arg(ctx, args, 0, op))));
    }
  }
  if (op == "re_union" || op == "re_concat" || op == "re_intersect") {
    if (n < 1) fail("'" + op + "' needs at least one argument");
    AstVec v;
    for (R_xlen_t i = 0; i < n; ++i) v.push_back(re_arg(ctx, args, i, op));
    std::vector<Z3_ast> r = raw(v);
    NaryFn f = op == "re_union" ? Z3_mk_re_union : op == "re_concat" ? Z3_mk_re_concat : Z3_mk_re_intersect;
    return wrap_expr(own_ast(ctx, f(c, static_cast<unsigned>(r.size()), r.data())));
  }
  if (op == "re_range") {
    nargs(2);
    Ast lo = arg(ctx, args, 0), hi = arg(ctx, args, 1);
    need(is_seq(lo), op, "single-character strings", lo);
    need(is_seq(hi), op, "single-character strings", hi);
    return wrap_expr(own_ast(ctx, Z3_mk_re_range(c, lo, hi)));
  }
  if (op == "re_loop") {
    nargs(1);
    if (params.size() != 2 || params[0] < 0 || params[1] < 0) fail("loop bounds must be non-negative");
    return wrap_expr(own_ast(ctx, Z3_mk_re_loop(c, re_arg(ctx, args, 0, op), static_cast<unsigned>(params[0]),
                                                static_cast<unsigned>(params[1]))));
  }
  if (op == "re_power") {
#if Z3R_AT_LEAST(4, 8, 15)
    nargs(1);
    if (params.size() != 1 || params[0] < 0) fail("the power must be non-negative");
    return wrap_expr(own_ast(ctx, Z3_mk_re_power(c, re_arg(ctx, args, 0, op), static_cast<unsigned>(params[0]))));
#else
    needs_version("z3_re_power()", "4.8.15");
#endif
  }
  if (op == "re_diff") {
#if Z3R_AT_LEAST(4, 8, 14)
    nargs(2);
    return wrap_expr(own_ast(ctx, Z3_mk_re_diff(c, re_arg(ctx, args, 0, op), re_arg(ctx, args, 1, op))));
#else
    needs_version("z3_re_diff()", "4.8.14");
#endif
  }

  // Characters and code points
  if (op == "char_to_int" || op == "char_to_bv" || op == "char_is_digit") {
    nargs(1);
    Ast ch = arg(ctx, args, 0);
    need(is_char(ch), op, "a character", ch);
    UnFn f = op == "char_to_int" ? Z3_mk_char_to_int : op == "char_to_bv" ? Z3_mk_char_to_bv : Z3_mk_char_is_digit;
    return wrap_expr(own_ast(ctx, f(c, ch)));
  }
  if (op == "char_from_bv") {
    nargs(1);
    return wrap_expr(own_ast(ctx, Z3_mk_char_from_bv(c, arg(ctx, args, 0))));
  }
  if (op == "char_le") {
    nargs(2);
    Ast a = arg(ctx, args, 0), b = arg(ctx, args, 1);
    need(is_char(a), op, "characters", a);
    need(is_char(b), op, "characters", b);
    return wrap_expr(own_ast(ctx, Z3_mk_char_le(c, a, b)));
  }
  if (op == "str_from_code" || op == "str_to_code") {
#if Z3R_AT_LEAST(4, 8, 15)
    nargs(1);
    Ast a = arg(ctx, args, 0, op == "str_from_code" ? I : nullptr);
    return wrap_expr(own_ast(ctx, op == "str_from_code" ? Z3_mk_string_from_code(c, a)
                                                        : Z3_mk_string_to_code(c, a)));
#else
    needs_version(("z3_" + op + "()").c_str(), "4.8.15");
#endif
  }
  if (op == "ubv_to_str" || op == "sbv_to_str") {
    nargs(1);
    Ast a = arg(ctx, args, 0);
    return wrap_expr(own_ast(ctx, op == "ubv_to_str" ? Z3_mk_ubv_to_str(c, a) : Z3_mk_sbv_to_str(c, a)));
  }
  fail("unknown sequence operation '" + op + "'");
}
