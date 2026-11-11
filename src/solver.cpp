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

// Builds a Z3 parameter set from a named R list, using the parameter
// descriptions of the target object to choose each value's type.
Ref<Z3_params> make_params(const CtxPtr& ctx, Z3_param_descrs descrs, SEXP values) {
  Z3_context c = ctx->c;
  Ref<Z3_params> p = own(ctx, Z3_mk_params(c));
  Rcpp::List lst(values);
  if (lst.size() == 0) return p;
  Rcpp::CharacterVector names = lst.names();
  for (R_xlen_t i = 0; i < lst.size(); ++i) {
    std::string name = Rcpp::as<std::string>(names[i]);
    if (name.empty()) fail("all parameters must be named");
    Z3_symbol sym = mk_symbol(ctx, name);
    Z3_param_kind kind = Z3_param_descrs_get_kind(c, descrs, sym);
    check(ctx);
    SEXP v = lst[i];
    if (Rf_xlength(v) != 1) fail("parameter '" + name + "' must be a single value");
    switch (kind) {
      case Z3_PK_BOOL: {
        if (TYPEOF(v) != LGLSXP || LOGICAL(v)[0] == NA_LOGICAL) {
          fail("parameter '" + name + "' must be TRUE or FALSE");
        }
        Z3_params_set_bool(c, p, sym, LOGICAL(v)[0] != 0);
        break;
      }
      case Z3_PK_UINT: {
        bool num = TYPEOF(v) == INTSXP || TYPEOF(v) == REALSXP;
        double d = num ? Rf_asReal(v) : NA_REAL;
        if (!num || ISNAN(d) || d < 0 ||
            d != std::floor(d) || d > 4294967295.0) {
          fail("parameter '" + name + "' must be a non-negative whole number");
        }
        Z3_params_set_uint(c, p, sym, static_cast<unsigned>(d));
        break;
      }
      case Z3_PK_DOUBLE: {
        bool num = TYPEOF(v) == INTSXP || TYPEOF(v) == REALSXP;
        double d = num ? Rf_asReal(v) : NA_REAL;
        if (!num || ISNAN(d)) {
          fail("parameter '" + name + "' must be a number");
        }
        Z3_params_set_double(c, p, sym, d);
        break;
      }
      case Z3_PK_SYMBOL:
      case Z3_PK_STRING: {
        if (TYPEOF(v) != STRSXP || STRING_ELT(v, 0) == NA_STRING) {
          fail("parameter '" + name + "' must be a string");
        }
        Z3_params_set_symbol(c, p, sym, mk_symbol(ctx, CHAR(STRING_ELT(v, 0))));
        break;
      }
      default:
        fail("unknown parameter '" + name + "'; z3_param_help() lists the "
             "parameters this object accepts");
    }
    check(ctx);
  }
  Z3_params_validate(c, p, descrs);
  check(ctx);
  return p;
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
    } else {
      out[i] = Z3_stats_get_double_value(c, st, i);
    }
    check(ctx);
  }
  out.names() = keys;
  return out;
}

}  // namespace z3r

using namespace z3r;

static SolverHolder* solver(SEXP x) { return get_xptr<SolverHolder>(x, Kind::Solver); }

static SEXP wrap_solver(const CtxPtr& ctx, Z3_solver s) {
  SolverHolder* h = new SolverHolder();
  h->s = own(ctx, s);
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
  // Validate everything before asserting anything.
  for (const Ast& a : v) {
    Z3_solver_assert(ctx->c, h->s, a);
    check(ctx);
  }
  h->touch();
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
  Z3_solver_assert_and_track(ctx->c, h->s, a, label);
  check(ctx);
  h->touch();
  return wrap_expr(label);
}

// [[Rcpp::export(rng = false)]]
std::string cpp_solver_check(SEXP s_, Rcpp::List assumptions) {
  SolverHolder* h = solver(s_);
  const CtxPtr& ctx = h->s.ctx();
  AstVec as;
  for (R_xlen_t i = 0; i < assumptions.size(); ++i) {
    Ast a = lift(ctx, assumptions[i], Z3_mk_bool_sort(ctx->c));
    if (!is_bool(a)) fail("assumptions must be Boolean");
    as.push_back(a);
  }
  std::vector<Z3_ast> r = raw(as);
  Z3_lbool st = r.empty() ? Z3_solver_check(ctx->c, h->s)
                          : Z3_solver_check_assumptions(ctx->c, h->s,
                                                        static_cast<unsigned>(r.size()), r.data());
  h->checked = true;
  h->checked_at = h->generation;
  h->status = Z3_L_UNDEF;
  check(ctx);
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
  return wrap_ast_vector(ctx, Z3_solver_get_unsat_core(ctx->c, h->s));
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
  Z3_solver_push(h->s.c(), h->s);
  check(h->s.ctx());
  h->touch();
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
  Z3_solver_pop(h->s.c(), h->s, static_cast<unsigned>(n));
  check(h->s.ctx());
  h->touch();
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

// [[Rcpp::export(rng = false)]]
void cpp_solver_set_params(SEXP s_, SEXP params) {
  SolverHolder* h = solver(s_);
  const CtxPtr& ctx = h->s.ctx();
  Ref<Z3_param_descrs> d = own(ctx, Z3_solver_get_param_descrs(ctx->c, h->s));
  Ref<Z3_params> p = make_params(ctx, d, params);
  Z3_solver_set_params(ctx->c, h->s, p);
  check(ctx);
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_solver_param_help(SEXP s_) {
  SolverHolder* h = solver(s_);
  return describe_params(h->s.ctx(), Z3_solver_get_param_descrs(h->s.c(), h->s));
}

// [[Rcpp::export(rng = false)]]
std::string cpp_solver_to_string(SEXP s_) {
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
  Z3_solver_from_string(h->s.c(), h->s, text.c_str());
  check(h->s.ctx());
  h->touch();
}
