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
  return Rcpp::List::create(Rcpp::Named("name") = Rcpp::String(symbol_string(ctx, Z3_get_decl_name(c, d))),
                            Rcpp::Named("domain") = dom,
                            Rcpp::Named("range") = wrap_sort(own_ast(ctx, sort_ast(Z3_get_range(c, d)))),
                            Rcpp::Named("text") = str(Z3_func_decl_to_string(c, d)));
}

// ---------------------------------------------------------------------------
// Quantifiers

bool cpp_is_const(SEXP x);

namespace {

// Patterns from an R list of patterns, each a list of terms. `keep` holds
// the terms and the patterns for the duration of the call.
std::vector<Z3_pattern> make_patterns(const CtxPtr& ctx, Rcpp::List patterns, AstVec& keep) {
  Z3_context c = ctx->c;
  std::vector<Z3_pattern> pats;
  for (R_xlen_t i = 0; i < patterns.size(); ++i) {
    Rcpp::List terms(patterns[i]);
    if (terms.size() == 0) fail("a pattern needs at least one term");
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
  return pats;
}

}  // namespace

// Quantifier over constants. Uses the plainest Z3 entry point that takes
// the options given.
// [[Rcpp::export(rng = false)]]
SEXP cpp_quantifier(bool forall, Rcpp::List vars, SEXP body_, Rcpp::List patterns,
                    Rcpp::List no_patterns, int weight, std::string qid, std::string skid) {
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
  std::vector<Z3_pattern> pats = make_patterns(ctx, patterns, keep);
  AstVec nopats = expr_list(ctx, no_patterns, "no-patterns");
  std::vector<Z3_ast> rn = raw(nopats);
  if (!pats.empty() && !rn.empty()) fail("a quantifier takes patterns or no-patterns, not both");
  if (weight < 0) fail("weight must be a non-negative integer");
  unsigned w = static_cast<unsigned>(weight);
  unsigned nb = static_cast<unsigned>(bound.size());
  unsigned np = static_cast<unsigned>(pats.size());
  Z3_pattern* pp = pats.empty() ? nullptr : pats.data();
  Z3_ast q;
  if (!qid.empty() || !skid.empty() || !rn.empty()) {
    q = Z3_mk_quantifier_const_ex(c, forall, w, mk_symbol(ctx, qid), mk_symbol(ctx, skid), nb, bound.data(), np,
                                  pp, static_cast<unsigned>(rn.size()), rn.empty() ? nullptr : rn.data(), body);
  } else if (w != 0 || np != 0) {
    q = Z3_mk_quantifier_const(c, forall, w, nb, bound.data(), np, pp, body);
  } else if (forall) {
    q = Z3_mk_forall_const(c, 0, nb, bound.data(), 0, nullptr, body);
  } else {
    q = Z3_mk_exists_const(c, 0, nb, bound.data(), 0, nullptr, body);
  }
  return wrap_expr(own_ast(ctx, q));
}

// Quantifier or lambda over de Bruijn variables (see z3_bound_var()). Z3
// binds index 0 to the last of `sorts` and `names`, index 1 to the one
// before it, and so on; higher indices refer to enclosing binders.
// [[Rcpp::export(rng = false)]]
SEXP cpp_quantifier_db(std::string type, Rcpp::List sorts, Rcpp::CharacterVector names, SEXP body_,
                       Rcpp::List patterns, Rcpp::List no_patterns, int weight, std::string qid,
                       std::string skid) {
  const Ast& body = get_expr(body_);
  const CtxPtr& ctx = body.ctx();
  Z3_context c = ctx->c;
  bool lambda = type == "lambda", forall = type == "forall";
  if (!lambda && !forall && type != "exists") fail("type must be \"forall\", \"exists\" or \"lambda\"");
  if (lambda && (patterns.size() != 0 || no_patterns.size() != 0 || weight != 0 || !qid.empty() || !skid.empty())) {
    fail("a lambda takes no patterns, no-patterns, weight or identifiers");
  }
  if (!lambda && !is_bool(body)) fail("the body of a quantifier must be Boolean");
  R_xlen_t n = sorts.size();
  if (n == 0) fail(std::string("a ") + (lambda ? "lambda" : "quantifier") + " needs at least one bound variable");
  if (names.size() != n) fail("sorts and names must have the same length");
  AstVec keep;
  std::vector<Z3_sort> ss;
  std::vector<Z3_symbol> syms;
  std::vector<std::string> labels;
  for (R_xlen_t i = 0; i < n; ++i) {
    const Ast& s = get_sort(sorts[i]);
    same_ctx(ctx, s.ctx());
    keep.push_back(s);
    ss.push_back(as_sort(s));
    SEXP nm = STRING_ELT(names, i);
    if (nm == NA_STRING) fail("names must not be missing");
    labels.push_back(Rf_translateCharUTF8(nm));
    syms.push_back(mk_symbol(ctx, labels.back()));
  }
  std::vector<Z3_pattern> pats = make_patterns(ctx, patterns, keep);
  AstVec nopats = expr_list(ctx, no_patterns, "no-patterns");
  std::vector<Z3_ast> rn = raw(nopats);
  // Z3 does not check that the variables it binds have the declared sorts,
  // in the body, the patterns or the no-patterns.
  std::vector<Z3_ast> scope = rn;
  scope.push_back(body);
  for (Z3_pattern p : pats) scope.push_back(Z3_pattern_to_ast(c, p));
  std::vector<Z3_sort> fv = free_var_sorts(c, scope);
  for (R_xlen_t k = 0; k < n && k < static_cast<R_xlen_t>(fv.size()); ++k) {
    Z3_sort want = ss[n - 1 - k];
    if (fv[k] && !Z3_is_eq_sort(c, fv[k], want)) {
      fail("bound variable " + std::to_string(k) + " has sort " + sort_name(ctx, fv[k]) + " but its binder '" +
           labels[n - 1 - k] + "' has sort " + sort_name(ctx, want));
    }
  }
  unsigned nd = static_cast<unsigned>(n);
  Z3_ast q;
  if (lambda) {
    q = Z3_mk_lambda(c, nd, ss.data(), syms.data(), body);
    return wrap_expr(own_ast(ctx, q));
  }
  if (!pats.empty() && !rn.empty()) fail("a quantifier takes patterns or no-patterns, not both");
  if (weight < 0) fail("weight must be a non-negative integer");
  unsigned w = static_cast<unsigned>(weight);
  unsigned np = static_cast<unsigned>(pats.size());
  Z3_pattern* pp = pats.empty() ? nullptr : pats.data();
  if (!qid.empty() || !skid.empty() || !rn.empty()) {
    q = Z3_mk_quantifier_ex(c, forall, w, mk_symbol(ctx, qid), mk_symbol(ctx, skid), np, pp,
                            static_cast<unsigned>(rn.size()), rn.empty() ? nullptr : rn.data(), nd, ss.data(),
                            syms.data(), body);
  } else if (w != 0 || np != 0) {
    q = Z3_mk_quantifier(c, forall, w, np, pp, nd, ss.data(), syms.data(), body);
  } else if (forall) {
    q = Z3_mk_forall(c, 0, 0, nullptr, nd, ss.data(), syms.data(), body);
  } else {
    q = Z3_mk_exists(c, 0, 0, nullptr, nd, ss.data(), syms.data(), body);
  }
  return wrap_expr(own_ast(ctx, q));
}

// Z3's text for a pattern of one or more terms.
// [[Rcpp::export(rng = false)]]
Rcpp::String cpp_pattern_to_string(Rcpp::List terms) {
  CtxPtr ctx = ctx_from_list(terms);
  AstVec keep;
  std::vector<Z3_pattern> p = make_patterns(ctx, Rcpp::List::create(terms), keep);
  std::string out = str(Z3_pattern_to_string(ctx->c, p[0]));
  check(ctx);
  return out;
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
Rcpp::String cpp_goal_to_string(SEXP g_) {
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
  // The subgoals are the result's own goals, so the result is kept for
  // format(), which shows later changes to them.
  out.attr("result") = make_xptr(new Ref<Z3_apply_result>(r), Kind::ApplyResult);
  out.attr("class") = "z3_apply_result";
  return out;
}

// Z3's text of a tactic result.
// [[Rcpp::export(rng = false)]]
Rcpp::String cpp_apply_result_to_string(SEXP r_) {
  const Ref<Z3_apply_result>& r = *get_xptr<Ref<Z3_apply_result>>(r_, Kind::ApplyResult);
  std::string out = str(Z3_apply_result_to_string(r.c(), r));
  check(r.ctx());
  return out;
}

// ---------------------------------------------------------------------------
// SMT-LIB2 text

// Parses declarations and assertions; returns the asserted formulas. The
// named lists `sorts` and `decls` give SMT-LIB names for existing sorts and
// function declarations, which the text may use without declaring them.
// [[Rcpp::export(rng = false)]]
SEXP cpp_parse_smtlib2(SEXP ctx_, SEXP text_, bool is_file, Rcpp::List sorts, Rcpp::List decls) {
  CtxPtr ctx = get_ctx(ctx_);
  std::string text = as_string(text_, is_file ? "file" : "text");
  auto names_of = [&](Rcpp::List l, const char* what) {
    std::vector<Z3_symbol> out;
    if (l.size() == 0) return out;
    SEXP nm = Rf_getAttrib(l, R_NamesSymbol);
    for (R_xlen_t i = 0; i < l.size(); ++i) {
      if (nm == R_NilValue || STRING_ELT(nm, i) == NA_STRING || CHAR(STRING_ELT(nm, i))[0] == '\0') {
        fail(std::string(what) + " must be a named list");
      }
      out.push_back(mk_symbol(ctx, Rf_translateCharUTF8(STRING_ELT(nm, i))));
    }
    return out;
  };
  std::vector<Z3_symbol> sort_names = names_of(sorts, "sorts"), decl_names = names_of(decls, "decls");
  std::vector<Z3_sort> ss;
  std::vector<Z3_func_decl> ds;
  AstVec keep;
  for (R_xlen_t i = 0; i < sorts.size(); ++i) {
    const Ast& a = get_sort(sorts[i]);
    same_ctx(ctx, a.ctx());
    keep.push_back(a);
    ss.push_back(as_sort(a));
  }
  for (R_xlen_t i = 0; i < decls.size(); ++i) {
    const Ast& a = get_decl(decls[i]);
    same_ctx(ctx, a.ctx());
    keep.push_back(a);
    ds.push_back(as_decl(a));
  }
  unsigned ns = static_cast<unsigned>(ss.size()), nd = static_cast<unsigned>(ds.size());
  Z3_symbol* sn = ss.empty() ? nullptr : sort_names.data();
  Z3_sort* sp = ss.empty() ? nullptr : ss.data();
  Z3_symbol* dn = ds.empty() ? nullptr : decl_names.data();
  Z3_func_decl* dp = ds.empty() ? nullptr : ds.data();
  Z3_ast_vector v = is_file ? Z3_parse_smtlib2_file(ctx->c, text.c_str(), ns, sn, sp, nd, dn, dp)
                            : Z3_parse_smtlib2_string(ctx->c, text.c_str(), ns, sn, sp, nd, dn, dp);
  return wrap_ast_vector(ctx, v);
}

// Runs SMT-LIB2 commands and returns Z3's textual output.
// [[Rcpp::export(rng = false)]]
Rcpp::String cpp_eval_smtlib2(SEXP ctx_, SEXP text_) {
  CtxPtr ctx = get_ctx(ctx_);
  std::string text = as_string(text_, "text");
  // The proof checker runs inside the interpreter and must not start
  // another evaluation on the same context.
  if (ctx->interpreting) fail("z3_eval_smtlib() cannot run inside a proof checker on the same context");
  if (ctx->proof_check) {
    ctx->proof_check->error.clear();
    ctx->proof_check->interrupted = false;
  }
  std::string out;
  {
    struct Flag {
      bool& f;
      explicit Flag(bool& x) : f(x) { f = true; }
      ~Flag() { f = false; }
    } flag(ctx->interpreting);
    out = str(Z3_eval_smtlib2_string(ctx->c, text.c_str()));
  }
  raise_proof_check_error(ctx);
  check(ctx);
  return out;
}

// SMT-LIB2 benchmark for a list of Boolean formulas.
// [[Rcpp::export(rng = false)]]
Rcpp::String cpp_benchmark(Rcpp::List formulas, std::string logic) {
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
