// Uninterpreted functions, quantifiers, tactics, goals and SMT-LIB2 text.
#include "z3r.h"

using namespace z3r;

// ---------------------------------------------------------------------------
// Uninterpreted functions

// [[Rcpp::export(rng = false)]]
SEXP cpp_mk_func_decl(SEXP ctx_, SEXP name_, Rcpp::List domain, SEXP range_) {
  CtxPtr ctx = get_ctx(ctx_);
  std::string name = as_string(name_, "name");
  std::vector<Z3_sort> dom;
  AstVec keep;
  for (R_xlen_t i = 0; i < domain.size(); ++i) {
    const Ast& s = get_sort(domain[i]);
    same_ctx(ctx, s.ctx());
    keep.push_back(s);
    dom.push_back(as_sort(s));
  }
  const Ast& r = get_sort(range_);
  same_ctx(ctx, r.ctx());
  Z3_func_decl d = Z3_mk_func_decl(ctx->c, mk_symbol(ctx, name), static_cast<unsigned>(dom.size()),
                                   dom.empty() ? nullptr : dom.data(), as_sort(r));
  check(ctx);
  return wrap_decl(own_ast(ctx, decl_ast(d)));
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_apply_decl(SEXP decl_, Rcpp::List args) {
  const Ast& decl = get_decl(decl_);
  const CtxPtr& ctx = decl.ctx();
  Z3_context c = ctx->c;
  Z3_func_decl d = as_decl(decl);
  unsigned arity = Z3_get_arity(c, d);
  if (static_cast<unsigned>(args.size()) != arity) {
    fail("the function takes " + std::to_string(arity) + " argument(s), got " +
         std::to_string(static_cast<long long>(args.size())));
  }
  AstVec v;
  for (unsigned i = 0; i < arity; ++i) {
    Z3_sort want = Z3_get_domain(c, d, i);
    Ast a = lift(ctx, args[i], want);
    if (Z3_get_sort_kind(c, want) == Z3_REAL_SORT && sort_kind(a) == Z3_INT_SORT) a = to_real(a);
    if (!Z3_is_eq_sort(c, Z3_get_sort(c, a), want)) {
      fail("argument " + std::to_string(i + 1) + " has sort " +
           sort_name(ctx, Z3_get_sort(c, a)) + " but the function expects " + sort_name(ctx, want));
    }
    v.push_back(a);
  }
  std::vector<Z3_ast> r = raw(v);
  return wrap_expr(own_ast(ctx, Z3_mk_app(c, d, arity, r.empty() ? nullptr : r.data())));
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_decl_info(SEXP decl_) {
  const Ast& decl = get_decl(decl_);
  const CtxPtr& ctx = decl.ctx();
  Z3_context c = ctx->c;
  Z3_func_decl d = as_decl(decl);
  unsigned arity = Z3_get_arity(c, d);
  Rcpp::List dom(arity);
  for (unsigned i = 0; i < arity; ++i) dom[i] = wrap_sort(own_ast(ctx, sort_ast(Z3_get_domain(c, d, i))));
  return Rcpp::List::create(Rcpp::Named("name") = symbol_string(ctx, Z3_get_decl_name(c, d)),
                            Rcpp::Named("domain") = dom,
                            Rcpp::Named("range") = wrap_sort(own_ast(ctx, sort_ast(Z3_get_range(c, d)))),
                            Rcpp::Named("text") = str(Z3_func_decl_to_string(c, d)));
}

// ---------------------------------------------------------------------------
// Quantifiers

bool cpp_is_const(SEXP x);

// [[Rcpp::export(rng = false)]]
SEXP cpp_quantifier(bool forall, Rcpp::List vars, SEXP body_, Rcpp::List patterns) {
  const Ast& body = get_expr(body_);
  const CtxPtr& ctx = body.ctx();
  Z3_context c = ctx->c;
  if (!is_bool(body)) fail("the body of a quantifier must be Boolean");
  if (vars.size() == 0) fail("a quantifier needs at least one bound variable");
  std::vector<Z3_app> bound;
  AstVec keep;
  for (R_xlen_t i = 0; i < vars.size(); ++i) {
    SEXP v = vars[i];
    if (!is_kind(v, Kind::Expr) || !cpp_is_const(v)) {
      fail("bound variables must be constants such as those made by z3_int()");
    }
    const Ast& a = get_expr(v);
    same_ctx(ctx, a.ctx());
    keep.push_back(a);
    bound.push_back(Z3_to_app(c, a));
  }
  std::vector<Z3_pattern> pats;
  for (R_xlen_t i = 0; i < patterns.size(); ++i) {
    Rcpp::List terms(patterns[i]);
    AstVec t;
    for (R_xlen_t j = 0; j < terms.size(); ++j) {
      t.push_back(get_expr(terms[j]));
      same_ctx(ctx, t.back().ctx());
      keep.push_back(t.back());
    }
    std::vector<Z3_ast> rt = raw(t);
    Z3_pattern p = Z3_mk_pattern(c, static_cast<unsigned>(rt.size()), rt.data());
    check(ctx);
    // Patterns are ASTs; hold a reference for the duration of the call.
    keep.push_back(own_ast(ctx, Z3_pattern_to_ast(c, p)));
    pats.push_back(p);
  }
  Z3_ast q = forall
      ? Z3_mk_forall_const(c, 0, static_cast<unsigned>(bound.size()), bound.data(),
                           static_cast<unsigned>(pats.size()), pats.empty() ? nullptr : pats.data(), body)
      : Z3_mk_exists_const(c, 0, static_cast<unsigned>(bound.size()), bound.data(),
                           static_cast<unsigned>(pats.size()), pats.empty() ? nullptr : pats.data(), body);
  return wrap_expr(own_ast(ctx, q));
}

// ---------------------------------------------------------------------------
// Tactics and goals

static const Ref<Z3_tactic>& tactic(SEXP x) { return *get_xptr<Ref<Z3_tactic>>(x, Kind::Tactic); }
static const Ref<Z3_goal>& goal(SEXP x) { return *get_xptr<Ref<Z3_goal>>(x, Kind::Goal); }
static SEXP wrap_tactic(const CtxPtr& ctx, Z3_tactic t) {
  return make_xptr(new Ref<Z3_tactic>(own(ctx, t)), Kind::Tactic);
}
static SEXP wrap_goal(const CtxPtr& ctx, Z3_goal g) {
  return make_xptr(new Ref<Z3_goal>(own(ctx, g)), Kind::Goal);
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_mk_tactic(SEXP ctx_, SEXP name_) {
  CtxPtr ctx = get_ctx(ctx_);
  std::string name = as_string(name_, "name");
  return wrap_tactic(ctx, Z3_mk_tactic(ctx->c, name.c_str()));
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_tactic_combine(std::string how, SEXP t1_, SEXP t2_) {
  const Ref<Z3_tactic>& t1 = tactic(t1_);
  const Ref<Z3_tactic>& t2 = tactic(t2_);
  same_ctx(t1.ctx(), t2.ctx());
  Z3_tactic t = how == "then" ? Z3_tactic_and_then(t1.c(), t1, t2) : Z3_tactic_or_else(t1.c(), t1, t2);
  return wrap_tactic(t1.ctx(), t);
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_tactic_modify(std::string how, SEXP t_, double n) {
  const Ref<Z3_tactic>& t = tactic(t_);
  if (ISNAN(n) || n < 0 || n > 4294967295.0) fail("the count must be a non-negative whole number");
  unsigned k = static_cast<unsigned>(n);
  Z3_tactic out = how == "repeat" ? Z3_tactic_repeat(t.c(), t, k) : Z3_tactic_try_for(t.c(), t, k);
  return wrap_tactic(t.ctx(), out);
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_tactic_names(SEXP ctx_) {
  CtxPtr ctx = get_ctx(ctx_);
  unsigned n = Z3_get_num_tactics(ctx->c);
  Rcpp::CharacterVector name(n), doc(n);
  for (unsigned i = 0; i < n; ++i) {
    std::string nm = str(Z3_get_tactic_name(ctx->c, i));
    name[i] = nm;
    doc[i] = str(Z3_tactic_get_descr(ctx->c, nm.c_str()));
  }
  check(ctx);
  return Rcpp::DataFrame::create(Rcpp::Named("name") = name, Rcpp::Named("description") = doc,
                                 Rcpp::Named("stringsAsFactors") = false);
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_mk_goal(SEXP ctx_, bool models, bool unsat_cores, bool proofs) {
  CtxPtr ctx = get_ctx(ctx_);
  return wrap_goal(ctx, Z3_mk_goal(ctx->c, models, unsat_cores, proofs));
}

// [[Rcpp::export(rng = false)]]
void cpp_goal_assert(SEXP g_, Rcpp::List constraints) {
  const Ref<Z3_goal>& g = goal(g_);
  const CtxPtr& ctx = g.ctx();
  AstVec v;
  for (R_xlen_t i = 0; i < constraints.size(); ++i) {
    Ast a = lift(ctx, constraints[i], Z3_mk_bool_sort(ctx->c));
    if (!is_bool(a)) fail("goal formulas must be Boolean");
    v.push_back(a);
  }
  for (const Ast& a : v) {
    Z3_goal_assert(ctx->c, g, a);
    check(ctx);
  }
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_goal_formulas(SEXP g_) {
  const Ref<Z3_goal>& g = goal(g_);
  const CtxPtr& ctx = g.ctx();
  unsigned n = Z3_goal_size(ctx->c, g);
  AstVec v;
  for (unsigned i = 0; i < n; ++i) v.push_back(own_ast(ctx, Z3_goal_formula(ctx->c, g, i)));
  return wrap_expr_list(v);
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_goal_status(SEXP g_) {
  const Ref<Z3_goal>& g = goal(g_);
  Z3_context c = g.c();
  bool sat = Z3_goal_is_decided_sat(c, g);
  bool unsat = Z3_goal_is_decided_unsat(c, g);
  check(g.ctx());
  return Rcpp::CharacterVector::create(sat ? "sat" : unsat ? "unsat" : "undecided");
}

// [[Rcpp::export(rng = false)]]
std::string cpp_goal_to_string(SEXP g_) {
  const Ref<Z3_goal>& g = goal(g_);
  std::string out = str(Z3_goal_to_string(g.c(), g));
  check(g.ctx());
  return out;
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_tactic_apply(SEXP t_, SEXP g_, SEXP params) {
  const Ref<Z3_tactic>& t = tactic(t_);
  const Ref<Z3_goal>& g = goal(g_);
  const CtxPtr& ctx = t.ctx();
  same_ctx(ctx, g.ctx());
  Z3_context c = ctx->c;
  Z3_apply_result raw_r;
  if (Rf_xlength(params) > 0) {
    Ref<Z3_param_descrs> d = own(ctx, Z3_tactic_get_param_descrs(c, t));
    Ref<Z3_params> p = make_params(ctx, d, params);
    raw_r = Z3_tactic_apply_ex(c, t, g, p);
  } else {
    raw_r = Z3_tactic_apply(c, t, g);
  }
  Ref<Z3_apply_result> r = own(ctx, raw_r);
  unsigned n = Z3_apply_result_get_num_subgoals(c, r);
  Rcpp::List out(n);
  for (unsigned i = 0; i < n; ++i) out[i] = wrap_goal(ctx, Z3_apply_result_get_subgoal(c, r, i));
  return out;
}

// ---------------------------------------------------------------------------
// SMT-LIB2 text

// Parses declarations and assertions; returns the asserted formulas.
// [[Rcpp::export(rng = false)]]
SEXP cpp_parse_smtlib2(SEXP ctx_, SEXP text_, bool is_file) {
  CtxPtr ctx = get_ctx(ctx_);
  std::string text = as_string(text_, is_file ? "file" : "text");
  Z3_ast_vector v = is_file
      ? Z3_parse_smtlib2_file(ctx->c, text.c_str(), 0, nullptr, nullptr, 0, nullptr, nullptr)
      : Z3_parse_smtlib2_string(ctx->c, text.c_str(), 0, nullptr, nullptr, 0, nullptr, nullptr);
  return wrap_ast_vector(ctx, v);
}

// Runs SMT-LIB2 commands and returns Z3's textual output.
// [[Rcpp::export(rng = false)]]
std::string cpp_eval_smtlib2(SEXP ctx_, SEXP text_) {
  CtxPtr ctx = get_ctx(ctx_);
  std::string text = as_string(text_, "text");
  std::string out = str(Z3_eval_smtlib2_string(ctx->c, text.c_str()));
  check(ctx);
  return out;
}

// SMT-LIB2 benchmark for a list of Boolean formulas.
// [[Rcpp::export(rng = false)]]
std::string cpp_benchmark(Rcpp::List formulas, std::string logic) {
  CtxPtr ctx = ctx_from_list(formulas);
  AstVec v;
  for (R_xlen_t i = 0; i < formulas.size(); ++i) {
    Ast a = lift(ctx, formulas[i], Z3_mk_bool_sort(ctx->c));
    if (!is_bool(a)) fail("only Boolean formulas can be exported as assertions");
    v.push_back(a);
  }
  Ast last = v.back();
  v.pop_back();
  std::vector<Z3_ast> r = raw(v);
  std::string out = str(Z3_benchmark_to_smtlib_string(
      ctx->c, "", logic.c_str(), "unknown", "", static_cast<unsigned>(r.size()),
      r.empty() ? nullptr : r.data(), last));
  check(ctx);
  return out;
}
