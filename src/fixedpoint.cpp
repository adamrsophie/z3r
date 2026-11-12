// Fixedpoint engine: Datalog and constrained Horn clauses (Spacer).
#include "z3r.h"

#include <algorithm>

using namespace z3r;

namespace {

FixedpointHolder* fixedpoint(SEXP x) { return fixedpoint_of(x); }

void require_status(FixedpointHolder* h, const char* what) {
  if (!h->checked) fail(std::string("call z3_query() before requesting ") + what);
  if (h->checked_at != h->generation) {
    fail(std::string("the engine has changed since the last z3_query(); query again before requesting ") + what);
  }
}

}  // namespace

namespace z3r {
CtxPtr fixedpoint_ctx(SEXP x) { return get_xptr<FixedpointHolder>(x, Kind::Fixedpoint)->f.ctx(); }
}  // namespace z3r

// [[Rcpp::export(rng = false)]]
SEXP cpp_mk_fixedpoint(SEXP ctx_) {
  CtxPtr ctx = get_ctx(ctx_);
  FixedpointHolder* h = new FixedpointHolder();
  std::unique_ptr<FixedpointHolder> guard(h);
  h->f = own(ctx, Z3_mk_fixedpoint(ctx->c));
  guard.release();
  return make_xptr(h, Kind::Fixedpoint);
}

// [[Rcpp::export(rng = false)]]
void cpp_fixedpoint_register(SEXP f_, SEXP decl_) {
  FixedpointHolder* h = fixedpoint(f_);
  const Ast& d = get_decl(decl_);
  same_ctx(h->f.ctx(), d.ctx());
  Z3_fixedpoint_register_relation(h->f.c(), h->f, as_decl(d));
  check(h->f.ctx());
  ++h->generation;
}

// Adds a rule (a Horn clause, possibly universally quantified) or a fact.
// [[Rcpp::export(rng = false)]]
void cpp_fixedpoint_add_rule(SEXP f_, SEXP rule_, std::string name, bool update) {
  FixedpointHolder* h = fixedpoint(f_);
  const CtxPtr& ctx = h->f.ctx();
  Ast rule = lift(ctx, rule_, Z3_mk_bool_sort(ctx->c));
  if (!is_bool(rule)) fail("rules must be Boolean");
  Z3_symbol sym = name.empty() ? nullptr : mk_symbol(ctx, name);
  if (update) {
    Z3_fixedpoint_update_rule(ctx->c, h->f, rule, sym);
  } else {
    Z3_fixedpoint_add_rule(ctx->c, h->f, rule, sym);
  }
  check(ctx);
  ++h->generation;
}

// Fact for a relation over finite domains, as unsigned element indices.
// [[Rcpp::export(rng = false)]]
void cpp_fixedpoint_add_fact(SEXP f_, SEXP decl_, Rcpp::NumericVector args) {
  FixedpointHolder* h = fixedpoint(f_);
  const Ast& d = get_decl(decl_);
  same_ctx(h->f.ctx(), d.ctx());
  std::vector<unsigned> a(args.size());
  for (R_xlen_t i = 0; i < args.size(); ++i) {
    if (ISNAN(args[i]) || args[i] < 0 || args[i] != std::floor(args[i]) || args[i] > 4294967295.0) {
      fail("fact arguments must be non-negative whole numbers");
    }
    a[i] = static_cast<unsigned>(args[i]);
  }
  Z3_fixedpoint_add_fact(h->f.c(), h->f, as_decl(d), static_cast<unsigned>(a.size()), a.empty() ? nullptr : a.data());
  check(h->f.ctx());
  ++h->generation;
}

// [[Rcpp::export(rng = false)]]
void cpp_fixedpoint_assert(SEXP f_, SEXP axiom_) {
  FixedpointHolder* h = fixedpoint(f_);
  const CtxPtr& ctx = h->f.ctx();
  Ast a = lift(ctx, axiom_, Z3_mk_bool_sort(ctx->c));
  if (!is_bool(a)) fail("background axioms must be Boolean");
  Z3_fixedpoint_assert(ctx->c, h->f, a);
  check(ctx);
  ++h->generation;
}

// Query: a formula, or a list of relation declarations. A level >= 0 starts
// the search at that level (Z3_fixedpoint_query_from_lvl).
// [[Rcpp::export(rng = false)]]
std::string cpp_fixedpoint_query(SEXP f_, SEXP query, int level) {
  FixedpointHolder* h = fixedpoint(f_);
  const CtxPtr& ctx = h->f.ctx();
  Z3_context c = ctx->c;
  Z3_lbool st;
  if (level >= 0 && TYPEOF(query) == VECSXP) fail("'level' needs a query formula, not relations");
  if (TYPEOF(query) == VECSXP) {
    std::vector<Z3_func_decl> rels;
    AstVec keep;
    for (R_xlen_t i = 0; i < Rf_xlength(query); ++i) {
      const Ast& d = get_decl(VECTOR_ELT(query, i));
      same_ctx(ctx, d.ctx());
      keep.push_back(d);
      rels.push_back(as_decl(d));
    }
    BusyGuard<FixedpointHolder> busy(h);
    for (CallbackPtr& cb : h->callbacks) cb->results.clear();
    st = Z3_fixedpoint_query_relations(c, h->f, static_cast<unsigned>(rels.size()), rels.data());
  } else {
    Ast q = lift(ctx, query, Z3_mk_bool_sort(c));
    if (!is_bool(q)) fail("the query must be Boolean");
    // Lemma and external relation callbacks run during the query.
    BusyGuard<FixedpointHolder> busy(h);
    for (CallbackPtr& cb : h->callbacks) cb->results.clear();
    st = level < 0 ? Z3_fixedpoint_query(c, h->f, q)
                   : Z3_fixedpoint_query_from_lvl(c, h->f, q, static_cast<unsigned>(level));
  }
  h->checked = true;
  h->checked_at = h->generation;
  h->status = Z3_L_UNDEF;
  raise_callback_errors(h->callbacks);
  check(ctx);
  h->status = st;
  if (st == Z3_L_UNDEF) forward_interrupt(st, str(Z3_fixedpoint_get_reason_unknown(c, h->f)));
  return lbool_string(st);
}

// Answer of the last query: derivation (sat) or inductive invariant (unsat).
// [[Rcpp::export(rng = false)]]
SEXP cpp_fixedpoint_answer(SEXP f_, std::string which) {
  FixedpointHolder* h = fixedpoint(f_);
  require_status(h, "an answer");
  if (h->status == Z3_L_UNDEF) fail("there is no answer: the last z3_query() returned \"unknown\"");
  if (which != "answer" && h->status != Z3_L_TRUE) {
    fail("ground answers and traces exist only after z3_query() returns \"sat\"");
  }
  const CtxPtr& ctx = h->f.ctx();
  Z3_context c = ctx->c;
  if (which == "answer") return wrap_expr(own_ast(ctx, Z3_fixedpoint_get_answer(c, h->f)));
  if (which == "ground") return wrap_expr(own_ast(ctx, Z3_fixedpoint_get_ground_sat_answer(c, h->f)));
  if (which == "trace") return wrap_ast_vector(ctx, Z3_fixedpoint_get_rules_along_trace(c, h->f));
  // Rule names along the trace, as a symbol joined by ';'.
  Z3_symbol names = Z3_fixedpoint_get_rule_names_along_trace(c, h->f);
  check(ctx);
  return Rcpp::wrap(symbol_string(ctx, names));
}

// [[Rcpp::export(rng = false)]]
std::string cpp_fixedpoint_reason_unknown(SEXP f_) {
  FixedpointHolder* h = fixedpoint(f_);
  std::string r = str(Z3_fixedpoint_get_reason_unknown(h->f.c(), h->f));
  check(h->f.ctx());
  return r;
}

// Property of a predicate at a level of the PDR/Spacer search; level -1 is
// the inductive invariant.
// [[Rcpp::export(rng = false)]]
SEXP cpp_fixedpoint_cover(SEXP f_, int level, SEXP decl_, SEXP property, bool add) {
  FixedpointHolder* h = fixedpoint(f_);
  const CtxPtr& ctx = h->f.ctx();
  const Ast& d = get_decl(decl_);
  same_ctx(ctx, d.ctx());
  if (level < -1) fail("level must be -1 or a whole number of at least 0");
  if (add) {
    Ast p = lift(ctx, property, Z3_mk_bool_sort(ctx->c));
    if (level >= 0) {
      // Spacer creates every missing level up to the one given, so a huge
      // level would exhaust memory. A predicate has no levels before the
      // first query, so small levels are always allowed.
      unsigned n = std::max(Z3_fixedpoint_get_num_levels(ctx->c, h->f, as_decl(d)), 1024u);
      check(ctx);
      if (static_cast<unsigned>(level) > n) {
        fail("level must be -1 or at most " + std::to_string(n) +
             " (1024, or z3_num_levels() when larger)");
      }
    }
    Z3_fixedpoint_add_cover(ctx->c, h->f, level, as_decl(d), p);
    check(ctx);
    ++h->generation;
    return R_NilValue;
  }
  return wrap_expr(own_ast(ctx, Z3_fixedpoint_get_cover_delta(ctx->c, h->f, level, as_decl(d))));
}

// [[Rcpp::export(rng = false)]]
int cpp_fixedpoint_num_levels(SEXP f_, SEXP decl_) {
  FixedpointHolder* h = fixedpoint(f_);
  const Ast& d = get_decl(decl_);
  same_ctx(h->f.ctx(), d.ctx());
  unsigned n = Z3_fixedpoint_get_num_levels(h->f.c(), h->f, as_decl(d));
  check(h->f.ctx());
  return static_cast<int>(n);
}

// Spacer: add a known invariant, or read the reachable states of a predicate.
// [[Rcpp::export(rng = false)]]
SEXP cpp_fixedpoint_spacer(SEXP f_, std::string what, SEXP decl_, SEXP property) {
  FixedpointHolder* h = fixedpoint(f_);
  const CtxPtr& ctx = h->f.ctx();
  const Ast& d = get_decl(decl_);
  same_ctx(ctx, d.ctx());
  if (what == "invariant") {
    Ast p = lift(ctx, property, Z3_mk_bool_sort(ctx->c));
    Z3_fixedpoint_add_invariant(ctx->c, h->f, as_decl(d), p);
    check(ctx);
    ++h->generation;
    return R_NilValue;
  }
  return wrap_expr(own_ast(ctx, Z3_fixedpoint_get_reachable(ctx->c, h->f, as_decl(d))));
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_fixedpoint_contents(SEXP f_, std::string which) {
  FixedpointHolder* h = fixedpoint(f_);
  const CtxPtr& ctx = h->f.ctx();
  if (which == "rules") return wrap_ast_vector(ctx, Z3_fixedpoint_get_rules(ctx->c, h->f));
  return wrap_ast_vector(ctx, Z3_fixedpoint_get_assertions(ctx->c, h->f));
}

// [[Rcpp::export(rng = false)]]
void cpp_fixedpoint_representation(SEXP f_, SEXP decl_, Rcpp::CharacterVector kinds) {
  FixedpointHolder* h = fixedpoint(f_);
  const CtxPtr& ctx = h->f.ctx();
  const Ast& d = get_decl(decl_);
  same_ctx(ctx, d.ctx());
  std::vector<Z3_symbol> syms;
  for (R_xlen_t i = 0; i < kinds.size(); ++i) syms.push_back(mk_symbol(ctx, Rcpp::as<std::string>(kinds[i])));
  Z3_fixedpoint_set_predicate_representation(ctx->c, h->f, as_decl(d), static_cast<unsigned>(syms.size()),
                                             syms.empty() ? nullptr : syms.data());
  check(ctx);
}

// [[Rcpp::export(rng = false)]]
void cpp_fixedpoint_set_params(SEXP f_, SEXP params) {
  FixedpointHolder* h = fixedpoint(f_);
  const CtxPtr& ctx = h->f.ctx();
  Ref<Z3_param_descrs> d = own(ctx, Z3_fixedpoint_get_param_descrs(ctx->c, h->f));
  Ref<Z3_params> p = make_params(ctx, d, params);
  Z3_fixedpoint_set_params(ctx->c, h->f, p);
  check(ctx);
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_fixedpoint_param_help(SEXP f_) {
  FixedpointHolder* h = fixedpoint(f_);
  return describe_params(h->f.ctx(), Z3_fixedpoint_get_param_descrs(h->f.c(), h->f));
}

// [[Rcpp::export(rng = false)]]
std::string cpp_fixedpoint_help(SEXP f_) {
  FixedpointHolder* h = fixedpoint(f_);
  std::string out = str(Z3_fixedpoint_get_help(h->f.c(), h->f));
  check(h->f.ctx());
  return out;
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_fixedpoint_statistics(SEXP f_) {
  FixedpointHolder* h = fixedpoint(f_);
  return stats_list(h->f.ctx(), Z3_fixedpoint_get_statistics(h->f.c(), h->f));
}

// [[Rcpp::export(rng = false)]]
Rcpp::String cpp_fixedpoint_to_string(SEXP f_, Rcpp::List queries) {
  FixedpointHolder* h = fixedpoint(f_);
  const CtxPtr& ctx = h->f.ctx();
  AstVec q = expr_list(ctx, queries, "queries");
  std::vector<Z3_ast> r = raw(q);
  std::string out = str(Z3_fixedpoint_to_string(ctx->c, h->f, static_cast<unsigned>(r.size()),
                                                r.empty() ? nullptr : r.data()));
  check(ctx);
  return out;
}

// Parses rules and facts; returns the queries found in the text.
// [[Rcpp::export(rng = false)]]
SEXP cpp_fixedpoint_from(SEXP f_, SEXP text_, bool is_file) {
  FixedpointHolder* h = fixedpoint(f_);
  const CtxPtr& ctx = h->f.ctx();
  std::string text = as_string(text_, is_file ? "file" : "text");
  Z3_ast_vector v = is_file ? Z3_fixedpoint_from_file(ctx->c, h->f, text.c_str())
                            : Z3_fixedpoint_from_string(ctx->c, h->f, text.c_str());
  ++h->generation;
  return wrap_ast_vector(ctx, v);
}

// Column sorts of a relation sort: the sort of the relations that external
// relation callbacks receive.
// [[Rcpp::export(rng = false)]]
SEXP cpp_relation_columns(SEXP sort_) {
  const Ast& s = get_sort(sort_);
  const CtxPtr& ctx = s.ctx();
  Z3_sort_kind k = Z3_get_sort_kind(ctx->c, as_sort(s));
  check(ctx);
  if (k != Z3_RELATION_SORT) fail("expected a relation sort");
  unsigned n = Z3_get_relation_arity(ctx->c, as_sort(s));
  check(ctx);
  Rcpp::List out(n);
  for (unsigned i = 0; i < n; ++i) {
    out[i] = wrap_sort(own_ast(ctx, sort_ast(Z3_get_relation_column(ctx->c, as_sort(s), i))));
  }
  return out;
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_fixedpoint_last_status(SEXP f_) {
  FixedpointHolder* h = fixedpoint(f_);
  if (!h->checked || h->checked_at != h->generation) return Rcpp::CharacterVector::create(NA_STRING);
  return Rcpp::CharacterVector::create(lbool_string(h->status));
}
