// Core ownership layer for z3r.
//
// Ownership rules:
//
// * A Z3 context is created with Z3_mk_context_rc and owned by a
//   std::shared_ptr<Context>. Every wrapper of a Z3 object (AST, solver,
//   model, ...) holds a copy of that shared_ptr, so the context is deleted
//   only after the last dependent object has released its reference. R's
//   garbage collector may therefore finalize objects in any order.
// * Every Z3 object handle is held through Ref<T>, which calls the matching
//   Z3_*_inc_ref on acquisition and Z3_*_dec_ref on release. A fresh Z3
//   result has reference count zero and may be reclaimed by the next API
//   call, so it is wrapped in a Ref immediately after the error check.
// * Z3's default error handler terminates the process, so it is disabled.
//   After every API call the error code is checked and converted into a C++
//   exception, which Rcpp turns into an ordinary R error after unwinding
//   (and therefore releasing) all local references.
// * R external pointers own heap-allocated holders. The finalizer clears
//   the pointer before deleting the holder, so a holder is freed once.
// * Contexts are not shared between threads. z3r never calls Z3 from more
//   than one thread.
#ifndef Z3R_H
#define Z3R_H

#include <Rcpp.h>
#include <z3.h>

// Z3 version known at compile time. Functions added after the minimum
// supported release (4.8.13) are compiled only when the headers have them,
// guarded by the release that first declared them.
#if defined(__has_include)
#if __has_include(<z3_version.h>)
#include <z3_version.h>
#endif
#endif
#ifdef Z3_MAJOR_VERSION
#define Z3R_VERSION (Z3_MAJOR_VERSION * 10000 + Z3_MINOR_VERSION * 100 + Z3_BUILD_NUMBER)
#else
#define Z3R_VERSION 40813
#endif
#define Z3R_AT_LEAST(a, b, c) (Z3R_VERSION >= ((a) * 10000 + (b) * 100 + (c)))

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace z3r {

[[noreturn]] void fail(const std::string& msg);
// Error for a feature that needs a newer Z3 than the one z3r was built with.
[[noreturn]] void needs_version(const char* feature, const char* version);

struct CallbackState;

struct Context {
  Z3_context c;
  std::shared_ptr<CallbackState> proof_check;  // Z3_set_proof_check_callback
  // Weak reference to the one R handle of this context, so that every R
  // object refers to the same handle (callbacks owned by the context are
  // kept in that handle's protected slot).
  SEXP handle = R_NilValue;
  bool interpreting = false;  // running Z3_eval_smtlib2_string
  // config: pairs of Z3 configuration parameter names and values.
  explicit Context(const std::vector<std::pair<std::string, std::string>>& config = {});
  ~Context();
  Context(const Context&) = delete;
  Context& operator=(const Context&) = delete;
};
typedef std::shared_ptr<Context> CtxPtr;

// Throws an R error if the last Z3 call on this context failed.
void check(Z3_context c);
inline void check(const CtxPtr& ctx) { check(ctx->c); }

template <typename T> struct RefOps;
#define Z3R_REFOPS(T, INC, DEC)                                         \
  template <> struct RefOps<T> {                                        \
    static void inc(Z3_context c, T p) { INC(c, p); }                   \
    static void dec(Z3_context c, T p) { DEC(c, p); }                   \
  };
Z3R_REFOPS(Z3_ast, Z3_inc_ref, Z3_dec_ref)
Z3R_REFOPS(Z3_solver, Z3_solver_inc_ref, Z3_solver_dec_ref)
Z3R_REFOPS(Z3_model, Z3_model_inc_ref, Z3_model_dec_ref)
Z3R_REFOPS(Z3_optimize, Z3_optimize_inc_ref, Z3_optimize_dec_ref)
Z3R_REFOPS(Z3_params, Z3_params_inc_ref, Z3_params_dec_ref)
Z3R_REFOPS(Z3_param_descrs, Z3_param_descrs_inc_ref, Z3_param_descrs_dec_ref)
Z3R_REFOPS(Z3_ast_vector, Z3_ast_vector_inc_ref, Z3_ast_vector_dec_ref)
Z3R_REFOPS(Z3_ast_map, Z3_ast_map_inc_ref, Z3_ast_map_dec_ref)
Z3R_REFOPS(Z3_stats, Z3_stats_inc_ref, Z3_stats_dec_ref)
Z3R_REFOPS(Z3_goal, Z3_goal_inc_ref, Z3_goal_dec_ref)
Z3R_REFOPS(Z3_tactic, Z3_tactic_inc_ref, Z3_tactic_dec_ref)
Z3R_REFOPS(Z3_apply_result, Z3_apply_result_inc_ref, Z3_apply_result_dec_ref)
Z3R_REFOPS(Z3_func_interp, Z3_func_interp_inc_ref, Z3_func_interp_dec_ref)
Z3R_REFOPS(Z3_func_entry, Z3_func_entry_inc_ref, Z3_func_entry_dec_ref)
Z3R_REFOPS(Z3_probe, Z3_probe_inc_ref, Z3_probe_dec_ref)
Z3R_REFOPS(Z3_fixedpoint, Z3_fixedpoint_inc_ref, Z3_fixedpoint_dec_ref)
#if Z3R_AT_LEAST(4, 12, 2)
Z3R_REFOPS(Z3_simplifier, Z3_simplifier_inc_ref, Z3_simplifier_dec_ref)
#endif
#if Z3R_AT_LEAST(4, 9, 0)
Z3R_REFOPS(Z3_parser_context, Z3_parser_context_inc_ref, Z3_parser_context_dec_ref)
#endif
#undef Z3R_REFOPS

// Reference-counted handle that also keeps its context alive.
template <typename T> class Ref {
 public:
  Ref() : p_(nullptr) {}
  Ref(CtxPtr ctx, T p) : ctx_(std::move(ctx)), p_(p) {
    if (p_) RefOps<T>::inc(ctx_->c, p_);
  }
  Ref(const Ref& o) : ctx_(o.ctx_), p_(o.p_) {
    if (p_) RefOps<T>::inc(ctx_->c, p_);
  }
  Ref(Ref&& o) noexcept : ctx_(std::move(o.ctx_)), p_(o.p_) { o.p_ = nullptr; }
  Ref& operator=(Ref o) noexcept {
    std::swap(ctx_, o.ctx_);
    std::swap(p_, o.p_);
    return *this;
  }
  ~Ref() {
    if (p_) RefOps<T>::dec(ctx_->c, p_);
  }
  T get() const { return p_; }
  operator T() const { return p_; }
  const CtxPtr& ctx() const { return ctx_; }
  Z3_context c() const { return ctx_->c; }

 private:
  CtxPtr ctx_;  // declared first so it is destroyed after the dec_ref
  T p_;
};

typedef Ref<Z3_ast> Ast;
typedef std::vector<Ast> AstVec;

// Checks the error state, then takes a reference to a fresh result.
template <typename T> inline Ref<T> own(const CtxPtr& ctx, T p) {
  check(ctx);
  if (!p) fail("Z3 returned an empty result");
  return Ref<T>(ctx, p);
}
inline Ast own_ast(const CtxPtr& ctx, Z3_ast p) { return own<Z3_ast>(ctx, p); }

inline Z3_sort as_sort(Z3_ast a) { return reinterpret_cast<Z3_sort>(a); }
inline Z3_func_decl as_decl(Z3_ast a) { return reinterpret_cast<Z3_func_decl>(a); }
inline Z3_ast sort_ast(Z3_sort s) { return reinterpret_cast<Z3_ast>(s); }
inline Z3_ast decl_ast(Z3_func_decl d) { return reinterpret_cast<Z3_ast>(d); }

// Raw pointer arrays for Z3 calls; the Ast objects keep the references.
std::vector<Z3_ast> raw(const AstVec& v);

// Z3 returns strings in a buffer that the next call overwrites; copy at once.
inline std::string str(const char* s) { return s ? std::string(s) : std::string(); }

// ---------------------------------------------------------------------------
// R external pointers

struct ContextHolder {
  CtxPtr ctx;
  ~ContextHolder();  // removes callbacks owned by this handle
};

// State of R callbacks registered with Z3 (user propagators, clause and
// model callbacks). Owned by the solver, optimizer or context that uses it,
// so it lives as long as Z3 may call it.
// The R functions are not preserved globally: they are stored in the
// protected slot of the R object that owns the state (solver, optimizer or
// context handle), so R's collector sees closures that refer back to their
// owner and can collect both.
struct CallbackState : std::enable_shared_from_this<CallbackState> {
  CtxPtr ctx;                // owning reference (solver and optimizer callbacks)
  std::weak_ptr<Context> weak;  // used instead by context-owned callbacks
  CtxPtr context() const { return ctx ? ctx : weak.lock(); }
  SEXP fns = R_NilValue;     // named list of R functions, kept by the owner
  std::string error;         // first error raised by an R callback
  bool interrupted = false;  // the error was a user interrupt
  std::string report;        // storage for strings returned to Z3
  bool cloned = false;       // Z3 copied the solver; the copy has no callbacks
  int running = 0;           // R callbacks of this state now running
  const void* owner = nullptr;  // holder of the owning R object
  Ref<Z3_model> model;       // model updated by Z3_optimize_register_model_eh
  AstVec results;            // terms returned to Z3 by the current query
};
typedef std::shared_ptr<CallbackState> CallbackPtr;

// Stores callback functions in the protected slot of their owner, and drops
// them all (after the owner's callbacks were removed).
void keep_callbacks(SEXP owner, SEXP fns);
void drop_callbacks(SEXP owner);

// Some Z3 solver functions use the solver's internal state without creating
// it first (Z3 creates it lazily), and crash on a solver that has not been
// used yet. Z3_solver_get_num_scopes creates it; call this before them.
inline void ensure_solver_ready(Z3_context c, Z3_solver s) { Z3_solver_get_num_scopes(c, s); }

// Raises the first callback error recorded during a check, if any (an R
// interrupt when the callback was interrupted).
void raise_callback_errors(std::vector<CallbackPtr>& cbs);
void raise_proof_check_error(const CtxPtr& ctx);

struct SolverHolder {
  Ref<Z3_solver> s;
  std::vector<CallbackPtr> callbacks;
  bool busy = false;            // running Z3 code that may call R callbacks
  uint64_t generation = 0;      // bumped whenever the assertion stack changes
  uint64_t checked_at = 0;      // generation of the last check
  bool checked = false;
  Z3_lbool status = Z3_L_UNDEF;
  void touch() { ++generation; }
};

struct OptimizeHolder {
  Ref<Z3_optimize> o;
  std::vector<CallbackPtr> callbacks;
  bool busy = false;
  uint64_t generation = 0;
  uint64_t checked_at = 0;
  bool checked = false;
  Z3_lbool status = Z3_L_UNDEF;
  void touch() { ++generation; }
  // Z3 reuses objective indices after a pop, so each objective also gets a
  // serial number; handles carry both and are refused once popped.
  std::vector<uint64_t> objectives;  // serial of the objective at each index
  std::vector<size_t> scopes;        // number of objectives at each push
  uint64_t next_objective = 0;
  uint64_t note_objective(unsigned idx) {
    while (objectives.size() <= idx) objectives.push_back(++next_objective);
    return objectives[idx];
  }
};

struct FixedpointHolder {
  Ref<Z3_fixedpoint> f;
  std::vector<CallbackPtr> callbacks;
  bool busy = false;
  bool external = false;        // Z3_fixedpoint_init was called
  uint64_t generation = 0;
  uint64_t checked_at = 0;
  bool checked = false;
  Z3_lbool status = Z3_L_UNDEF;
};

enum class Kind { Context, Sort, Expr, FuncDecl, Solver, Model, Optimize, Goal, Tactic, Probe,
                  Simplifier, Fixedpoint, Rcf, Callback, Parser };

const char* kind_tag(Kind k);
const char* kind_class(Kind k);
const char* kind_label(Kind k);

template <typename T> void finalize_xptr(SEXP x) {
  T* p = static_cast<T*>(R_ExternalPtrAddr(x));
  if (p) {
    R_ClearExternalPtr(x);
    delete p;
  }
}

template <typename T> SEXP make_xptr(T* obj, Kind k) {
  std::unique_ptr<T> guard(obj);
  SEXP x = PROTECT(R_MakeExternalPtr(obj, Rf_install(kind_tag(k)), R_NilValue));
  guard.release();
  R_RegisterCFinalizerEx(x, finalize_xptr<T>, TRUE);
  Rf_setAttrib(x, R_ClassSymbol, Rf_mkString(kind_class(k)));
  UNPROTECT(1);
  return x;
}

template <typename T> T* get_xptr(SEXP x, Kind k) {
  if (TYPEOF(x) != EXTPTRSXP || R_ExternalPtrTag(x) != Rf_install(kind_tag(k))) {
    fail(std::string("expected ") + kind_label(k));
  }
  T* p = static_cast<T*>(R_ExternalPtrAddr(x));
  if (!p && k == Kind::Callback) {
    fail("this callback handle has expired; it is valid only during the callback that received it");
  }
  if (!p) {
    fail(std::string("this ") + kind_label(k) +
         " is no longer valid; Z3 objects cannot be saved and restored "
         "across R sessions");
  }
  return p;
}

// Solvers and optimizers as seen by every entry point. While one runs Z3
// code that may call R callbacks, every other use of it is refused: a
// callback that changed or reentered its own solver would corrupt the
// search. Callbacks act through their callback handle instead.
template <typename H> H* holder_of(SEXP x, Kind k, const char* what) {
  H* h = get_xptr<H>(x, k);
  if (h->busy) {
    fail(std::string("this ") + what + " is running a check and cannot be used from its own callbacks; "
         "use the callback handle instead");
  }
  return h;
}
inline SolverHolder* solver_of(SEXP x) { return holder_of<SolverHolder>(x, Kind::Solver, "solver"); }
inline OptimizeHolder* optimizer_of(SEXP x) { return holder_of<OptimizeHolder>(x, Kind::Optimize, "optimizer"); }
inline FixedpointHolder* fixedpoint_of(SEXP x) {
  return holder_of<FixedpointHolder>(x, Kind::Fixedpoint, "fixedpoint engine");
}

// Marks a holder busy for the duration of a Z3 call that may run callbacks,
// and clears errors left from earlier calls.
template <typename H> struct BusyGuard {
  H* h;
  explicit BusyGuard(H* holder) : h(holder) {
    h->busy = true;
    for (CallbackPtr& cb : h->callbacks) {
      cb->error.clear();
      cb->interrupted = false;
    }
  }
  ~BusyGuard() { h->busy = false; }
  BusyGuard(const BusyGuard&) = delete;
  BusyGuard& operator=(const BusyGuard&) = delete;
};

inline bool is_kind(SEXP x, Kind k) {
  return TYPEOF(x) == EXTPTRSXP && R_ExternalPtrTag(x) == Rf_install(kind_tag(k));
}

// Convenience accessors
CtxPtr get_ctx(SEXP x);     // from a z3_context object
CtxPtr ctx_of_any(SEXP x);  // from any z3r object
CtxPtr fixedpoint_ctx(SEXP x);
CtxPtr rcf_ctx(SEXP x);
inline const Ast& get_expr(SEXP x) { return *get_xptr<Ast>(x, Kind::Expr); }
inline const Ast& get_sort(SEXP x) { return *get_xptr<Ast>(x, Kind::Sort); }
inline const Ast& get_decl(SEXP x) { return *get_xptr<Ast>(x, Kind::FuncDecl); }

SEXP wrap_ctx(const CtxPtr& ctx);
SEXP wrap_expr(const Ast& a);
SEXP wrap_sort(const Ast& a);
SEXP wrap_decl(const Ast& a);
SEXP wrap_expr_list(const AstVec& v);
SEXP wrap_ast_vector(const CtxPtr& ctx, Z3_ast_vector v);  // checks errors first

void same_ctx(const CtxPtr& a, const CtxPtr& b);

// Strings and symbols
std::string as_string(SEXP x, const char* what);  // single non-NA string, UTF-8
Z3_symbol mk_symbol(const CtxPtr& ctx, const std::string& name);
std::string symbol_string(const CtxPtr& ctx, Z3_symbol s);

// Sort helpers
Ast sort_of(const Ast& e);
Z3_sort_kind sort_kind(const Ast& e);
std::string sort_name(const CtxPtr& ctx, Z3_sort s);
std::string to_string(const Ast& a);

// Literal lifting (see expressions.cpp): converts an R scalar or a z3_expr
// into an AST in ctx. `hint` (may be null) is the sort of the other operand.
Ast lift(const CtxPtr& ctx, SEXP x, Z3_sort hint);
// Finds the context of the first z3_expr in a list; fails if there is none.
CtxPtr ctx_from_list(SEXP args);
// Shared helpers (ast.cpp)
Ref<Z3_ast_vector> make_ast_vector(const CtxPtr& ctx, const AstVec& v);
AstVec expr_list(const CtxPtr& ctx, SEXP lst, const char* what);

// Shared helpers (solver.cpp)
const char* lbool_string(Z3_lbool r);
Ref<Z3_params> make_params(const CtxPtr& ctx, Z3_param_descrs descrs, SEXP values);
SEXP describe_params(const CtxPtr& ctx, Z3_param_descrs raw_descrs);
SEXP stats_list(const CtxPtr& ctx, Z3_stats raw_stats);
// Z3 handles SIGINT itself during checks and returns unknown with this
// reason; z3r turns that into an ordinary R interrupt.
void forward_interrupt(Z3_lbool status, const std::string& reason);
bool has_propagator(const CallbackPtr& cb);
void refuse_copy_with_callbacks(SolverHolder* h, const char* what);

// Floating point (fpa.cpp)
bool is_fp(const Ast& e);
bool is_rm(const Ast& e);
bool fp_to_double(const Ast& a, double* out);
Ast fp_literal(const CtxPtr& ctx, double v, bool is_na, Z3_sort sort);
Ast fp_binary(const CtxPtr& ctx, const std::string& op, const Ast& a, const Ast& b);

bool is_bool(const Ast& e);
bool is_arith(const Ast& e);
Ast to_real(const Ast& e);
// Converts an Int to Real when `sort` is Real; fails on any other mismatch.
Ast coerce_sort(const Ast& v, Z3_sort sort, const std::string& what);
// Sorts of the free de Bruijn variables of an expression, by index (null for
// indices that do not occur).
std::vector<Z3_sort> free_var_sorts(Z3_context c, Z3_ast a);

}  // namespace z3r

#endif
