// Remaining C API functionality: implied equalities, term updates, parser
// contexts, numeral forms, memory and logging.
#include "z3r.h"

using namespace z3r;

// Classes of terms that are equal in every model of the solver's assertions.
// [[Rcpp::export(rng = false)]]
SEXP cpp_implied_equalities(SEXP s_, Rcpp::List terms) {
  SolverHolder* h = get_xptr<SolverHolder>(s_, Kind::Solver);
  const CtxPtr& ctx = h->s.ctx();
  AstVec t = expr_list(ctx, terms, "terms");
  std::vector<Z3_ast> r = raw(t);
  std::vector<unsigned> ids(r.size(), 0);
  Z3_lbool st = Z3_get_implied_equalities(ctx->c, h->s, static_cast<unsigned>(r.size()),
                                          r.empty() ? nullptr : r.data(), ids.empty() ? nullptr : ids.data());
  check(ctx);
  Rcpp::IntegerVector cls(ids.size());
  for (size_t i = 0; i < ids.size(); ++i) cls[i] = static_cast<int>(ids[i]);
  return Rcpp::List::create(Rcpp::Named("status") = lbool_string(st), Rcpp::Named("class") = cls);
}

// Same application with new arguments.
// [[Rcpp::export(rng = false)]]
SEXP cpp_update_term(SEXP x, Rcpp::List args) {
  const Ast& a = get_expr(x);
  const CtxPtr& ctx = a.ctx();
  Z3_context c = ctx->c;
  if (!Z3_is_app(c, a)) fail("only applications can be updated");
  unsigned n = Z3_get_app_num_args(c, Z3_to_app(c, a));
  if (static_cast<unsigned>(args.size()) != n) {
    fail("the term has " + std::to_string(n) + " argument(s), got " +
         std::to_string(static_cast<long long>(args.size())));
  }
  AstVec v;
  for (unsigned i = 0; i < n; ++i) {
    Z3_ast old = Z3_get_app_arg(c, Z3_to_app(c, a), i);
    Ast na = lift(ctx, args[i], Z3_get_sort(c, old));
    if (!Z3_is_eq_sort(c, Z3_get_sort(c, na), Z3_get_sort(c, old))) {
      fail("argument " + std::to_string(i + 1) + " has the wrong sort");
    }
    v.push_back(na);
  }
  std::vector<Z3_ast> r = raw(v);
  return wrap_expr(own_ast(ctx, Z3_update_term(c, a, n, r.empty() ? nullptr : r.data())));
}

// [[Rcpp::export(rng = false)]]
void cpp_solver_from_file(SEXP s_, SEXP file_) {
  SolverHolder* h = get_xptr<SolverHolder>(s_, Kind::Solver);
  std::string file = as_string(file_, "file");
  Z3_solver_from_file(h->s.c(), h->s, file.c_str());
  check(h->s.ctx());
  h->touch();
}

// [[Rcpp::export(rng = false)]]
void cpp_solver_import_model_converter(SEXP src_, SEXP dst_) {
  SolverHolder* src = get_xptr<SolverHolder>(src_, Kind::Solver);
  SolverHolder* dst = get_xptr<SolverHolder>(dst_, Kind::Solver);
  same_ctx(src->s.ctx(), dst->s.ctx());
  ensure_solver_ready(src->s.c(), src->s);
  ensure_solver_ready(dst->s.c(), dst->s);
  check(src->s.ctx());
  Z3_solver_import_model_converter(src->s.c(), src->s, dst->s);
  check(src->s.ctx());
}

// Updates a context configuration parameter after creation.
// [[Rcpp::export(rng = false)]]
void cpp_update_param(SEXP ctx_, std::string name, std::string value) {
  CtxPtr ctx = get_ctx(ctx_);
  Z3_update_param_value(ctx->c, name.c_str(), value.c_str());
  check(ctx);
}

// Rational bounds of an algebraic number within 1/10^precision.
// [[Rcpp::export(rng = false)]]
SEXP cpp_algebraic_bounds(SEXP x, int precision) {
  const Ast& a = get_expr(x);
  const CtxPtr& ctx = a.ctx();
  if (!Z3_is_algebraic_number(a.c(), a)) fail("expected an irrational algebraic number");
  if (precision < 0) fail("precision must be non-negative");
  Ast lo = own_ast(ctx, Z3_get_algebraic_number_lower(a.c(), a, static_cast<unsigned>(precision)));
  Ast hi = own_ast(ctx, Z3_get_algebraic_number_upper(a.c(), a, static_cast<unsigned>(precision)));
  return Rcpp::List::create(Rcpp::Named("lower") = wrap_expr(lo), Rcpp::Named("upper") = wrap_expr(hi));
}

// Binary digits of an integer numeral.
// [[Rcpp::export(rng = false)]]
std::string cpp_binary_string(SEXP x) {
  const Ast& a = get_expr(x);
  std::string out = str(Z3_get_numeral_binary_string(a.c(), a));
  check(a.ctx());
  return out;
}

// Bit-vector from bits, least significant first.
// [[Rcpp::export(rng = false)]]
SEXP cpp_bv_from_bits(SEXP ctx_, Rcpp::LogicalVector bits) {
  CtxPtr ctx = get_ctx(ctx_);
  if (bits.size() == 0) fail("at least one bit is needed");
  std::unique_ptr<bool[]> b(new bool[bits.size()]);
  for (R_xlen_t i = 0; i < bits.size(); ++i) {
    if (bits[i] == NA_LOGICAL) fail("bits must not be NA");
    b[i] = bits[i] != 0;
  }
  return wrap_expr(own_ast(ctx, Z3_mk_bv_numeral(ctx->c, static_cast<unsigned>(bits.size()), b.get())));
}

// Floating point numeral from sign, unbiased exponent and significand (the
// fraction bits, without the hidden bit), exactly.
// [[Rcpp::export(rng = false)]]
SEXP cpp_fp_from_parts(SEXP sort_, bool negative, std::string exponent, std::string significand) {
  const Ast& s = get_sort(sort_);
  char* end = nullptr;
  long long e = std::strtoll(exponent.c_str(), &end, 10);
  if (exponent.empty() || *end) fail("exponent must be a whole number");
  unsigned long long m = std::strtoull(significand.c_str(), &end, 10);
  if (significand.empty() || *end || significand[0] == '-') fail("significand must be a non-negative whole number");
  return wrap_expr(own_ast(s.ctx(), Z3_mk_fpa_numeral_int64_uint64(s.c(), negative, e, m, as_sort(s))));
}

// [[Rcpp::export(rng = false)]]
double cpp_memory_usage() { return static_cast<double>(Z3_get_estimated_alloc_size()); }

// [[Rcpp::export(rng = false)]]
void cpp_append_log(std::string text) { Z3_append_log(text.c_str()); }

// [[Rcpp::export(rng = false)]]
SEXP cpp_decl_identity(SEXP x, SEXP y) {
  const Ast& a = is_kind(x, Kind::Sort) ? get_sort(x) : get_decl(x);
  Z3_context c = a.c();
  unsigned id = is_kind(x, Kind::Sort) ? Z3_get_sort_id(c, as_sort(a)) : Z3_get_func_decl_id(c, as_decl(a));
  check(a.ctx());
  if (Rf_isNull(y)) return Rcpp::wrap(static_cast<double>(id));
  const Ast& b = is_kind(y, Kind::Sort) ? get_sort(y) : get_decl(y);
  if (a.ctx().get() != b.ctx().get()) return Rcpp::wrap(false);
  bool eq = is_kind(x, Kind::Sort) ? Z3_is_eq_sort(c, as_sort(a), as_sort(b)) : Z3_is_eq_func_decl(c, as_decl(a), as_decl(b));
  return Rcpp::wrap(eq);
}

// ---------------------------------------------------------------------------
// Parser contexts (Z3 4.9.0): parse several SMT-LIB2 fragments that share
// declared sorts and functions.

// [[Rcpp::export(rng = false)]]
SEXP cpp_mk_parser(SEXP ctx_) {
#if Z3R_AT_LEAST(4, 9, 0)
  CtxPtr ctx = get_ctx(ctx_);
  return make_xptr(new Ref<Z3_parser_context>(own(ctx, Z3_mk_parser_context(ctx->c))), Kind::Parser);
#else
  (void)ctx_;
  needs_version("z3_parser()", "4.9.0");
#endif
}

// [[Rcpp::export(rng = false)]]
void cpp_parser_add(SEXP p_, SEXP item) {
#if Z3R_AT_LEAST(4, 9, 0)
  const Ref<Z3_parser_context>& p = *get_xptr<Ref<Z3_parser_context>>(p_, Kind::Parser);
  if (is_kind(item, Kind::Sort)) {
    const Ast& s = get_sort(item);
    same_ctx(p.ctx(), s.ctx());
    Z3_parser_context_add_sort(p.c(), p, as_sort(s));
  } else {
    const Ast& d = get_decl(item);
    same_ctx(p.ctx(), d.ctx());
    Z3_parser_context_add_decl(p.c(), p, as_decl(d));
  }
  check(p.ctx());
#else
  (void)p_; (void)item;
  needs_version("z3_parser()", "4.9.0");
#endif
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_parser_parse(SEXP p_, SEXP text_) {
#if Z3R_AT_LEAST(4, 9, 0)
  const Ref<Z3_parser_context>& p = *get_xptr<Ref<Z3_parser_context>>(p_, Kind::Parser);
  std::string text = as_string(text_, "text");
  return wrap_ast_vector(p.ctx(), Z3_parser_context_from_string(p.c(), p, text.c_str()));
#else
  (void)p_; (void)text_;
  needs_version("z3_parser()", "4.9.0");
#endif
}
