// Solvers: assertion, checking, models, cores, parameters and statistics.
#include "z3r.h"

#include <cmath>

namespace z3r {

void forward_interrupt(Z3_lbool status, const std::string& reason) {
  if (status == Z3_L_UNDEF && reason == "interrupted from keyboard") {
    throw Rcpp::internal::InterruptedException();
  }
}

const char* lbool_string(Z3_lbool r) {
  return r == Z3_L_TRUE ? "sat" : r == Z3_L_FALSE ? "unsat" : "unknown";
}

bool has_propagator(const CallbackPtr& cb) {
  SEXP names = Rf_getAttrib(cb->fns, R_NamesSymbol);
  for (R_xlen_t i = 0; i < Rf_xlength(cb->fns); ++i) {
    if (std::string(CHAR(STRING_ELT(names, i))) == "push") return true;
  }
  return false;
}

// Z3 copies solvers without their R callbacks; a copy of a solver with a
// user propagator or clause callback would silently lose them.
void refuse_copy_with_callbacks(SolverHolder* h, const char* what) {
  if (!h->callbacks.empty()) {
    fail(std::string(what) + " cannot copy a solver with R callbacks (a user propagator or "
         "z3_on_clause()); Z3 would copy it without them");
  }
}

// Reads a named R list of parameter values, using the parameter
// descriptions of the target object to choose each value's type.
static KeptParams read_param_values(const CtxPtr& ctx, Z3_param_descrs descrs, SEXP values) {
  Z3_context c = ctx->c;
  KeptParams out;
  Rcpp::List lst(values);
  if (lst.size() == 0) return out;
  Rcpp::CharacterVector names = lst.names();
  for (R_xlen_t i = 0; i < lst.size(); ++i) {
    std::string name = Rcpp::as<std::string>(names[i]);
    if (name.empty()) fail("all parameters must be named");
    Z3_param_kind kind = Z3_param_descrs_get_kind(c, descrs, mk_symbol(ctx, name));
    check(ctx);
    SEXP v = lst[i];
    if (Rf_xlength(v) != 1) fail("parameter '" + name + "' must be a single value");
    KeptParam kp{name, kind, 0, std::string()};
    switch (kind) {
      case Z3_PK_BOOL: {
        if (TYPEOF(v) != LGLSXP || LOGICAL(v)[0] == NA_LOGICAL) {
          fail("parameter '" + name + "' must be TRUE or FALSE");
        }
        kp.number = LOGICAL(v)[0] != 0;
        break;
      }
      case Z3_PK_UINT: {
        bool num = TYPEOF(v) == INTSXP || TYPEOF(v) == REALSXP;
        double d = num ? Rf_asReal(v) : NA_REAL;
        if (!num || ISNAN(d) || d < 0 ||
            d != std::floor(d) || d > 4294967295.0) {
          fail("parameter '" + name + "' must be a non-negative whole number");
        }
        kp.number = d;
        break;
      }
      case Z3_PK_DOUBLE: {
        bool num = TYPEOF(v) == INTSXP || TYPEOF(v) == REALSXP;
        double d = num ? Rf_asReal(v) : NA_REAL;
        if (!num || ISNAN(d)) {
          fail("parameter '" + name + "' must be a number");
        }
        kp.number = d;
        break;
      }
      case Z3_PK_SYMBOL:
      case Z3_PK_STRING: {
        if (TYPEOF(v) != STRSXP || STRING_ELT(v, 0) == NA_STRING) {
          fail("parameter '" + name + "' must be a string");
        }
        kp.text = CHAR(STRING_ELT(v, 0));
        break;
      }
      default:
        fail("unknown parameter '" + name + "'; z3_param_help() lists the "
             "parameters this object accepts");
    }
    out.push_back(kp);
  }
  return out;
}

static void set_param(const CtxPtr& ctx, Z3_params p, const KeptParam& kp) {
  Z3_context c = ctx->c;
  Z3_symbol sym = mk_symbol(ctx, kp.name);
  switch (kp.kind) {
    case Z3_PK_BOOL: Z3_params_set_bool(c, p, sym, kp.number != 0); break;
    case Z3_PK_UINT: Z3_params_set_uint(c, p, sym, static_cast<unsigned>(kp.number)); break;
    case Z3_PK_DOUBLE: Z3_params_set_double(c, p, sym, kp.number); break;
    default: Z3_params_set_symbol(c, p, sym, mk_symbol(ctx, kp.text)); break;
  }
  check(ctx);
}

Ref<Z3_params> kept_param_set(const CtxPtr& ctx, const KeptParams& kept) {
  Ref<Z3_params> p = own(ctx, Z3_mk_params(ctx->c));
  for (const KeptParam& kp : kept) set_param(ctx, p, kp);
  return p;
}

// Builds a Z3 parameter set from a named R list.
Ref<Z3_params> make_params(const CtxPtr& ctx, Z3_param_descrs descrs, SEXP values) {
  Ref<Z3_params> p = kept_param_set(ctx, read_param_values(ctx, descrs, values));
  if (Rf_xlength(values) == 0) return p;
  Z3_params_validate(ctx->c, p, descrs);
  check(ctx);
  return p;
}

static void put_param(KeptParams& kept, const KeptParam& kp) {
  for (KeptParam& old : kept) {
    if (old.name == kp.name) {
      old = kp;
      return;
    }
  }
  kept.push_back(kp);
}

KeptParams with_params(const CtxPtr& ctx, Z3_param_descrs descrs, SEXP values, const KeptParams& kept) {
  KeptParams out = kept;
  for (const KeptParam& kp : read_param_values(ctx, descrs, values)) put_param(out, kp);
  return out;
}

Ref<Z3_params> restored_params(const CtxPtr& ctx, KeptParams& kept) {
  bool has_timeout = false;
  for (const KeptParam& kp : kept) has_timeout = has_timeout || kp.name == "timeout";
  if (!has_timeout) {
    kept.push_back(KeptParam{"timeout", Z3_PK_UINT, static_cast<double>(ctx->timeout), std::string()});
  }
  return kept_param_set(ctx, kept);
}

// The kept set with a timeout for one check.
Ref<Z3_params> params_with_timeout(const CtxPtr& ctx, const KeptParams& kept, unsigned ms) {
  KeptParams once = kept;
  put_param(once, KeptParam{"timeout", Z3_PK_UINT, static_cast<double>(ms), std::string()});
  return kept_param_set(ctx, once);
}

SEXP describe_params(const CtxPtr& ctx, Z3_param_descrs raw_descrs) {
  Z3_context c = ctx->c;
  Ref<Z3_param_descrs> d = own(ctx, raw_descrs);
  unsigned n = Z3_param_descrs_size(c, d);
  Rcpp::CharacterVector name(n), kind(n), doc(n);
  for (unsigned i = 0; i < n; ++i) {
    Z3_symbol s = Z3_param_descrs_get_name(c, d, i);
    name[i] = symbol_string(ctx, s);
    switch (Z3_param_descrs_get_kind(c, d, s)) {
      case Z3_PK_BOOL: kind[i] = "bool"; break;
      case Z3_PK_UINT: kind[i] = "uint"; break;
      case Z3_PK_DOUBLE: kind[i] = "double"; break;
      case Z3_PK_SYMBOL: kind[i] = "symbol"; break;
      case Z3_PK_STRING: kind[i] = "string"; break;
      default: kind[i] = "other";
    }
    doc[i] = str(Z3_param_descrs_get_documentation(c, d, s));
    check(ctx);
  }
  return Rcpp::DataFrame::create(Rcpp::Named("name") = name, Rcpp::Named("kind") = kind,
                                 Rcpp::Named("description") = doc,
                                 Rcpp::Named("stringsAsFactors") = false);
}

SEXP stats_list(const CtxPtr& ctx, Z3_stats raw_stats) {
  Z3_context c = ctx->c;
  Ref<Z3_stats> st = own(ctx, raw_stats);
  unsigned n = Z3_stats_size(c, st);
  Rcpp::List out(n);
  Rcpp::CharacterVector keys(n);
  for (unsigned i = 0; i < n; ++i) {
    keys[i] = str(Z3_stats_get_key(c, st, i));
    if (Z3_stats_is_uint(c, st, i)) {
      out[i] = static_cast<double>(Z3_stats_get_uint_value(c, st, i));
    } else if (Z3_stats_is_double(c, st, i)) {
      out[i] = Z3_stats_get_double_value(c, st, i);
    } else {
      out[i] = NA_REAL;  // a kind of value this version of z3r does not know
    }
    check(ctx);
  }
  out.names() = keys;
  return out;
}

// Statistics as Z3 writes them.
static std::string stats_text(const CtxPtr& ctx, Z3_stats raw_stats) {
  Ref<Z3_stats> st = own(ctx, raw_stats);
  std::string out = str(Z3_stats_to_string(ctx->c, st));
  check(ctx);
  return out;
}

}  // namespace z3r

using namespace z3r;

static SolverHolder* solver(SEXP x) { return solver_of(x); }

// Gives a solver a timeout for one check. Z3 cannot remove a parameter, so
// the destructor applies the kept set again with the timeout z3r set before
// (or else the context's timeout, which the kept set then records), also
// when the check fails. apply() runs after construction so that the
// destructor runs even if it throws.
struct SolverTimeout {
  SolverHolder* h = nullptr;
  void apply(SolverHolder* holder, SEXP ms) {
    if (Rf_isNull(ms)) return;
    int v = Rf_asInteger(ms);
    if (v == NA_INTEGER || v < 1) fail("timeout must be a positive whole number of milliseconds");
    const CtxPtr& ctx = holder->s.ctx();
    Ref<Z3_params> p = params_with_timeout(ctx, holder->params, static_cast<unsigned>(v));
    h = holder;
    Z3_solver_set_params(ctx->c, h->s, p);
    check(ctx);
  }
  ~SolverTimeout() {
    if (!h) return;
    Z3_context c = h->s.c();
    try {
      Ref<Z3_params> p = restored_params(h->s.ctx(), h->params);
      Z3_solver_set_params(c, h->s, p);
    } catch (...) {
    }
    Z3_set_error(c, Z3_OK);  // a destructor cannot raise it; the set was accepted before
  }
};

static SEXP wrap_solver(const CtxPtr& ctx, Z3_solver s) {
  Ref<Z3_solver> ref = own(ctx, s);  // checks for an error before allocating
  SolverHolder* h = new SolverHolder();
  h->s = std::move(ref);
  return make_xptr(h, Kind::Solver);
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_mk_solver(SEXP ctx_, SEXP logic_) {
  CtxPtr ctx = get_ctx(ctx_);
  if (Rf_isNull(logic_)) return wrap_solver(ctx, Z3_mk_solver(ctx->c));
  std::string logic = as_string(logic_, "logic");
  return wrap_solver(ctx, Z3_mk_solver_for_logic(ctx->c, mk_symbol(ctx, logic)));
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_mk_simple_solver(SEXP ctx_) {
  CtxPtr ctx = get_ctx(ctx_);
  return wrap_solver(ctx, Z3_mk_simple_solver(ctx->c));
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_mk_solver_from_tactic(SEXP tactic_) {
  const Ref<Z3_tactic>& t = *get_xptr<Ref<Z3_tactic>>(tactic_, Kind::Tactic);
  return wrap_solver(t.ctx(), Z3_mk_solver_from_tactic(t.c(), t));
}

// [[Rcpp::export(rng = false)]]
void cpp_solver_assert(SEXP s_, Rcpp::List constraints) {
  SolverHolder* h = solver(s_);
  const CtxPtr& ctx = h->s.ctx();
  AstVec v;
  for (R_xlen_t i = 0; i < constraints.size(); ++i) {
    Ast a = lift(ctx, constraints[i], Z3_mk_bool_sort(ctx->c));
    if (!is_bool(a)) {
      fail("constraint " + std::to_string(static_cast<long long>(i + 1)) +
           " is not Boolean (it has sort " + sort_name(ctx, Z3_get_sort(ctx->c, a)) + ")");
    }
    v.push_back(a);
  }
  // Validate everything before asserting anything. The solver counts as
  // changed even if Z3 fails partway.
  h->touch();
  {
    // Asserting after a check backtracks Z3's search, which runs pop
    // callbacks.
    BusyGuard<SolverHolder> busy(h);
    for (const Ast& a : v) {
      Z3_solver_assert(ctx->c, h->s, a);
      check(ctx);
    }
  }
  raise_callback_errors(h->callbacks);
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_solver_assert_and_track(SEXP s_, SEXP constraint, SEXP label_) {
  SolverHolder* h = solver(s_);
  const CtxPtr& ctx = h->s.ctx();
  Ast a = lift(ctx, constraint, Z3_mk_bool_sort(ctx->c));
  if (!is_bool(a)) fail("tracked constraints must be Boolean");
  Ast label;
  if (is_kind(label_, Kind::Expr)) {
    label = get_expr(label_);
    same_ctx(ctx, label.ctx());
  } else {
    std::string name = as_string(label_, "name");
    label = own_ast(ctx, Z3_mk_const(ctx->c, mk_symbol(ctx, name), Z3_mk_bool_sort(ctx->c)));
  }
  if (!is_bool(label)) fail("the tracking label must be a Boolean constant");
  h->touch();
  {
    BusyGuard<SolverHolder> busy(h);
    Z3_solver_assert_and_track(ctx->c, h->s, a, label);
  }
  raise_callback_errors(h->callbacks);
  check(ctx);
  return wrap_expr(label);
}

// [[Rcpp::export(rng = false)]]
std::string cpp_solver_check(SEXP s_, Rcpp::List assumptions, SEXP timeout) {
  SolverHolder* h = solver(s_);
  const CtxPtr& ctx = h->s.ctx();
  AstVec as;
  for (R_xlen_t i = 0; i < assumptions.size(); ++i) {
    Ast a = lift(ctx, assumptions[i], Z3_mk_bool_sort(ctx->c));
    if (!is_bool(a)) fail("assumptions must be Boolean");
    as.push_back(a);
  }
  std::vector<Z3_ast> r = raw(as);
  SolverTimeout once;
  once.apply(h, timeout);
  for (CallbackPtr& cb : h->callbacks) cb->cloned = false;
  Z3_lbool st;
  {
    BusyGuard<SolverHolder> busy(h);
    st = r.empty() ? Z3_solver_check(ctx->c, h->s)
                   : Z3_solver_check_assumptions(ctx->c, h->s, static_cast<unsigned>(r.size()), r.data());
  }
  h->checked = true;
  h->checked_at = h->generation;
  h->status = Z3_L_UNDEF;
  raise_callback_errors(h->callbacks);
  check(ctx);
  // A copy made by Z3 during the check runs no R callbacks, so a "sat"
  // found in it may violate constraints a propagator enforces. ("unsat"
  // stays sound: the copy has fewer constraints.)
  if (st == Z3_L_TRUE) {
    for (CallbackPtr& cb : h->callbacks) {
      if (cb->cloned && has_propagator(cb)) {
        fail("Z3 copied the solver during this check (for example in parallel mode), and the copy "
             "does not run R callbacks; the \"sat\" result may violate constraints of the user "
             "propagator, so it is refused");
      }
    }
  }
  h->status = st;
  if (st == Z3_L_UNDEF) forward_interrupt(st, str(Z3_solver_get_reason_unknown(ctx->c, h->s)));
  return lbool_string(st);
}

// Last check result, or NA when the solver changed since then.
// [[Rcpp::export(rng = false)]]
SEXP cpp_solver_last_status(SEXP s_) {
  SolverHolder* h = solver(s_);
  if (!h->checked || h->checked_at != h->generation) return Rcpp::CharacterVector::create(NA_STRING);
  return Rcpp::CharacterVector::create(lbool_string(h->status));
}

static void require_status(SolverHolder* h, Z3_lbool want, const char* what) {
  if (!h->checked) fail(std::string("call z3_check() before requesting ") + what);
  if (h->checked_at != h->generation) {
    fail(std::string("the solver has changed since the last z3_check(); call "
                     "z3_check() again before requesting ") + what);
  }
  if (h->status != want) {
    fail(std::string(what) + " is only available after z3_check() returns \"" +
         lbool_string(want) + "\" (the last result was \"" + lbool_string(h->status) + "\")");
  }
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_solver_model(SEXP s_) {
  SolverHolder* h = solver(s_);
  require_status(h, Z3_L_TRUE, "a model");
  const CtxPtr& ctx = h->s.ctx();
  return make_xptr(new Ref<Z3_model>(own(ctx, Z3_solver_get_model(ctx->c, h->s))), Kind::Model);
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_solver_unsat_core(SEXP s_) {
  SolverHolder* h = solver(s_);
  require_status(h, Z3_L_FALSE, "an unsatisfiable core");
  const CtxPtr& ctx = h->s.ctx();
  // Core minimization may search again, running callbacks.
  Z3_ast_vector core;
  {
    BusyGuard<SolverHolder> busy(h);
    core = Z3_solver_get_unsat_core(ctx->c, h->s);
  }
  raise_callback_errors(h->callbacks);
  return wrap_ast_vector(ctx, core);
}

// [[Rcpp::export(rng = false)]]
std::string cpp_solver_reason_unknown(SEXP s_) {
  SolverHolder* h = solver(s_);
  const CtxPtr& ctx = h->s.ctx();
  std::string r = str(Z3_solver_get_reason_unknown(ctx->c, h->s));
  check(ctx);
  return r;
}

// [[Rcpp::export(rng = false)]]
void cpp_solver_push(SEXP s_) {
  SolverHolder* h = solver(s_);
  {
    BusyGuard<SolverHolder> busy(h);
    Z3_solver_push(h->s.c(), h->s);
  }
  h->touch();
  raise_callback_errors(h->callbacks);
  check(h->s.ctx());
}

// [[Rcpp::export(rng = false)]]
void cpp_solver_pop(SEXP s_, int n) {
  SolverHolder* h = solver(s_);
  unsigned scopes = Z3_solver_get_num_scopes(h->s.c(), h->s);
  if (n < 0) fail("n must be a non-negative integer");
  if (static_cast<unsigned>(n) > scopes) {
    fail("cannot pop " + std::to_string(n) + " scope(s); the solver has " +
         std::to_string(scopes) + " open scope(s)");
  }
  {
    BusyGuard<SolverHolder> busy(h);
    Z3_solver_pop(h->s.c(), h->s, static_cast<unsigned>(n));
  }
  h->touch();
  raise_callback_errors(h->callbacks);
  check(h->s.ctx());
}

// [[Rcpp::export(rng = false)]]
int cpp_solver_num_scopes(SEXP s_) {
  SolverHolder* h = solver(s_);
  unsigned n = Z3_solver_get_num_scopes(h->s.c(), h->s);
  check(h->s.ctx());
  return static_cast<int>(n);
}

// [[Rcpp::export(rng = false)]]
void cpp_solver_reset(SEXP s_) {
  SolverHolder* h = solver(s_);
  Z3_solver_reset(h->s.c(), h->s);
  check(h->s.ctx());
  h->touch();
  h->checked = false;
  // Resetting discards Z3's solver, including user propagators and clause
  // callbacks; drop their state so they can be installed again.
  h->callbacks.clear();
  drop_callbacks(s_);
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_solver_assertions(SEXP s_) {
  SolverHolder* h = solver(s_);
  return wrap_ast_vector(h->s.ctx(), Z3_solver_get_assertions(h->s.c(), h->s));
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_solver_statistics(SEXP s_) {
  SolverHolder* h = solver(s_);
  return stats_list(h->s.ctx(), Z3_solver_get_statistics(h->s.c(), h->s));
}

// Statistics of a solver, optimizer or fixedpoint engine as Z3 writes them.
// [[Rcpp::export(rng = false)]]
Rcpp::String cpp_statistics_text(SEXP x) {
  if (is_kind(x, Kind::Optimize)) {
    OptimizeHolder* h = optimizer_of(x);
    return stats_text(h->o.ctx(), Z3_optimize_get_statistics(h->o.c(), h->o));
  }
  if (is_kind(x, Kind::Fixedpoint)) {
    FixedpointHolder* h = fixedpoint_of(x);
    return stats_text(h->f.ctx(), Z3_fixedpoint_get_statistics(h->f.c(), h->f));
  }
  SolverHolder* h = solver(x);
  return stats_text(h->s.ctx(), Z3_solver_get_statistics(h->s.c(), h->s));
}

// Stops the check running on this solver. Unlike every other entry point it
// accepts a busy solver: it is meant for the solver's own callbacks. Z3
// ignores it when no check runs (the solver then has no event handler).
// [[Rcpp::export(rng = false)]]
void cpp_solver_interrupt(SEXP s_) {
  SolverHolder* h = get_xptr<SolverHolder>(s_, Kind::Solver);
  Z3_solver_interrupt(h->s.c(), h->s);
}

// [[Rcpp::export(rng = false)]]
void cpp_solver_set_params(SEXP s_, SEXP params) {
  SolverHolder* h = solver(s_);
  const CtxPtr& ctx = h->s.ctx();
  Ref<Z3_param_descrs> d = own(ctx, Z3_solver_get_param_descrs(ctx->c, h->s));
  make_params(ctx, d, params);  // validates the new values
  // Some solvers (the combined solver's solver2_timeout, ignore_solver1,
  // solver2_unknown) reset the options a set omits, so apply the whole kept
  // set.
  KeptParams kept = with_params(ctx, d, params, h->params);
  Ref<Z3_params> p = kept_param_set(ctx, kept);
  Z3_solver_set_params(ctx->c, h->s, p);
  check(ctx);
  h->params = std::move(kept);
}

// Parameters z3r has applied to a solver or optimizer, as Z3 writes them
// ("" if none).
// [[Rcpp::export(rng = false)]]
Rcpp::String cpp_kept_params(SEXP x) {
  bool opt = is_kind(x, Kind::Optimize);
  const CtxPtr& ctx = opt ? optimizer_of(x)->o.ctx() : solver(x)->s.ctx();
  const KeptParams& kept = opt ? optimizer_of(x)->params : solver(x)->params;
  if (kept.empty()) return Rcpp::String("");
  Ref<Z3_params> p = kept_param_set(ctx, kept);
  std::string out = str(Z3_params_to_string(ctx->c, p));
  check(ctx);
  return out;
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_solver_param_help(SEXP s_) {
  SolverHolder* h = solver(s_);
  return describe_params(h->s.ctx(), Z3_solver_get_param_descrs(h->s.c(), h->s));
}

// [[Rcpp::export(rng = false)]]
Rcpp::String cpp_solver_to_string(SEXP s_) {
  SolverHolder* h = solver(s_);
  std::string out = str(Z3_solver_to_string(h->s.c(), h->s));
  check(h->s.ctx());
  return out;
}

// Parses SMT-LIB2 text and asserts its assertions into the solver.
// [[Rcpp::export(rng = false)]]
void cpp_solver_from_string(SEXP s_, SEXP text_) {
  SolverHolder* h = solver(s_);
  std::string text = as_string(text_, "text");
  h->touch();
  {
    BusyGuard<SolverHolder> busy(h);
    Z3_solver_from_string(h->s.c(), h->s, text.c_str());
  }
  raise_callback_errors(h->callbacks);
  check(h->s.ctx());
}

// ---------------------------------------------------------------------------
// Proofs, consequences, cubes and solver internals

// [[Rcpp::export(rng = false)]]
SEXP cpp_solver_proof(SEXP s_) {
  SolverHolder* h = solver(s_);
  if (!h->checked || h->checked_at != h->generation || h->status != Z3_L_FALSE) {
    fail("a proof is only available after z3_check() returns \"unsat\" and before the solver changes");
  }
  const CtxPtr& ctx = h->s.ctx();
  Z3_ast p = Z3_solver_get_proof(ctx->c, h->s);
  return wrap_expr(own_ast(ctx, p));
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_solver_consequences(SEXP s_, Rcpp::List assumptions, Rcpp::List variables) {
  SolverHolder* h = solver(s_);
  const CtxPtr& ctx = h->s.ctx();
  AstVec as = expr_list(ctx, assumptions, "assumptions");
  for (const Ast& a : as) if (!is_bool(a)) fail("assumptions must be Boolean");
  AstVec vs = expr_list(ctx, variables, "variables");
  Ref<Z3_ast_vector> va = make_ast_vector(ctx, as);
  Ref<Z3_ast_vector> vv = make_ast_vector(ctx, vs);
  Ref<Z3_ast_vector> out = own(ctx, Z3_mk_ast_vector(ctx->c));
  Z3_lbool st;
  {
    BusyGuard<SolverHolder> busy(h);
    st = Z3_solver_get_consequences(ctx->c, h->s, va, vv, out);
  }
  h->checked = true;
  h->checked_at = h->generation;
  h->status = Z3_L_UNDEF;
  raise_callback_errors(h->callbacks);
  check(ctx);
  h->status = st;
  if (st == Z3_L_UNDEF) forward_interrupt(st, str(Z3_solver_get_reason_unknown(ctx->c, h->s)));
  unsigned n = Z3_ast_vector_size(ctx->c, out);
  AstVec cons;
  for (unsigned i = 0; i < n; ++i) cons.push_back(own_ast(ctx, Z3_ast_vector_get(ctx->c, out, i)));
  return Rcpp::List::create(Rcpp::Named("status") = lbool_string(st),
                            Rcpp::Named("consequences") = wrap_expr_list(cons));
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_solver_cube(SEXP s_, Rcpp::List vars, int backtrack_level) {
  SolverHolder* h = solver(s_);
  const CtxPtr& ctx = h->s.ctx();
  AstVec vs = expr_list(ctx, vars, "variables");
  Ref<Z3_ast_vector> vv = make_ast_vector(ctx, vs);
  ensure_solver_ready(ctx->c, h->s);
  check(ctx);
  Z3_ast_vector cube;
  {
    BusyGuard<SolverHolder> busy(h);
    cube = Z3_solver_cube(ctx->c, h->s, vv, static_cast<unsigned>(backtrack_level));
  }
  // Cubing searches and replaces the solver's last result.
  h->checked = false;
  raise_callback_errors(h->callbacks);
  return wrap_ast_vector(ctx, cube);
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_solver_literals(SEXP s_, std::string which) {
  SolverHolder* h = solver(s_);
  const CtxPtr& ctx = h->s.ctx();
  Z3_context c = ctx->c;
  if (which == "units") return wrap_ast_vector(ctx, Z3_solver_get_units(c, h->s));
  if (which == "non_units") return wrap_ast_vector(ctx, Z3_solver_get_non_units(c, h->s));
  // trail with decision levels
  Ref<Z3_ast_vector> trail = own(ctx, Z3_solver_get_trail(c, h->s));
  unsigned n = Z3_ast_vector_size(c, trail);
  std::vector<unsigned> levels(n);
  if (n) Z3_solver_get_levels(c, h->s, trail, n, levels.data());
  check(ctx);
  AstVec lits;
  for (unsigned i = 0; i < n; ++i) lits.push_back(own_ast(ctx, Z3_ast_vector_get(c, trail, i)));
  Rcpp::IntegerVector lv(n);
  for (unsigned i = 0; i < n; ++i) lv[i] = static_cast<int>(levels[i]);
  return Rcpp::List::create(Rcpp::Named("literals") = wrap_expr_list(lits),
                            Rcpp::Named("levels") = lv);
}

// [[Rcpp::export(rng = false)]]
std::string cpp_solver_help(SEXP s_) {
  SolverHolder* h = solver(s_);
  std::string out = str(Z3_solver_get_help(h->s.c(), h->s));
  check(h->s.ctx());
  return out;
}

// Help text for the parameters of z3_simplify().
// [[Rcpp::export(rng = false)]]
std::string cpp_simplify_help_text(SEXP ctx_) {
  CtxPtr ctx = get_ctx(ctx_);
  std::string out = str(Z3_simplify_get_help(ctx->c));
  check(ctx);
  return out;
}

// Names of the parameters of a solver, optimizer, tactic, simplifier,
// fixedpoint engine or (for a context) z3_simplify(), as Z3 lists them.
// [[Rcpp::export(rng = false)]]
std::string cpp_param_names(SEXP x) {
  CtxPtr ctx;
  Z3_param_descrs d = nullptr;
  if (is_kind(x, Kind::Solver)) {
    SolverHolder* h = solver(x);
    ctx = h->s.ctx();
    d = Z3_solver_get_param_descrs(ctx->c, h->s);
  } else if (is_kind(x, Kind::Optimize)) {
    OptimizeHolder* h = optimizer_of(x);
    ctx = h->o.ctx();
    d = Z3_optimize_get_param_descrs(ctx->c, h->o);
  } else if (is_kind(x, Kind::Fixedpoint)) {
    FixedpointHolder* h = fixedpoint_of(x);
    ctx = h->f.ctx();
    d = Z3_fixedpoint_get_param_descrs(ctx->c, h->f);
  } else if (is_kind(x, Kind::Tactic)) {
    const Ref<Z3_tactic>& t = *get_xptr<Ref<Z3_tactic>>(x, Kind::Tactic);
    ctx = t.ctx();
    d = Z3_tactic_get_param_descrs(ctx->c, t);
  } else if (is_kind(x, Kind::Simplifier)) {
#if Z3R_AT_LEAST(4, 12, 2)
    const Ref<Z3_simplifier>& s = *get_xptr<Ref<Z3_simplifier>>(x, Kind::Simplifier);
    ctx = s.ctx();
    d = Z3_simplifier_get_param_descrs(ctx->c, s);
#else
    needs_version("z3_help() for simplifiers", "4.12.2");
#endif
  } else {
    ctx = get_ctx(x);
    d = Z3_simplify_get_param_descrs(ctx->c);
  }
  Ref<Z3_param_descrs> descrs = own(ctx, d);
  std::string out = str(Z3_param_descrs_to_string(ctx->c, descrs));
  check(ctx);
  return out;
}

// [[Rcpp::export(rng = false)]]
Rcpp::String cpp_solver_to_dimacs(SEXP s_, bool include_names) {
  SolverHolder* h = solver(s_);
  std::string out = str(Z3_solver_to_dimacs_string(h->s.c(), h->s, include_names));
  check(h->s.ctx());
  return out;
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_solver_translate(SEXP s_, SEXP target_ctx) {
  SolverHolder* h = solver(s_);
  refuse_copy_with_callbacks(h, "z3_translate()");
  CtxPtr target = get_ctx(target_ctx);
  Z3_solver t = Z3_solver_translate(h->s.c(), h->s, target->c);
  check(h->s.ctx());
  SEXP out = PROTECT(wrap_solver(target, t));
  get_xptr<SolverHolder>(out, Kind::Solver)->params = h->params;  // Z3 copies the parameters
  UNPROTECT(1);
  return out;
}

// [[Rcpp::export(rng = false)]]
void cpp_solver_initial_value(SEXP s_, SEXP var, SEXP value) {
#if Z3R_AT_LEAST(4, 13, 2)
  SolverHolder* h = solver(s_);
  const CtxPtr& ctx = h->s.ctx();
  Ast v = lift(ctx, var, nullptr);
  Ast val = lift(ctx, value, Z3_get_sort(ctx->c, v));
  Z3_solver_set_initial_value(ctx->c, h->s, v, val);
  check(ctx);
#else
  needs_version("z3_set_initial_value()", "4.13.2");
#endif
}

// Congruence closure queries after a satisfiable check. what is "root",
// "next" or "explain" (b is only used by "explain").
// [[Rcpp::export(rng = false)]]
SEXP cpp_solver_congruence(SEXP s_, std::string what, SEXP a_, SEXP b_) {
  SolverHolder* h = solver(s_);
  const CtxPtr& ctx = h->s.ctx();
  if (!h->checked || h->checked_at != h->generation || h->status != Z3_L_TRUE) {
    fail("congruence classes are only available after z3_check() returns \"sat\"");
  }
  Ast a = lift(ctx, a_, nullptr);
  if (what == "explain") {
#if Z3R_AT_LEAST(4, 14, 0)
    Ast b = lift(ctx, b_, Z3_get_sort(ctx->c, a));
    return wrap_expr(own_ast(ctx, Z3_solver_congruence_explain(ctx->c, h->s, a, b)));
#else
    (void)b_;
    needs_version("z3_congruence_explain()", "4.14.0");
#endif
  }
#if Z3R_AT_LEAST(4, 12, 0)
  if (what == "root") return wrap_expr(own_ast(ctx, Z3_solver_congruence_root(ctx->c, h->s, a)));
  return wrap_expr(own_ast(ctx, Z3_solver_congruence_next(ctx->c, h->s, a)));
#else
  needs_version("z3_congruence_root()", "4.12.0");
#endif
}

// Solves the current assertions for some variables: terms[i] is a term for
// variables[i] that holds under the conjunction of guards.
// [[Rcpp::export(rng = false)]]
SEXP cpp_solver_solve_for(SEXP s_, Rcpp::List variables) {
#if Z3R_AT_LEAST(4, 14, 0)
  SolverHolder* h = solver(s_);
  const CtxPtr& ctx = h->s.ctx();
  AstVec vs = expr_list(ctx, variables, "variables");
  Ref<Z3_ast_vector> vv = make_ast_vector(ctx, vs);
  Ref<Z3_ast_vector> terms = own(ctx, Z3_mk_ast_vector(ctx->c));
  Ref<Z3_ast_vector> guards = own(ctx, Z3_mk_ast_vector(ctx->c));
  {
    BusyGuard<SolverHolder> busy(h);
    Z3_solver_solve_for(ctx->c, h->s, vv, terms, guards);
  }
  raise_callback_errors(h->callbacks);
  check(ctx);
  // Z3 rewrites `vv` to the variables it solved for, in the order of the
  // terms. Z3 4.14.x also keeps unsolved variables with an empty term and
  // leaves unconditional guards empty; drop and fill them as 4.15.0 does.
  AstVec ov, ot, og;
  for (unsigned i = 0, n = Z3_ast_vector_size(ctx->c, terms); i < n; ++i) {
    Z3_ast t = Z3_ast_vector_get(ctx->c, terms, i);
    if (!t) continue;
    Z3_ast g = Z3_ast_vector_get(ctx->c, guards, i);
    ov.push_back(own_ast(ctx, Z3_ast_vector_get(ctx->c, vv, i)));
    ot.push_back(own_ast(ctx, t));
    og.push_back(own_ast(ctx, g ? g : Z3_mk_true(ctx->c)));
  }
  return Rcpp::List::create(Rcpp::Named("vars") = wrap_expr_list(ov), Rcpp::Named("terms") = wrap_expr_list(ot),
                            Rcpp::Named("guards") = wrap_expr_list(og));
#else
  (void)s_; (void)variables;
  needs_version("z3_solve_for()", "4.14.0");
#endif
}
