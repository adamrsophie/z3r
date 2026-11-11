// Optimization (Z3's MaxSMT / OptSMT engine).
#include "z3r.h"

using namespace z3r;

static OptimizeHolder* optimizer(SEXP x) { return get_xptr<OptimizeHolder>(x, Kind::Optimize); }

// [[Rcpp::export(rng = false)]]
SEXP cpp_mk_optimize(SEXP ctx_) {
  CtxPtr ctx = get_ctx(ctx_);
  OptimizeHolder* h = new OptimizeHolder();
  std::unique_ptr<OptimizeHolder> guard(h);
  h->o = own(ctx, Z3_mk_optimize(ctx->c));
  guard.release();
  return make_xptr(h, Kind::Optimize);
}

static Ast bool_arg(const CtxPtr& ctx, SEXP x, const char* what) {
  Ast a = lift(ctx, x, Z3_mk_bool_sort(ctx->c));
  if (!is_bool(a)) fail(std::string(what) + " must be Boolean");
  return a;
}

// [[Rcpp::export(rng = false)]]
void cpp_optimize_assert(SEXP o_, Rcpp::List constraints) {
  OptimizeHolder* h = optimizer(o_);
  const CtxPtr& ctx = h->o.ctx();
  AstVec v;
  for (R_xlen_t i = 0; i < constraints.size(); ++i) v.push_back(bool_arg(ctx, constraints[i], "constraints"));
  for (const Ast& a : v) {
    Z3_optimize_assert(ctx->c, h->o, a);
    check(ctx);
  }
  h->touch();
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_optimize_assert_and_track(SEXP o_, SEXP constraint, SEXP label_) {
  OptimizeHolder* h = optimizer(o_);
  const CtxPtr& ctx = h->o.ctx();
  Ast a = bool_arg(ctx, constraint, "tracked constraints");
  std::string name = as_string(label_, "name");
  Ast label = own_ast(ctx, Z3_mk_const(ctx->c, mk_symbol(ctx, name), Z3_mk_bool_sort(ctx->c)));
  Z3_optimize_assert_and_track(ctx->c, h->o, a, label);
  check(ctx);
  h->touch();
  return wrap_expr(label);
}

// Soft constraint; weight is an exact decimal or rational string.
// [[Rcpp::export(rng = false)]]
int cpp_optimize_assert_soft(SEXP o_, SEXP constraint, std::string weight, std::string id) {
  OptimizeHolder* h = optimizer(o_);
  const CtxPtr& ctx = h->o.ctx();
  Ast a = bool_arg(ctx, constraint, "soft constraints");
  unsigned idx = Z3_optimize_assert_soft(ctx->c, h->o, a, weight.c_str(), mk_symbol(ctx, id));
  check(ctx);
  h->touch();
  return static_cast<int>(idx);
}

// [[Rcpp::export(rng = false)]]
int cpp_optimize_objective(SEXP o_, SEXP term, bool maximize) {
  OptimizeHolder* h = optimizer(o_);
  const CtxPtr& ctx = h->o.ctx();
  Ast t = lift(ctx, term, nullptr);
  Z3_sort_kind k = sort_kind(t);
  if (k != Z3_INT_SORT && k != Z3_REAL_SORT && k != Z3_BV_SORT) {
    fail("objectives must be Int, Real or bit-vector expressions");
  }
  unsigned idx = maximize ? Z3_optimize_maximize(ctx->c, h->o, t)
                          : Z3_optimize_minimize(ctx->c, h->o, t);
  check(ctx);
  h->touch();
  return static_cast<int>(idx);
}

// [[Rcpp::export(rng = false)]]
std::string cpp_optimize_check(SEXP o_, Rcpp::List assumptions) {
  OptimizeHolder* h = optimizer(o_);
  const CtxPtr& ctx = h->o.ctx();
  AstVec as;
  for (R_xlen_t i = 0; i < assumptions.size(); ++i) as.push_back(bool_arg(ctx, assumptions[i], "assumptions"));
  std::vector<Z3_ast> r = raw(as);
  Z3_lbool st = Z3_optimize_check(ctx->c, h->o, static_cast<unsigned>(r.size()),
                                  r.empty() ? nullptr : r.data());
  h->checked = true;
  h->checked_at = h->generation;
  h->status = Z3_L_UNDEF;
  check(ctx);
  h->status = st;
  if (st == Z3_L_UNDEF) forward_interrupt(st, str(Z3_optimize_get_reason_unknown(ctx->c, h->o)));
  return lbool_string(st);
}

static void require_status(OptimizeHolder* h, Z3_lbool want, const char* what) {
  if (!h->checked) fail(std::string("call z3_check() before requesting ") + what);
  if (h->checked_at != h->generation) {
    fail(std::string("the optimizer has changed since the last z3_check(); call "
                     "z3_check() again before requesting ") + what);
  }
  if (h->status != want) {
    fail(std::string(what) + " is only available after z3_check() returns \"" +
         lbool_string(want) + "\" (the last result was \"" + lbool_string(h->status) + "\")");
  }
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_optimize_last_status(SEXP o_) {
  OptimizeHolder* h = optimizer(o_);
  if (!h->checked || h->checked_at != h->generation) return Rcpp::CharacterVector::create(NA_STRING);
  return Rcpp::CharacterVector::create(lbool_string(h->status));
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_optimize_model(SEXP o_) {
  OptimizeHolder* h = optimizer(o_);
  require_status(h, Z3_L_TRUE, "a model");
  const CtxPtr& ctx = h->o.ctx();
  return make_xptr(new Ref<Z3_model>(own(ctx, Z3_optimize_get_model(ctx->c, h->o))), Kind::Model);
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_optimize_unsat_core(SEXP o_) {
  OptimizeHolder* h = optimizer(o_);
  require_status(h, Z3_L_FALSE, "an unsatisfiable core");
  const CtxPtr& ctx = h->o.ctx();
  return wrap_ast_vector(ctx, Z3_optimize_get_unsat_core(ctx->c, h->o));
}

// Bound of objective `idx` as (infinity coefficient, finite part, epsilon
// coefficient): the bound is inf * oo + finite + eps * epsilon.
static SEXP bound_parts(const CtxPtr& ctx, Z3_ast_vector raw_v) {
  Ref<Z3_ast_vector> v = own(ctx, raw_v);
  unsigned n = Z3_ast_vector_size(ctx->c, v);
  if (n != 3) fail("unexpected objective bound representation from Z3");
  AstVec parts;
  for (unsigned i = 0; i < 3; ++i) parts.push_back(own_ast(ctx, Z3_ast_vector_get(ctx->c, v, i)));
  return Rcpp::List::create(Rcpp::Named("infinity") = wrap_expr(parts[0]),
                            Rcpp::Named("value") = wrap_expr(parts[1]),
                            Rcpp::Named("epsilon") = wrap_expr(parts[2]));
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_optimize_bounds(SEXP o_, int idx) {
  OptimizeHolder* h = optimizer(o_);
  require_status(h, Z3_L_TRUE, "objective values");
  const CtxPtr& ctx = h->o.ctx();
  if (idx < 0) fail("invalid objective index");
  unsigned i = static_cast<unsigned>(idx);
  SEXP lower = PROTECT(bound_parts(ctx, Z3_optimize_get_lower_as_vector(ctx->c, h->o, i)));
  SEXP upper = PROTECT(bound_parts(ctx, Z3_optimize_get_upper_as_vector(ctx->c, h->o, i)));
  SEXP out = Rcpp::List::create(Rcpp::Named("lower") = lower, Rcpp::Named("upper") = upper);
  UNPROTECT(2);
  return out;
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_optimize_objectives(SEXP o_) {
  OptimizeHolder* h = optimizer(o_);
  return wrap_ast_vector(h->o.ctx(), Z3_optimize_get_objectives(h->o.c(), h->o));
}

// [[Rcpp::export(rng = false)]]
void cpp_optimize_push(SEXP o_) {
  OptimizeHolder* h = optimizer(o_);
  Z3_optimize_push(h->o.c(), h->o);
  check(h->o.ctx());
  h->touch();
}

// [[Rcpp::export(rng = false)]]
void cpp_optimize_pop(SEXP o_) {
  OptimizeHolder* h = optimizer(o_);
  Z3_optimize_pop(h->o.c(), h->o);
  check(h->o.ctx());
  h->touch();
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_optimize_assertions(SEXP o_) {
  OptimizeHolder* h = optimizer(o_);
  return wrap_ast_vector(h->o.ctx(), Z3_optimize_get_assertions(h->o.c(), h->o));
}

// [[Rcpp::export(rng = false)]]
std::string cpp_optimize_reason_unknown(SEXP o_) {
  OptimizeHolder* h = optimizer(o_);
  std::string r = str(Z3_optimize_get_reason_unknown(h->o.c(), h->o));
  check(h->o.ctx());
  return r;
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_optimize_statistics(SEXP o_) {
  OptimizeHolder* h = optimizer(o_);
  return stats_list(h->o.ctx(), Z3_optimize_get_statistics(h->o.c(), h->o));
}

// [[Rcpp::export(rng = false)]]
void cpp_optimize_set_params(SEXP o_, SEXP params) {
  OptimizeHolder* h = optimizer(o_);
  const CtxPtr& ctx = h->o.ctx();
  Ref<Z3_param_descrs> d = own(ctx, Z3_optimize_get_param_descrs(ctx->c, h->o));
  Ref<Z3_params> p = make_params(ctx, d, params);
  Z3_optimize_set_params(ctx->c, h->o, p);
  check(ctx);
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_optimize_param_help(SEXP o_) {
  OptimizeHolder* h = optimizer(o_);
  return describe_params(h->o.ctx(), Z3_optimize_get_param_descrs(h->o.c(), h->o));
}

// [[Rcpp::export(rng = false)]]
std::string cpp_optimize_to_string(SEXP o_) {
  OptimizeHolder* h = optimizer(o_);
  std::string out = str(Z3_optimize_to_string(h->o.c(), h->o));
  check(h->o.ctx());
  return out;
}

// [[Rcpp::export(rng = false)]]
void cpp_optimize_from_string(SEXP o_, SEXP text_) {
  OptimizeHolder* h = optimizer(o_);
  std::string text = as_string(text_, "text");
  Z3_optimize_from_string(h->o.c(), h->o, text.c_str());
  check(h->o.ctx());
  h->touch();
}
