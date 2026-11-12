// Optimization (Z3's MaxSMT / OptSMT engine).
#include "z3r.h"

using namespace z3r;

static OptimizeHolder* optimizer(SEXP x) { return optimizer_of(x); }

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
  h->note_objective(idx);
  return static_cast<int>(idx);
}

// Returns the objective's index and serial number.
// [[Rcpp::export(rng = false)]]
Rcpp::NumericVector cpp_optimize_objective(SEXP o_, SEXP term, bool maximize) {
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
  uint64_t serial = h->note_objective(idx);
  return Rcpp::NumericVector::create(static_cast<double>(idx), static_cast<double>(serial));
}

// [[Rcpp::export(rng = false)]]
std::string cpp_optimize_check(SEXP o_, Rcpp::List assumptions) {
  OptimizeHolder* h = optimizer(o_);
  const CtxPtr& ctx = h->o.ctx();
  AstVec as;
  for (R_xlen_t i = 0; i < assumptions.size(); ++i) as.push_back(bool_arg(ctx, assumptions[i], "assumptions"));
  std::vector<Z3_ast> r = raw(as);
  Z3_lbool st;
  {
    BusyGuard<OptimizeHolder> busy(h);
    st = Z3_optimize_check(ctx->c, h->o, static_cast<unsigned>(r.size()), r.empty() ? nullptr : r.data());
  }
  h->checked = true;
  h->checked_at = h->generation;
  h->status = Z3_L_UNDEF;
  raise_callback_errors(h->callbacks);
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
SEXP cpp_optimize_bounds(SEXP o_, int idx, double serial) {
  OptimizeHolder* h = optimizer(o_);
  if (idx < 0 || static_cast<size_t>(idx) >= h->objectives.size() ||
      static_cast<double>(h->objectives[idx]) != serial) {
    fail("this objective was removed by z3_pop()");
  }
  require_status(h, Z3_L_TRUE, "objective values");
  const CtxPtr& ctx = h->o.ctx();
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
  // Count the objectives Z3 has, including any added by parsing.
  Ref<Z3_ast_vector> objs = own(h->o.ctx(), Z3_optimize_get_objectives(h->o.c(), h->o));
  size_t n = Z3_ast_vector_size(h->o.c(), objs);
  Z3_optimize_push(h->o.c(), h->o);
  check(h->o.ctx());
  h->touch();
  h->scopes.push_back(n);
}

// [[Rcpp::export(rng = false)]]
void cpp_optimize_pop(SEXP o_) {
  OptimizeHolder* h = optimizer(o_);
  // Z3 ignores a pop without a scope; refuse it, as for solvers.
  if (h->scopes.empty()) fail("cannot pop: the optimizer has no open scope from z3_push()");
  Z3_optimize_pop(h->o.c(), h->o);
  check(h->o.ctx());
  h->touch();
  if (h->objectives.size() > h->scopes.back()) h->objectives.resize(h->scopes.back());
  h->scopes.pop_back();
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

// [[Rcpp::export(rng = false)]]
std::string cpp_optimize_help(SEXP o_) {
  OptimizeHolder* h = optimizer(o_);
  std::string out = str(Z3_optimize_get_help(h->o.c(), h->o));
  check(h->o.ctx());
  return out;
}

// [[Rcpp::export(rng = false)]]
void cpp_optimize_from_file(SEXP o_, SEXP file_) {
  OptimizeHolder* h = optimizer(o_);
  std::string file = as_string(file_, "file");
  Z3_optimize_from_file(h->o.c(), h->o, file.c_str());
  check(h->o.ctx());
  h->touch();
}

// [[Rcpp::export(rng = false)]]
void cpp_optimize_initial_value(SEXP o_, SEXP var, SEXP value) {
#if Z3R_AT_LEAST(4, 13, 2)
  OptimizeHolder* h = optimizer(o_);
  const CtxPtr& ctx = h->o.ctx();
  Ast v = lift(ctx, var, nullptr);
  Ast val = lift(ctx, value, Z3_get_sort(ctx->c, v));
  Z3_optimize_set_initial_value(ctx->c, h->o, v, val);
  check(ctx);
#else
  (void)o_; (void)var; (void)value;
  needs_version("z3_set_initial_value() for optimizers", "4.13.2");
#endif
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_optimize_translate(SEXP o_, SEXP target_) {
#if Z3R_AT_LEAST(4, 15, 5)
  OptimizeHolder* h = optimizer(o_);
  if (!h->callbacks.empty()) {
    fail("z3_translate() cannot copy an optimizer with a z3_on_model() callback; Z3 would copy it without it");
  }
  CtxPtr target = get_ctx(target_);
  Z3_optimize t = Z3_optimize_translate(h->o.c(), h->o, target->c);
  check(h->o.ctx());
  OptimizeHolder* nh = new OptimizeHolder();
  std::unique_ptr<OptimizeHolder> guard(nh);
  nh->o = own(target, t);
  guard.release();
  return make_xptr(nh, Kind::Optimize);
#else
  (void)o_; (void)target_;
  needs_version("z3_translate() for optimizers", "4.15.5");
#endif
}
