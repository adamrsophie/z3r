// R callbacks called by Z3 during search: user propagators, clause
// callbacks, optimizer model callbacks and proof checking.
//
// Z3 calls these synchronously on the thread running the check, which is
// R's main thread. Everything a trampoline does in R (allocating arguments,
// evaluating the R function) runs inside R_ToplevelExec, so neither an R
// error nor a longjmp from a failed allocation can unwind through Z3's
// frames, and C++ exceptions are caught before they reach R or Z3. After an
// error the check is interrupted with Z3_interrupt and the error is raised
// in R once Z3 has returned. While a check runs, its solver or optimizer is
// marked busy, so callbacks cannot reenter or change it.
#include "z3r.h"

#include <climits>
#include <functional>

using namespace z3r;

namespace {

struct CbHolder {
  Z3_solver_callback cb;
  CtxPtr ctx;
};

// Handle given to R for the duration of one callback.
struct ScopedHandle {
  SEXP x;
  ScopedHandle(Z3_solver_callback cb, const CtxPtr& ctx) {
    CbHolder* h = new CbHolder{cb, ctx};
    x = PROTECT(make_xptr(h, Kind::Callback));
  }
  ~ScopedHandle() {
    CbHolder* h = static_cast<CbHolder*>(R_ExternalPtrAddr(x));
    if (h) {
      R_ClearExternalPtr(x);
      delete h;
    }
    UNPROTECT(1);
  }
};

// Counts the R callbacks of a state that are running.
struct Running {
  CallbackState* st;
  explicit Running(CallbackState* s) : st(s) { ++st->running; }
  ~Running() { --st->running; }
};

void record_error(CallbackState* st, const std::string& msg, bool interrupted = false) {
  if (!st || !st->error.empty()) return;
  st->error = msg;
  st->interrupted = interrupted;
  if (CtxPtr c = st->context()) Z3_interrupt(c->c);
}

// Calls fns[[name]](args...) through z3r:::callback_invoke, which returns
// list(value, error message, interrupted). Returns the R value (or
// R_NilValue), unprotected; callers protect it if needed.
SEXP invoke(CallbackState* st, const char* name, std::vector<SEXP> args) {
  if (!st || !st->error.empty()) return R_NilValue;
  // fns holds the user's functions and, under ".invoke", z3r's wrapper
  // that evaluates them inside tryCatch.
  SEXP fn = R_NilValue, invoker = R_NilValue;
  SEXP names = Rf_getAttrib(st->fns, R_NamesSymbol);
  for (R_xlen_t i = 0; i < Rf_xlength(st->fns); ++i) {
    std::string nm = CHAR(STRING_ELT(names, i));
    if (nm == name) fn = VECTOR_ELT(st->fns, i);
    if (nm == ".invoke") invoker = VECTOR_ELT(st->fns, i);
  }
  if (fn == R_NilValue || invoker == R_NilValue) return R_NilValue;
  SEXP call = PROTECT(Rf_lcons(invoker, R_NilValue));
  SEXP tail = call;
  SETCDR(tail, Rf_cons(fn, R_NilValue));
  tail = CDR(tail);
  for (SEXP a : args) {
    SETCDR(tail, Rf_cons(a, R_NilValue));
    tail = CDR(tail);
  }
  int failed = 0;
  SEXP res;
  {
    Running r(st);
    res = R_tryEval(call, R_GlobalEnv, &failed);
  }
  UNPROTECT(1);
  if (failed || TYPEOF(res) != VECSXP || Rf_xlength(res) != 3) {
    record_error(st, std::string("the R callback '") + name + "' failed");
    return R_NilValue;
  }
  SEXP msg = VECTOR_ELT(res, 1);
  if (msg != R_NilValue) {
    bool intr = Rf_asLogical(VECTOR_ELT(res, 2)) == TRUE;
    record_error(st, std::string("error in the R callback '") + name + "': " + CHAR(STRING_ELT(msg, 0)), intr);
    return R_NilValue;
  }
  return VECTOR_ELT(res, 0);
}

struct Job {
  std::function<void()> body;
  std::string error;
};

void run_job(void* data) {
  Job* j = static_cast<Job*>(data);
  try {
    j->body();
  } catch (std::exception& e) {
    j->error = e.what();
  } catch (...) {
    j->error = "unexpected error in a callback";
  }
}

// Runs the R-facing part of a trampoline. R_ToplevelExec stops any R
// longjmp at this frame; run_job stops C++ exceptions.
template <typename F> void guarded(CallbackState* st, F f) {
  Job j{f, std::string()};
  Rboolean ok = R_ToplevelExec(run_job, &j);
  if (!ok && j.error.empty()) j.error = "an R error occurred while calling a callback";
  if (!j.error.empty()) record_error(st, j.error);
}

CallbackState* state(void* p) { return static_cast<CallbackState*>(p); }

[[maybe_unused]] SEXP expr_arg(const CtxPtr& ctx, Z3_ast a) { return wrap_expr(Ast(ctx, a)); }

// ---- user propagator trampolines ----------------------------------------

#if Z3R_AT_LEAST(4, 12, 3)
void Z3_API push_eh(void* p, Z3_solver_callback cb) {
  CallbackState* st = state(p);
  guarded(st, [&] {
    if (!st) return;
    ScopedHandle h(cb, st->ctx);
    invoke(st, "push", {h.x});
  });
}

void Z3_API pop_eh(void* p, Z3_solver_callback cb, unsigned n) {
  CallbackState* st = state(p);
  guarded(st, [&] {
    if (!st) return;
    ScopedHandle h(cb, st->ctx);
    SEXP k = PROTECT(Rf_ScalarInteger(static_cast<int>(n)));
    invoke(st, "pop", {h.x, k});
    UNPROTECT(1);
  });
}

// Z3 copies the solver in some configurations. The copy lives in another
// context and gets no callbacks; z3r records this and warns after the check.
void* Z3_API fresh_eh(void* p, Z3_context) {
  CallbackState* st = state(p);
  if (st) st->cloned = true;
  return nullptr;
}

void Z3_API fixed_eh(void* p, Z3_solver_callback cb, Z3_ast t, Z3_ast value) {
  CallbackState* st = state(p);
  guarded(st, [&] {
    if (!st) return;
    ScopedHandle h(cb, st->ctx);
    SEXP a = PROTECT(expr_arg(st->ctx, t));
    SEXP v = PROTECT(expr_arg(st->ctx, value));
    invoke(st, "fixed", {h.x, a, v});
    UNPROTECT(2);
  });
}

void Z3_API eq_eh(void* p, Z3_solver_callback cb, Z3_ast s, Z3_ast t) {
  CallbackState* st = state(p);
  guarded(st, [&] {
    if (!st) return;
    ScopedHandle h(cb, st->ctx);
    SEXP a = PROTECT(expr_arg(st->ctx, s));
    SEXP b = PROTECT(expr_arg(st->ctx, t));
    invoke(st, "eq", {h.x, a, b});
    UNPROTECT(2);
  });
}

void Z3_API diseq_eh(void* p, Z3_solver_callback cb, Z3_ast s, Z3_ast t) {
  CallbackState* st = state(p);
  guarded(st, [&] {
    if (!st) return;
    ScopedHandle h(cb, st->ctx);
    SEXP a = PROTECT(expr_arg(st->ctx, s));
    SEXP b = PROTECT(expr_arg(st->ctx, t));
    invoke(st, "diseq", {h.x, a, b});
    UNPROTECT(2);
  });
}

void Z3_API final_eh(void* p, Z3_solver_callback cb) {
  CallbackState* st = state(p);
  guarded(st, [&] {
    if (!st) return;
    ScopedHandle h(cb, st->ctx);
    invoke(st, "final", {h.x});
  });
}

void Z3_API created_eh(void* p, Z3_solver_callback cb, Z3_ast t) {
  CallbackState* st = state(p);
  guarded(st, [&] {
    if (!st) return;
    ScopedHandle h(cb, st->ctx);
    SEXP a = PROTECT(expr_arg(st->ctx, t));
    invoke(st, "created", {h.x, a});
    UNPROTECT(1);
  });
}

void Z3_API decide_eh(void* p, Z3_solver_callback cb, Z3_ast t, unsigned idx, bool phase) {
  CallbackState* st = state(p);
  guarded(st, [&] {
    if (!st) return;
    ScopedHandle h(cb, st->ctx);
    SEXP a = PROTECT(expr_arg(st->ctx, t));
    SEXP i = PROTECT(Rf_ScalarInteger(static_cast<int>(idx)));
    SEXP ph = PROTECT(Rf_ScalarLogical(phase));
    invoke(st, "decide", {h.x, a, i, ph});
    UNPROTECT(3);
  });
}

void Z3_API on_clause_eh(void* p, Z3_ast proof_hint, unsigned n, unsigned const* deps, Z3_ast_vector literals) {
  CallbackState* st = state(p);
  guarded(st, [&] {
    if (!st) return;
    SEXP hint = PROTECT(proof_hint ? expr_arg(st->ctx, proof_hint) : R_NilValue);
    SEXP dv = PROTECT(Rf_allocVector(INTSXP, n));
    for (unsigned i = 0; i < n; ++i) INTEGER(dv)[i] = static_cast<int>(deps[i]);
    SEXP lits = PROTECT(wrap_ast_vector(st->ctx, literals));
    invoke(st, "clause", {hint, dv, lits});
    UNPROTECT(3);
  });
}
#endif

#if Z3R_AT_LEAST(4, 15, 3)
bool Z3_API on_binding_eh(void* p, Z3_solver_callback cb, Z3_ast q, Z3_ast inst) {
  CallbackState* st = state(p);
  bool keep = true;
  guarded(st, [&] {
    if (!st) return;
    ScopedHandle h(cb, st->ctx);
    SEXP a = PROTECT(expr_arg(st->ctx, q));
    SEXP b = PROTECT(expr_arg(st->ctx, inst));
    SEXP r = invoke(st, "binding", {h.x, a, b});
    if (TYPEOF(r) == LGLSXP && Rf_xlength(r) == 1 && LOGICAL(r)[0] == 0) keep = false;
    UNPROTECT(2);
  });
  return keep;
}
#endif

void Z3_API model_eh(void* p) {
  CallbackState* st = state(p);
  guarded(st, [&] {
    if (!st || !st->model.get()) return;
    // Snapshot: Z3 keeps updating the registered model.
    Z3_model copy = Z3_model_translate(st->ctx->c, st->model, st->ctx->c);
    SEXP m = PROTECT(make_xptr(new Ref<Z3_model>(own(st->ctx, copy)), Kind::Model));
    invoke(st, "model", {m});
    UNPROTECT(1);
  });
}

#if Z3R_AT_LEAST(5, 3, 2)
Z3_string Z3_API proof_check_eh(void* p, Z3_context, Z3_ast proof, Z3_ast_vector assertions) {
  CallbackState* st = state(p);
  if (!st) return "";
  // Hold the state: the checker cannot replace itself while it runs (see
  // cpp_set_proof_check), but keep it alive regardless.
  CallbackPtr keep = st->shared_from_this();
  guarded(st, [&] {
    CtxPtr ctx = st->context();
    if (!ctx) return;
    SEXP pr = PROTECT(expr_arg(ctx, proof));
    SEXP as = PROTECT(wrap_ast_vector(ctx, assertions));
    SEXP r = invoke(st, "check", {pr, as});
    if (TYPEOF(r) == STRSXP && Rf_xlength(r) == 1 && STRING_ELT(r, 0) != NA_STRING) {
      st->report = Rf_translateCharUTF8(STRING_ELT(r, 0));
    } else {
      st->report.clear();  // NULL or no usable report: an empty report
    }
    UNPROTECT(2);
  });
  // Z3 copies the report; an error is raised after the interpreter returns.
  return st->report.c_str();
}
#endif

// A state whose R functions are kept by `owner`, the R object of `holder`.
CallbackPtr new_state(const CtxPtr& ctx, SEXP fns, SEXP owner, const void* holder) {
  CallbackPtr st = std::make_shared<CallbackState>();
  st->ctx = ctx;
  keep_callbacks(owner, fns);
  st->fns = fns;
  st->owner = holder;
  return st;
}

[[maybe_unused]] bool has(SEXP fns, const char* name) {
  SEXP names = Rf_getAttrib(fns, R_NamesSymbol);
  for (R_xlen_t i = 0; i < Rf_xlength(fns); ++i) {
    if (std::string(CHAR(STRING_ELT(names, i))) == name) return true;
  }
  return false;
}

[[maybe_unused]] CbHolder* cb_handle(SEXP x) { return get_xptr<CbHolder>(x, Kind::Callback); }

}  // namespace

namespace z3r {
void raise_callback_errors(std::vector<CallbackPtr>& cbs) {
  for (CallbackPtr& cb : cbs) {
    if (!cb->error.empty()) {
      std::string msg = cb->error;
      bool intr = cb->interrupted;
      cb->error.clear();
      cb->interrupted = false;
      if (intr) throw Rcpp::internal::InterruptedException();
      fail(msg + " (the check was interrupted)");
    }
  }
}
}  // namespace z3r

// [[Rcpp::export(rng = false)]]
void cpp_propagate_init(SEXP s_, SEXP fns) {
#if Z3R_AT_LEAST(4, 12, 3)
  SolverHolder* h = solver_of(s_);
  const CtxPtr& ctx = h->s.ctx();
  Z3_context c = ctx->c;
  for (const CallbackPtr& cb : h->callbacks) {
    if (has(cb->fns, "push")) fail("this solver already has a user propagator");
  }
  // Check every option before Z3 can keep a pointer to the state.
#if !Z3R_AT_LEAST(4, 15, 3)
  if (has(fns, "binding")) needs_version("the binding callback", "4.15.3");
#endif
  // The holder owns the state from now on, even if registration fails.
  CallbackPtr st = new_state(ctx, fns, s_, h);
  h->callbacks.push_back(st);
  Z3_solver_propagate_init(c, h->s, st.get(), push_eh, pop_eh, fresh_eh);
  check(ctx);
  if (has(fns, "fixed")) Z3_solver_propagate_fixed(c, h->s, fixed_eh);
  if (has(fns, "eq")) Z3_solver_propagate_eq(c, h->s, eq_eh);
  if (has(fns, "diseq")) Z3_solver_propagate_diseq(c, h->s, diseq_eh);
  if (has(fns, "final")) Z3_solver_propagate_final(c, h->s, final_eh);
  if (has(fns, "created")) Z3_solver_propagate_created(c, h->s, created_eh);
  if (has(fns, "decide")) Z3_solver_propagate_decide(c, h->s, decide_eh);
#if Z3R_AT_LEAST(4, 15, 3)
  if (has(fns, "binding")) Z3_solver_propagate_on_binding(c, h->s, on_binding_eh);
#endif
  check(ctx);
#else
  (void)s_; (void)fns;
  needs_version("user propagators", "4.12.3");
#endif
}

// [[Rcpp::export(rng = false)]]
void cpp_propagate_register(SEXP s_, SEXP e_) {
#if Z3R_AT_LEAST(4, 12, 3)
  SolverHolder* h = solver_of(s_);
  bool has_propagator = false;
  for (const CallbackPtr& cb : h->callbacks) has_propagator = has_propagator || has(cb->fns, "push");
  if (!has_propagator) fail("install a user propagator with z3_propagator() before registering terms");
  Ast e = lift(h->s.ctx(), e_, nullptr);
  ensure_solver_ready(h->s.c(), h->s);
  {
    // Registration replays pending scopes, which runs push callbacks.
    BusyGuard<SolverHolder> busy(h);
    Z3_solver_propagate_register(h->s.c(), h->s, e);
  }
  raise_callback_errors(h->callbacks);
  check(h->s.ctx());
#else
  (void)s_; (void)e_;
  needs_version("user propagators", "4.12.3");
#endif
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_propagate_declare(SEXP ctx_, SEXP name_, Rcpp::List domain, SEXP range_) {
#if Z3R_AT_LEAST(4, 12, 3)
  CtxPtr ctx = get_ctx(ctx_);
  std::string name = as_string(name_, "name");
  std::vector<Z3_sort> d;
  AstVec keep;
  for (R_xlen_t i = 0; i < domain.size(); ++i) {
    const Ast& s = get_sort(domain[i]);
    same_ctx(ctx, s.ctx());
    keep.push_back(s);
    d.push_back(as_sort(s));
  }
  const Ast& r = get_sort(range_);
  same_ctx(ctx, r.ctx());
  Z3_func_decl f = Z3_solver_propagate_declare(ctx->c, mk_symbol(ctx, name), static_cast<unsigned>(d.size()),
                                               d.empty() ? nullptr : d.data(), as_sort(r));
  check(ctx);
  return wrap_decl(own_ast(ctx, decl_ast(f)));
#else
  (void)ctx_; (void)name_; (void)domain; (void)range_;
  needs_version("user propagators", "4.12.3");
#endif
}

// Inside callbacks: propagate a consequence (or a conflict when conseq is
// FALSE) justified by fixed terms and equalities.
// [[Rcpp::export(rng = false)]]
bool cpp_cb_consequence(SEXP cb_, Rcpp::List fixed, Rcpp::List lhs, Rcpp::List rhs, SEXP conseq) {
#if Z3R_AT_LEAST(4, 12, 3)
  CbHolder* h = cb_handle(cb_);
  const CtxPtr& ctx = h->ctx;
  AstVec f = expr_list(ctx, fixed, "fixed terms");
  AstVec l = expr_list(ctx, lhs, "equality left sides");
  AstVec r = expr_list(ctx, rhs, "equality right sides");
  if (l.size() != r.size()) fail("lhs and rhs must have the same length");
  Ast q = lift(ctx, conseq, Z3_mk_bool_sort(ctx->c));
  if (!is_bool(q)) fail("the consequence must be Boolean");
  std::vector<Z3_ast> rf = raw(f), rl = raw(l), rr = raw(r);
  bool ok = Z3_solver_propagate_consequence(ctx->c, h->cb, static_cast<unsigned>(rf.size()),
                                            rf.empty() ? nullptr : rf.data(), static_cast<unsigned>(rl.size()),
                                            rl.empty() ? nullptr : rl.data(), rr.empty() ? nullptr : rr.data(), q);
  check(ctx);
  return ok;
#else
  (void)cb_; (void)fixed; (void)lhs; (void)rhs; (void)conseq;
  needs_version("user propagators", "4.12.3");
#endif
}

// [[Rcpp::export(rng = false)]]
void cpp_cb_register(SEXP cb_, SEXP e_) {
#if Z3R_AT_LEAST(4, 12, 3)
  CbHolder* h = cb_handle(cb_);
  Ast e = lift(h->ctx, e_, nullptr);
  Z3_solver_propagate_register_cb(h->ctx->c, h->cb, e);
  check(h->ctx);
#else
  (void)cb_; (void)e_;
  needs_version("user propagators", "4.12.3");
#endif
}

// [[Rcpp::export(rng = false)]]
bool cpp_cb_next_split(SEXP cb_, SEXP t_, int idx, int phase) {
#if Z3R_AT_LEAST(4, 12, 3)
  CbHolder* h = cb_handle(cb_);
  Ast t = lift(h->ctx, t_, nullptr);
  if (idx < 0) fail("idx must be non-negative");
  Z3_lbool ph = phase > 0 ? Z3_L_TRUE : phase < 0 ? Z3_L_FALSE : Z3_L_UNDEF;
  bool ok = Z3_solver_next_split(h->ctx->c, h->cb, t, static_cast<unsigned>(idx), ph);
  check(h->ctx);
  return ok;
#else
  (void)cb_; (void)t_; (void)idx; (void)phase;
  needs_version("user propagators", "4.12.3");
#endif
}

// [[Rcpp::export(rng = false)]]
void cpp_on_clause(SEXP s_, SEXP fns) {
#if Z3R_AT_LEAST(4, 12, 3)
  SolverHolder* h = solver_of(s_);
  CallbackPtr st = new_state(h->s.ctx(), fns, s_, h);
  h->callbacks.push_back(st);
  Z3_solver_register_on_clause(h->s.c(), h->s, st.get(), on_clause_eh);
  check(h->s.ctx());
#else
  (void)s_; (void)fns;
  needs_version("z3_on_clause()", "4.12.3");
#endif
}

// [[Rcpp::export(rng = false)]]
void cpp_on_model(SEXP o_, SEXP fns) {
  OptimizeHolder* h = optimizer_of(o_);
  const CtxPtr& ctx = h->o.ctx();
  CallbackPtr st = new_state(ctx, fns, o_, h);
  st->model = own(ctx, Z3_mk_model(ctx->c));
  h->callbacks.push_back(st);
  Z3_optimize_register_model_eh(ctx->c, h->o, st->model, st.get(), model_eh);
  check(ctx);
}

// The checker belongs to the context's R handle: its function is kept in
// the handle's protected slot and it is removed when the handle is
// collected (ContextHolder::~ContextHolder).
// [[Rcpp::export(rng = false)]]
void cpp_set_proof_check(SEXP ctx_, SEXP fns) {
#if Z3R_AT_LEAST(5, 3, 2)
  CtxPtr ctx = get_ctx(ctx_);
  if (ctx->proof_check && ctx->proof_check->running > 0) {
    fail("the proof checker cannot be replaced or removed while it runs");
  }
  SEXP handle = PROTECT(wrap_ctx(ctx));
  ContextHolder* holder = get_xptr<ContextHolder>(handle, Kind::Context);
  if (Rf_isNull(fns)) {
    Z3_set_proof_check_callback(ctx->c, nullptr, nullptr);
    ctx->proof_check.reset();
    drop_callbacks(handle);
    UNPROTECT(1);
    check(ctx);
    return;
  }
  drop_callbacks(handle);
  CallbackPtr st = new_state(ctx, fns, handle, holder);
  st->ctx.reset();  // the context owns this state; avoid a reference cycle
  st->weak = ctx;
  ctx->proof_check = st;
  Z3_set_proof_check_callback(ctx->c, st.get(), proof_check_eh);
  UNPROTECT(1);
  check(ctx);
#else
  (void)ctx_; (void)fns;
  needs_version("z3_set_proof_checker()", "5.3.2");
#endif
}

// Raises an error recorded by the proof checker during the last SMT-LIB2
// evaluation.
namespace z3r {
void raise_proof_check_error(const CtxPtr& ctx) {
  if (!ctx->proof_check) return;
  std::vector<CallbackPtr> v(1, ctx->proof_check);
  raise_callback_errors(v);
}
}  // namespace z3r

// ---------------------------------------------------------------------------
// Fixedpoint callbacks: Spacer lemma export and external Datalog relations.

namespace {

void fp_lemma_eh(void* p, Z3_ast lemma, unsigned level) {
  CallbackState* st = state(p);
  guarded(st, [&] {
    if (!st) return;
    SEXP l = PROTECT(expr_arg(st->ctx, lemma));
    // Infinite level (an inductive invariant) is reported as Inf.
    SEXP lv = PROTECT(Rf_ScalarReal(level == UINT_MAX ? R_PosInf : static_cast<double>(level)));
    invoke(st, "lemma", {l, lv});
    UNPROTECT(2);
  });
}

void fp_predecessor_eh(void* p) {
  CallbackState* st = state(p);
  guarded(st, [&] {
    if (st) invoke(st, "predecessor", {});
  });
}

void fp_unfold_eh(void* p) {
  CallbackState* st = state(p);
  guarded(st, [&] {
    if (st) invoke(st, "unfold", {});
  });
}

SEXP ast_args(const CtxPtr& ctx, unsigned n, Z3_ast const args[]) {
  SEXP out = PROTECT(Rf_allocVector(VECSXP, n));
  for (unsigned i = 0; i < n; ++i) SET_VECTOR_ELT(out, i, expr_arg(ctx, args[i]));
  UNPROTECT(1);
  return out;
}

// Reduces an application of a relational operator on external relations.
// The R function returns a term of the operator's range sort, or NULL to
// keep the application as it is.
void fp_reduce_app_eh(void* p, Z3_func_decl f, unsigned n, Z3_ast const args[], Z3_ast* result) {
  CallbackState* st = state(p);
  *result = nullptr;
  guarded(st, [&] {
    if (!st) return;
    SEXP d = PROTECT(wrap_decl(Ast(st->ctx, decl_ast(f))));
    SEXP a = PROTECT(ast_args(st->ctx, n, args));
    SEXP r = PROTECT(invoke(st, "app", {d, a}));
    if (r != R_NilValue) {
      if (!is_kind(r, Kind::Expr)) fail("the 'app' callback must return a Z3 expression or NULL");
      const Ast& e = get_expr(r);
      same_ctx(st->ctx, e.ctx());
      if (!Z3_is_eq_sort(st->ctx->c, Z3_get_sort(st->ctx->c, e), Z3_get_range(st->ctx->c, f))) {
        fail("the 'app' callback returned a term of the wrong sort");
      }
      st->results.push_back(e);  // keep it alive until Z3 has its own reference
      *result = e;
    }
    UNPROTECT(3);
  });
}

void fp_reduce_assign_eh(void* p, Z3_func_decl f, unsigned n, Z3_ast const args[], unsigned nout,
                         Z3_ast const outs[]) {
  CallbackState* st = state(p);
  guarded(st, [&] {
    if (!st) return;
    SEXP d = PROTECT(wrap_decl(Ast(st->ctx, decl_ast(f))));
    SEXP a = PROTECT(ast_args(st->ctx, n, args));
    SEXP o = PROTECT(ast_args(st->ctx, nout, outs));
    invoke(st, "assign", {d, a, o});
    UNPROTECT(3);
  });
}

}  // namespace

// [[Rcpp::export(rng = false)]]
void cpp_fixedpoint_on_lemma(SEXP f_, SEXP fns) {
  FixedpointHolder* h = fixedpoint_of(f_);
  const CtxPtr& ctx = h->f.ctx();
  CallbackPtr st = new_state(ctx, fns, f_, h);
  h->callbacks.push_back(st);
  Z3_fixedpoint_add_callback(ctx->c, h->f, st.get(), has(fns, "lemma") ? fp_lemma_eh : nullptr,
                             has(fns, "predecessor") ? fp_predecessor_eh : nullptr,
                             has(fns, "unfold") ? fp_unfold_eh : nullptr);
  check(ctx);
}

// [[Rcpp::export(rng = false)]]
void cpp_fixedpoint_external(SEXP f_, SEXP fns) {
  FixedpointHolder* h = fixedpoint_of(f_);
  if (h->external) fail("external relation callbacks can be set only once per fixedpoint engine");
  const CtxPtr& ctx = h->f.ctx();
  CallbackPtr st = new_state(ctx, fns, f_, h);
  h->callbacks.push_back(st);
  h->external = true;
  Z3_fixedpoint_init(ctx->c, h->f, st.get());
  check(ctx);
  if (has(fns, "app")) Z3_fixedpoint_set_reduce_app_callback(ctx->c, h->f, fp_reduce_app_eh);
  if (has(fns, "assign")) Z3_fixedpoint_set_reduce_assign_callback(ctx->c, h->f, fp_reduce_assign_eh);
  check(ctx);
}
