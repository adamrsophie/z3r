// Probes, tactic combinators, simplifiers and goal inspection.
#include "z3r.h"

using namespace z3r;

static const Ref<Z3_tactic>& tactic(SEXP x) { return *get_xptr<Ref<Z3_tactic>>(x, Kind::Tactic); }
static const Ref<Z3_goal>& goal(SEXP x) { return *get_xptr<Ref<Z3_goal>>(x, Kind::Goal); }
static const Ref<Z3_probe>& probe(SEXP x) { return *get_xptr<Ref<Z3_probe>>(x, Kind::Probe); }
static SEXP wrap_tactic(const CtxPtr& ctx, Z3_tactic t) {
  return make_xptr(new Ref<Z3_tactic>(own(ctx, t)), Kind::Tactic);
}
static SEXP wrap_probe(const CtxPtr& ctx, Z3_probe p) {
  return make_xptr(new Ref<Z3_probe>(own(ctx, p)), Kind::Probe);
}

// ---------------------------------------------------------------------------
// Probes

// [[Rcpp::export(rng = false)]]
SEXP cpp_mk_probe(SEXP ctx_, SEXP name_) {
  CtxPtr ctx = get_ctx(ctx_);
  std::string name = as_string(name_, "name");
  return wrap_probe(ctx, Z3_mk_probe(ctx->c, name.c_str()));
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_probe_const(SEXP ctx_, double value) {
  CtxPtr ctx = get_ctx(ctx_);
  return wrap_probe(ctx, Z3_probe_const(ctx->c, value));
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_probe_combine(std::string op, SEXP p1_, SEXP p2_) {
  const Ref<Z3_probe>& p1 = probe(p1_);
  const CtxPtr& ctx = p1.ctx();
  Z3_context c = ctx->c;
  if (op == "not") return wrap_probe(ctx, Z3_probe_not(c, p1));
  const Ref<Z3_probe>& p2 = probe(p2_);
  same_ctx(ctx, p2.ctx());
  Z3_probe r;
  if (op == "<") r = Z3_probe_lt(c, p1, p2);
  else if (op == ">") r = Z3_probe_gt(c, p1, p2);
  else if (op == "<=") r = Z3_probe_le(c, p1, p2);
  else if (op == ">=") r = Z3_probe_ge(c, p1, p2);
  else if (op == "==") r = Z3_probe_eq(c, p1, p2);
  else if (op == "&") r = Z3_probe_and(c, p1, p2);
  else if (op == "|") r = Z3_probe_or(c, p1, p2);
  else fail("probes support <, >, <=, >=, ==, &, | and !");
  return wrap_probe(ctx, r);
}

// [[Rcpp::export(rng = false)]]
double cpp_probe_apply(SEXP p_, SEXP g_) {
  const Ref<Z3_probe>& p = probe(p_);
  const Ref<Z3_goal>& g = goal(g_);
  same_ctx(p.ctx(), g.ctx());
  double v = Z3_probe_apply(p.c(), p, g);
  check(p.ctx());
  return v;
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_probe_names(SEXP ctx_) {
  CtxPtr ctx = get_ctx(ctx_);
  unsigned n = Z3_get_num_probes(ctx->c);
  Rcpp::CharacterVector name(n), doc(n);
  for (unsigned i = 0; i < n; ++i) {
    std::string nm = str(Z3_get_probe_name(ctx->c, i));
    name[i] = nm;
    doc[i] = str(Z3_probe_get_descr(ctx->c, nm.c_str()));
  }
  check(ctx);
  return Rcpp::DataFrame::create(Rcpp::Named("name") = name, Rcpp::Named("description") = doc,
                                 Rcpp::Named("stringsAsFactors") = false);
}

// ---------------------------------------------------------------------------
// Tactic combinators

// [[Rcpp::export(rng = false)]]
SEXP cpp_tactic_basic(SEXP ctx_, std::string which) {
  CtxPtr ctx = get_ctx(ctx_);
  Z3_tactic t = which == "skip" ? Z3_tactic_skip(ctx->c)
                : which == "fail" ? Z3_tactic_fail(ctx->c)
                                  : Z3_tactic_fail_if_not_decided(ctx->c);
  return wrap_tactic(ctx, t);
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_tactic_probe(std::string how, SEXP p_, SEXP t1_, SEXP t2_) {
  const Ref<Z3_probe>& p = probe(p_);
  const CtxPtr& ctx = p.ctx();
  Z3_context c = ctx->c;
  if (how == "fail_if") return wrap_tactic(ctx, Z3_tactic_fail_if(c, p));
  const Ref<Z3_tactic>& t1 = tactic(t1_);
  same_ctx(ctx, t1.ctx());
  if (how == "when") return wrap_tactic(ctx, Z3_tactic_when(c, p, t1));
  const Ref<Z3_tactic>& t2 = tactic(t2_);
  same_ctx(ctx, t2.ctx());
  return wrap_tactic(ctx, Z3_tactic_cond(c, p, t1, t2));
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_tactic_par(std::string how, Rcpp::List tactics) {
  if (tactics.size() == 0) fail("at least one tactic is needed");
  std::vector<Z3_tactic> ts;
  CtxPtr ctx = tactic(tactics[0]).ctx();
  for (R_xlen_t i = 0; i < tactics.size(); ++i) {
    const Ref<Z3_tactic>& t = tactic(tactics[i]);
    same_ctx(ctx, t.ctx());
    ts.push_back(t);
  }
  if (how == "par_or") {
    return wrap_tactic(ctx, Z3_tactic_par_or(ctx->c, static_cast<unsigned>(ts.size()), ts.data()));
  }
  if (ts.size() != 2) fail("z3_tactic_par_then() combines exactly two tactics");
  return wrap_tactic(ctx, Z3_tactic_par_and_then(ctx->c, ts[0], ts[1]));
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_tactic_using_params(SEXP t_, SEXP params) {
  const Ref<Z3_tactic>& t = tactic(t_);
  const CtxPtr& ctx = t.ctx();
  Ref<Z3_param_descrs> d = own(ctx, Z3_tactic_get_param_descrs(ctx->c, t));
  Ref<Z3_params> p = make_params(ctx, d, params);
  return wrap_tactic(ctx, Z3_tactic_using_params(ctx->c, t, p));
}

// [[Rcpp::export(rng = false)]]
std::string cpp_tactic_help(SEXP t_) {
  const Ref<Z3_tactic>& t = tactic(t_);
  std::string out = str(Z3_tactic_get_help(t.c(), t));
  check(t.ctx());
  return out;
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_tactic_param_help(SEXP t_) {
  const Ref<Z3_tactic>& t = tactic(t_);
  return describe_params(t.ctx(), Z3_tactic_get_param_descrs(t.c(), t));
}

// ---------------------------------------------------------------------------
// Goals

// [[Rcpp::export(rng = false)]]
SEXP cpp_goal_info(SEXP g_) {
  const Ref<Z3_goal>& g = goal(g_);
  Z3_context c = g.c();
  Z3_goal_prec p = Z3_goal_precision(c, g);
  const char* prec = p == Z3_GOAL_PRECISE ? "precise" : p == Z3_GOAL_UNDER ? "under"
                     : p == Z3_GOAL_OVER ? "over" : "under_over";
  SEXP out = Rcpp::List::create(Rcpp::Named("size") = static_cast<int>(Z3_goal_size(c, g)),
                                Rcpp::Named("num_exprs") = static_cast<int>(Z3_goal_num_exprs(c, g)),
                                Rcpp::Named("depth") = static_cast<int>(Z3_goal_depth(c, g)),
                                Rcpp::Named("precision") = prec,
                                Rcpp::Named("inconsistent") = Z3_goal_inconsistent(c, g));
  check(g.ctx());
  return out;
}

// [[Rcpp::export(rng = false)]]
void cpp_goal_reset(SEXP g_) {
  const Ref<Z3_goal>& g = goal(g_);
  Z3_goal_reset(g.c(), g);
  check(g.ctx());
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_goal_translate(SEXP g_, SEXP target_) {
  const Ref<Z3_goal>& g = goal(g_);
  CtxPtr target = get_ctx(target_);
  Z3_goal t = Z3_goal_translate(g.c(), g, target->c);
  check(g.ctx());
  return make_xptr(new Ref<Z3_goal>(own(target, t)), Kind::Goal);
}

// Converts a model of a subgoal into a model of the original goal.
// [[Rcpp::export(rng = false)]]
SEXP cpp_goal_convert_model(SEXP g_, SEXP m_) {
  const Ref<Z3_goal>& g = goal(g_);
  const Ref<Z3_model>& m = *get_xptr<Ref<Z3_model>>(m_, Kind::Model);
  same_ctx(g.ctx(), m.ctx());
  Z3_model r = Z3_goal_convert_model(g.c(), g, m);
  return make_xptr(new Ref<Z3_model>(own(g.ctx(), r)), Kind::Model);
}

// [[Rcpp::export(rng = false)]]
std::string cpp_goal_to_dimacs(SEXP g_, bool include_names) {
  const Ref<Z3_goal>& g = goal(g_);
  std::string out = str(Z3_goal_to_dimacs_string(g.c(), g, include_names));
  check(g.ctx());
  return out;
}

// ---------------------------------------------------------------------------
// Simplifiers (Z3 4.12.2 and later): preprocessing for incremental solvers

#if Z3R_AT_LEAST(4, 12, 2)
static const Ref<Z3_simplifier>& simplifier(SEXP x) {
  return *get_xptr<Ref<Z3_simplifier>>(x, Kind::Simplifier);
}
static SEXP wrap_simplifier(const CtxPtr& ctx, Z3_simplifier s) {
  return make_xptr(new Ref<Z3_simplifier>(own(ctx, s)), Kind::Simplifier);
}
#endif

// [[Rcpp::export(rng = false)]]
SEXP cpp_mk_simplifier(SEXP ctx_, SEXP name_) {
#if Z3R_AT_LEAST(4, 12, 2)
  CtxPtr ctx = get_ctx(ctx_);
  std::string name = as_string(name_, "name");
  return wrap_simplifier(ctx, Z3_mk_simplifier(ctx->c, name.c_str()));
#else
  (void)ctx_; (void)name_;
  needs_version("z3_simplifier()", "4.12.2");
#endif
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_simplifier_then(SEXP s1_, SEXP s2_) {
#if Z3R_AT_LEAST(4, 12, 2)
  const Ref<Z3_simplifier>& s1 = simplifier(s1_);
  const Ref<Z3_simplifier>& s2 = simplifier(s2_);
  same_ctx(s1.ctx(), s2.ctx());
  return wrap_simplifier(s1.ctx(), Z3_simplifier_and_then(s1.c(), s1, s2));
#else
  (void)s1_; (void)s2_;
  needs_version("z3_simplifier_then()", "4.12.2");
#endif
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_simplifier_using_params(SEXP s_, SEXP params) {
#if Z3R_AT_LEAST(4, 12, 2)
  const Ref<Z3_simplifier>& s = simplifier(s_);
  const CtxPtr& ctx = s.ctx();
  Ref<Z3_param_descrs> d = own(ctx, Z3_simplifier_get_param_descrs(ctx->c, s));
  Ref<Z3_params> p = make_params(ctx, d, params);
  return wrap_simplifier(ctx, Z3_simplifier_using_params(ctx->c, s, p));
#else
  (void)s_; (void)params;
  needs_version("z3_simplifier()", "4.12.2");
#endif
}

// [[Rcpp::export(rng = false)]]
std::string cpp_simplifier_help(SEXP s_) {
#if Z3R_AT_LEAST(4, 12, 2)
  const Ref<Z3_simplifier>& s = simplifier(s_);
  std::string out = str(Z3_simplifier_get_help(s.c(), s));
  check(s.ctx());
  return out;
#else
  (void)s_;
  needs_version("z3_help() for simplifiers", "4.12.2");
#endif
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_simplifier_names(SEXP ctx_) {
#if Z3R_AT_LEAST(4, 12, 2)
  CtxPtr ctx = get_ctx(ctx_);
  unsigned n = Z3_get_num_simplifiers(ctx->c);
  Rcpp::CharacterVector name(n), doc(n);
  for (unsigned i = 0; i < n; ++i) {
    std::string nm = str(Z3_get_simplifier_name(ctx->c, i));
    name[i] = nm;
    doc[i] = str(Z3_simplifier_get_descr(ctx->c, nm.c_str()));
  }
  check(ctx);
  return Rcpp::DataFrame::create(Rcpp::Named("name") = name, Rcpp::Named("description") = doc,
                                 Rcpp::Named("stringsAsFactors") = false);
#else
  (void)ctx_;
  needs_version("z3_simplifiers()", "4.12.2");
#endif
}

// Returns a new solver that runs the simplifier before solving.
// [[Rcpp::export(rng = false)]]
SEXP cpp_solver_add_simplifier(SEXP s_, SEXP simp_) {
#if Z3R_AT_LEAST(4, 12, 2)
  SolverHolder* h = solver_of(s_);
  refuse_copy_with_callbacks(h, "z3_solver_add_simplifier()");
  const Ref<Z3_simplifier>& sp = simplifier(simp_);
  same_ctx(h->s.ctx(), sp.ctx());
  Z3_solver ns = Z3_solver_add_simplifier(sp.c(), h->s, sp);
  SolverHolder* nh = new SolverHolder();
  std::unique_ptr<SolverHolder> guard(nh);
  nh->s = own(sp.ctx(), ns);
  guard.release();
  return make_xptr(nh, Kind::Solver);
#else
  (void)s_; (void)simp_;
  needs_version("z3_solver_add_simplifier()", "4.12.2");
#endif
}
