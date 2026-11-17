// Probes, tactic combinators, simplifiers and goal inspection.
#include "z3r.h"

#include <cstdio>
#include <map>

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
  const Ref<Z3_model>& m = model_holder(m_)->m;
  same_ctx(g.ctx(), m.ctx());
  Z3_model r = Z3_goal_convert_model(g.c(), g, m);
  return wrap_model(own(g.ctx(), r));
}

// DIMACS export. z3r prints the clauses itself: Z3's printer turns any
// formula that is not a literal into a fresh variable, which changes its
// meaning (p == !p would become a satisfiable unit clause), and writes
// names into comments unescaped, so a name holding a newline adds a clause.

namespace {
// A literal: a Boolean constant or its negation, or true or false.
struct DimacsLit {
  enum Kind { Bad, Atom, True, False } kind = Bad;
  Z3_ast atom = nullptr;
  bool negated = false;
};

DimacsLit dimacs_literal(Z3_context c, Z3_ast f, bool may_negate = true) {
  DimacsLit l;
  if (!Z3_is_app(c, f)) return l;
  Z3_app a = Z3_to_app(c, f);
  Z3_func_decl d = Z3_get_app_decl(c, a);
  Z3_decl_kind k = Z3_get_decl_kind(c, d);
  if (k == Z3_OP_TRUE) l.kind = DimacsLit::True;
  else if (k == Z3_OP_FALSE) l.kind = DimacsLit::False;
  else if (k == Z3_OP_NOT && may_negate) {
    l = dimacs_literal(c, Z3_get_app_arg(c, a, 0), false);
    if (l.kind == DimacsLit::Atom) l.negated = true;
    else if (l.kind == DimacsLit::True) l.kind = DimacsLit::False;
    else if (l.kind == DimacsLit::False) l.kind = DimacsLit::True;
  } else if (k == Z3_OP_UNINTERPRETED && Z3_get_app_num_args(c, a) == 0 &&
             Z3_get_sort_kind(c, Z3_get_sort(c, f)) == Z3_BOOL_SORT) {
    l.kind = DimacsLit::Atom;
    l.atom = f;
  }
  return l;
}

struct DimacsWriter {
  Z3_context c;
  const CtxPtr& ctx;
  std::map<unsigned, int> var;  // AST id of an atom -> DIMACS variable
  std::vector<Z3_ast> atoms;
  std::vector<std::vector<int>> clauses;

  int var_of(Z3_ast a) {
    auto it = var.find(Z3_get_ast_id(c, a));
    if (it != var.end()) return it->second;
    atoms.push_back(a);
    int v = static_cast<int>(atoms.size());
    var[Z3_get_ast_id(c, a)] = v;
    return v;
  }
  // Adds lit to the clause; returns false when it makes the clause true.
  bool add(const DimacsLit& l, std::vector<int>& cl) {
    if (l.kind == DimacsLit::True) return false;
    if (l.kind == DimacsLit::Atom) {
      int v = var_of(l.atom);
      cl.push_back(l.negated ? -v : v);
    }
    return true;  // a false literal adds nothing
  }
  // A clause: a literal or a disjunction of literals. Sets *ok to false if f
  // is not one.
  void clause(Z3_ast f, std::vector<int> cl, bool* ok) {
    DimacsLit l = dimacs_literal(c, f);
    if (l.kind != DimacsLit::Bad) {
      if (add(l, cl)) clauses.push_back(cl);
      return;
    }
    if (Z3_is_app(c, f) && Z3_get_decl_kind(c, Z3_get_app_decl(c, Z3_to_app(c, f))) == Z3_OP_OR) {
      Z3_app a = Z3_to_app(c, f);
      std::vector<DimacsLit> ls;
      for (unsigned j = 0, n = Z3_get_app_num_args(c, a); j < n; ++j) {
        ls.push_back(dimacs_literal(c, Z3_get_app_arg(c, a, j)));
        if (ls.back().kind == DimacsLit::Bad) { *ok = false; return; }
      }
      for (const DimacsLit& x : ls) if (!add(x, cl)) return;
      clauses.push_back(cl);
      return;
    }
    *ok = false;
  }
  void formula(Z3_ast f) {
    bool ok = true;
    Z3_app a = Z3_is_app(c, f) ? Z3_to_app(c, f) : nullptr;
    Z3_decl_kind k = a ? Z3_get_decl_kind(c, Z3_get_app_decl(c, a)) : Z3_OP_UNINTERPRETED;
    if (k == Z3_OP_AND) {
      for (unsigned j = 0, n = Z3_get_app_num_args(c, a); j < n; ++j) formula(Z3_get_app_arg(c, a, j));
      return;
    }
    if (k == Z3_OP_IMPLIES) {
      // A tracked clause, label => clause, is (not label) or clause.
      DimacsLit lhs = dimacs_literal(c, Z3_get_app_arg(c, a, 0), false);
      if (lhs.kind == DimacsLit::Atom) {
        std::vector<int> cl(1, -var_of(lhs.atom));
        clause(Z3_get_app_arg(c, a, 1), cl, &ok);
        if (ok) return;
      }
      ok = false;
    } else {
      clause(f, std::vector<int>(), &ok);
    }
    if (!ok) {
      fail("DIMACS needs clauses of Boolean constants, and '" + str(Z3_ast_to_string(c, f)) +
           "' is not one; apply the \"tseitin-cnf\" tactic to a goal (after \"bit-blast\" for "
           "bit-vectors) first");
    }
  }
};

// A name for a DIMACS comment: control characters and backslashes escaped,
// so the comment stays on its line.
std::string dimacs_name(const std::string& s) {
  std::string out;
  for (unsigned char ch : s) {
    if (ch == '\\') out += "\\\\";
    else if (ch == '\n') out += "\\n";
    else if (ch == '\r') out += "\\r";
    else if (ch < 0x20 || ch == 0x7f) {
      char buf[8];
      std::snprintf(buf, sizeof(buf), "\\x%02x", ch);
      out += buf;
    } else out += static_cast<char>(ch);
  }
  return out;
}
}  // namespace

std::string z3r::dimacs_text(const CtxPtr& ctx, const AstVec& formulas, bool include_names) {
  DimacsWriter w{ctx->c, ctx, {}, {}, {}};
  for (const Ast& a : formulas) w.formula(a);
  std::string out = "p cnf " + std::to_string(w.atoms.size()) + " " + std::to_string(w.clauses.size());
  for (const std::vector<int>& cl : w.clauses) {
    out += "\n";
    for (int l : cl) out += std::to_string(l) + " ";
    out += "0";
  }
  if (include_names) {
    for (size_t i = 0; i < w.atoms.size(); ++i) {
      Z3_func_decl d = Z3_get_app_decl(ctx->c, Z3_to_app(ctx->c, w.atoms[i]));
      out += "\nc " + std::to_string(i + 1) + " " + dimacs_name(symbol_string(ctx, Z3_get_decl_name(ctx->c, d)));
    }
  }
  check(ctx);
  return out;
}

// [[Rcpp::export(rng = false)]]
Rcpp::String cpp_goal_to_dimacs(SEXP g_, bool include_names) {
  const Ref<Z3_goal>& g = goal(g_);
  Z3_context c = g.c();
  AstVec fs;
  for (unsigned i = 0, n = Z3_goal_size(c, g); i < n; ++i) fs.push_back(own_ast(g.ctx(), Z3_goal_formula(c, g, i)));
  return dimacs_text(g.ctx(), fs, include_names);
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
  // z3r cannot carry assertions or scopes into the new solver's record of
  // its inputs (a solver with a simplifier holds them back from Z3's list).
  bool nonempty = Z3_solver_get_num_scopes(h->s.c(), h->s) > 0 || !h->inputs.empty() || h->inputs_partial;
  if (!h->simplified) {
    Ref<Z3_ast_vector> v = own(h->s.ctx(), Z3_solver_get_assertions(h->s.c(), h->s));
    nonempty = nonempty || Z3_ast_vector_size(h->s.c(), v) > 0;
  }
  check(h->s.ctx());
  if (nonempty) fail("add the simplifier to a solver with no assertions or scopes");
  const Ref<Z3_simplifier>& sp = simplifier(simp_);
  same_ctx(h->s.ctx(), sp.ctx());
  Z3_solver ns = Z3_solver_add_simplifier(sp.c(), h->s, sp);
  SolverHolder* nh = new SolverHolder();
  std::unique_ptr<SolverHolder> guard(nh);
  nh->s = own(sp.ctx(), ns);
  nh->simplified = true;
  // The new solver starts with Z3's defaults: apply the base solver's set.
  nh->params = h->params;
  if (!nh->params.empty()) {
    Z3_solver_set_params(sp.c(), nh->s, kept_param_set(sp.ctx(), nh->params));
    check(sp.ctx());
  }
  guard.release();
  return make_xptr(nh, Kind::Solver);
#else
  (void)s_; (void)simp_;
  needs_version("z3_solver_add_simplifier()", "4.12.2");
#endif
}
