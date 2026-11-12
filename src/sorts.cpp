#include "z3r.h"

using namespace z3r;

static SEXP sort_result(const CtxPtr& ctx, Z3_sort s) {
  return wrap_sort(own_ast(ctx, sort_ast(s)));
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_int_sort(SEXP ctx_) {
  CtxPtr ctx = get_ctx(ctx_);
  return sort_result(ctx, Z3_mk_int_sort(ctx->c));
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_real_sort(SEXP ctx_) {
  CtxPtr ctx = get_ctx(ctx_);
  return sort_result(ctx, Z3_mk_real_sort(ctx->c));
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_bool_sort(SEXP ctx_) {
  CtxPtr ctx = get_ctx(ctx_);
  return sort_result(ctx, Z3_mk_bool_sort(ctx->c));
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_bv_sort(SEXP ctx_, int width) {
  CtxPtr ctx = get_ctx(ctx_);
  if (width < 1) fail("bit-vector width must be a positive integer");
  return sort_result(ctx, Z3_mk_bv_sort(ctx->c, static_cast<unsigned>(width)));
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_string_sort(SEXP ctx_) {
  CtxPtr ctx = get_ctx(ctx_);
  return sort_result(ctx, Z3_mk_string_sort(ctx->c));
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_array_sort(SEXP domain_, SEXP range_) {
  const Ast& d = get_sort(domain_);
  const Ast& r = get_sort(range_);
  same_ctx(d.ctx(), r.ctx());
  return sort_result(d.ctx(), Z3_mk_array_sort(d.c(), as_sort(d), as_sort(r)));
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_uninterpreted_sort(SEXP ctx_, SEXP name_) {
  CtxPtr ctx = get_ctx(ctx_);
  std::string name = as_string(name_, "name");
  return sort_result(ctx, Z3_mk_uninterpreted_sort(ctx->c, mk_symbol(ctx, name)));
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_sort_of(SEXP x) {
  return wrap_sort(sort_of(get_expr(x)));
}

static const char* kind_name(Z3_sort_kind k) {
  switch (k) {
    case Z3_UNINTERPRETED_SORT: return "uninterpreted";
    case Z3_BOOL_SORT: return "bool";
    case Z3_INT_SORT: return "int";
    case Z3_REAL_SORT: return "real";
    case Z3_BV_SORT: return "bitvector";
    case Z3_ARRAY_SORT: return "array";
    case Z3_DATATYPE_SORT: return "datatype";
    case Z3_RELATION_SORT: return "relation";
    case Z3_FINITE_DOMAIN_SORT: return "finite_domain";
    case Z3_FLOATING_POINT_SORT: return "floating_point";
    case Z3_ROUNDING_MODE_SORT: return "rounding_mode";
    case Z3_SEQ_SORT: return "sequence";
    case Z3_RE_SORT: return "regex";
    case Z3_CHAR_SORT: return "char";
#if Z3R_AT_LEAST(5, 3, 0)
    case Z3_FINITE_FIELD_SORT: return "finite_field";
#endif
#if Z3R_AT_LEAST(5, 3, 2)
    case Z3_COMPLEX_SORT: return "complex";
#endif
    default: return "unknown";
  }
}

// Sort kind of a sort or an expression, as a lower-case string. Z3 strings
// are sequences of characters; they are reported as "string".
// [[Rcpp::export(rng = false)]]
std::string cpp_sort_kind(SEXP x) {
  Ast s = is_kind(x, Kind::Sort) ? get_sort(x) : sort_of(get_expr(x));
  Z3_sort_kind k = Z3_get_sort_kind(s.c(), as_sort(s));
  check(s.ctx());
  if (k == Z3_SEQ_SORT) {
    bool is_str = Z3_is_string_sort(s.c(), as_sort(s));
    check(s.ctx());
    if (is_str) return "string";
  }
  return kind_name(k);
}

// [[Rcpp::export(rng = false)]]
std::string cpp_sort_to_string(SEXP x) {
  const Ast& s = get_sort(x);
  return sort_name(s.ctx(), as_sort(s));
}

// [[Rcpp::export(rng = false)]]
int cpp_bv_width(SEXP x) {
  Ast s = is_kind(x, Kind::Sort) ? get_sort(x) : sort_of(get_expr(x));
  if (Z3_get_sort_kind(s.c(), as_sort(s)) != Z3_BV_SORT) fail("not a bit-vector");
  unsigned w = Z3_get_bv_sort_size(s.c(), as_sort(s));
  check(s.ctx());
  return static_cast<int>(w);
}

// Domain and range of an array sort.
// [[Rcpp::export(rng = false)]]
SEXP cpp_array_sort_parts(SEXP x) {
  Ast s = is_kind(x, Kind::Sort) ? get_sort(x) : sort_of(get_expr(x));
  if (Z3_get_sort_kind(s.c(), as_sort(s)) != Z3_ARRAY_SORT) fail("not an array");
  Ast d = own_ast(s.ctx(), sort_ast(Z3_get_array_sort_domain(s.c(), as_sort(s))));
  Ast r = own_ast(s.ctx(), sort_ast(Z3_get_array_sort_range(s.c(), as_sort(s))));
  return Rcpp::List::create(Rcpp::Named("domain") = wrap_sort(d),
                            Rcpp::Named("range") = wrap_sort(r));
}

// [[Rcpp::export(rng = false)]]
bool cpp_sort_equal(SEXP a, SEXP b) {
  const Ast& x = get_sort(a);
  const Ast& y = get_sort(b);
  if (x.ctx().get() != y.ctx().get()) return false;
  bool eq = Z3_is_eq_sort(x.c(), as_sort(x), as_sort(y));
  check(x.ctx());
  return eq;
}
