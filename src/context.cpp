#include "z3r.h"

#include <cerrno>
#include <cstdlib>

#if defined(__has_include)
#if __has_include(<z3_version.h>)
#include <z3_version.h>
#endif
#endif

namespace z3r {

void fail(const std::string& msg) { throw Rcpp::exception(msg.c_str(), false); }

void needs_version(const char* feature, const char* version) {
  fail(std::string(feature) + " needs Z3 " + version +
       " or later; z3r was built with an older Z3. Reinstall z3r against a newer Z3.");
}

void Context::note_param(const std::string& name, const std::string& value) {
  if (name != "timeout") return;
  char* end = nullptr;
  errno = 0;
  unsigned long long v = std::strtoull(value.c_str(), &end, 10);
  if (errno == 0 && end != value.c_str() && *end == '\0' && v <= 4294967295ull) {
    timeout = static_cast<unsigned>(v);
  }
}

Context::Context(const std::vector<std::pair<std::string, std::string>>& config) {
  Z3_config cfg = Z3_mk_config();
  if (!cfg) fail("could not create a Z3 configuration");
  Z3_set_param_value(cfg, "model", "true");
  for (const auto& kv : config) {
    Z3_set_param_value(cfg, kv.first.c_str(), kv.second.c_str());
    note_param(kv.first, kv.second);
  }
  c = Z3_mk_context_rc(cfg);
  Z3_del_config(cfg);
  if (!c) fail("could not create a Z3 context");
  // The default handler calls exit(); errors are checked explicitly instead.
  Z3_set_error_handler(c, nullptr);
}

Context::~Context() {
  proof_check.reset();
  if (handle != R_NilValue) R_ReleaseObject(handle);
  Z3_del_context(c);
}

ContextHolder::~ContextHolder() {
  // The proof checker's R function lives in this handle's protected slot;
  // unregister it before the handle goes.
#if Z3R_AT_LEAST(5, 3, 2)
  if (ctx && ctx->proof_check && ctx->proof_check->owner == this) {
    Z3_set_proof_check_callback(ctx->c, nullptr, nullptr);
    Z3_set_error(ctx->c, Z3_OK);
    ctx->proof_check.reset();
  }
#endif
}

void keep_callbacks(SEXP owner, SEXP fns) {
  R_SetExternalPtrProtected(owner, Rf_cons(fns, R_ExternalPtrProtected(owner)));
}

void drop_callbacks(SEXP owner) { R_SetExternalPtrProtected(owner, R_NilValue); }

void check(Z3_context c) {
  Z3_error_code e = Z3_get_error_code(c);
  if (e != Z3_OK) {
    std::string msg = str(Z3_get_error_msg(c, e));
    Z3_set_error(c, Z3_OK);
    fail("Z3 error: " + msg);
  }
}

const char* kind_tag(Kind k) {
  switch (k) {
    case Kind::Context: return "z3r_context";
    case Kind::Sort: return "z3r_sort";
    case Kind::Expr: return "z3r_expr";
    case Kind::FuncDecl: return "z3r_func_decl";
    case Kind::Solver: return "z3r_solver";
    case Kind::Model: return "z3r_model";
    case Kind::Optimize: return "z3r_optimize";
    case Kind::Goal: return "z3r_goal";
    case Kind::Tactic: return "z3r_tactic";
    case Kind::Probe: return "z3r_probe";
    case Kind::Simplifier: return "z3r_simplifier";
    case Kind::Fixedpoint: return "z3r_fixedpoint";
    case Kind::Rcf: return "z3r_rcf";
    case Kind::Callback: return "z3r_callback";
    case Kind::Parser: return "z3r_parser";
    case Kind::AstMap: return "z3r_ast_map";
  }
  return "z3r_unknown";
}

const char* kind_class(Kind k) {
  switch (k) {
    case Kind::Context: return "z3_context";
    case Kind::Sort: return "z3_sort";
    case Kind::Expr: return "z3_expr";
    case Kind::FuncDecl: return "z3_func_decl";
    case Kind::Solver: return "z3_solver";
    case Kind::Model: return "z3_model";
    case Kind::Optimize: return "z3_optimize";
    case Kind::Goal: return "z3_goal";
    case Kind::Tactic: return "z3_tactic";
    case Kind::Probe: return "z3_probe";
    case Kind::Simplifier: return "z3_simplifier";
    case Kind::Fixedpoint: return "z3_fixedpoint";
    case Kind::Rcf: return "z3_rcf";
    case Kind::Callback: return "z3_callback";
    case Kind::Parser: return "z3_parser";
    case Kind::AstMap: return "z3_ast_map";
  }
  return "z3_unknown";
}

const char* kind_label(Kind k) {
  switch (k) {
    case Kind::Context: return "a Z3 context (z3_context)";
    case Kind::Sort: return "a Z3 sort (z3_sort)";
    case Kind::Expr: return "a Z3 expression (z3_expr)";
    case Kind::FuncDecl: return "a Z3 function declaration (z3_func_decl)";
    case Kind::Solver: return "a Z3 solver (z3_solver)";
    case Kind::Model: return "a Z3 model (z3_model)";
    case Kind::Optimize: return "a Z3 optimizer (z3_optimize)";
    case Kind::Goal: return "a Z3 goal (z3_goal)";
    case Kind::Tactic: return "a Z3 tactic (z3_tactic)";
    case Kind::Probe: return "a Z3 probe (z3_probe)";
    case Kind::Simplifier: return "a Z3 simplifier (z3_simplifier)";
    case Kind::Fixedpoint: return "a Z3 fixedpoint engine (z3_fixedpoint)";
    case Kind::Rcf: return "a real closed field number (z3_rcf)";
    case Kind::Callback: return "a solver callback handle (z3_callback)";
    case Kind::Parser: return "an SMT-LIB2 parser (z3_parser)";
    case Kind::AstMap: return "a Z3 AST map (z3_ast_map)";
  }
  return "a Z3 object";
}

CtxPtr get_ctx(SEXP x) { return get_xptr<ContextHolder>(x, Kind::Context)->ctx; }

SEXP wrap_ctx(const CtxPtr& ctx) {
  if (ctx->handle != R_NilValue) {
    SEXP key = R_WeakRefKey(ctx->handle);
    if (key != R_NilValue && R_ExternalPtrAddr(key)) return key;
  }
  ContextHolder* h = new ContextHolder();
  h->ctx = ctx;
  SEXP x = PROTECT(make_xptr(h, Kind::Context));
  SEXP w = PROTECT(R_MakeWeakRef(x, R_NilValue, R_NilValue, FALSE));
  if (ctx->handle != R_NilValue) R_ReleaseObject(ctx->handle);
  R_PreserveObject(w);
  ctx->handle = w;
  UNPROTECT(2);
  return x;
}
SEXP wrap_expr(const Ast& a) { return make_xptr(new Ast(a), Kind::Expr); }
SEXP wrap_sort(const Ast& a) { return make_xptr(new Ast(a), Kind::Sort); }
SEXP wrap_decl(const Ast& a) { return make_xptr(new Ast(a), Kind::FuncDecl); }

SEXP wrap_expr_list(const AstVec& v) {
  Rcpp::List out(v.size());
  for (size_t i = 0; i < v.size(); ++i) out[i] = wrap_expr(v[i]);
  return out;
}

SEXP wrap_ast_vector(const CtxPtr& ctx, Z3_ast_vector raw_v) {
  Ref<Z3_ast_vector> v = own(ctx, raw_v);
  unsigned n = Z3_ast_vector_size(ctx->c, v);
  AstVec items;
  items.reserve(n);
  for (unsigned i = 0; i < n; ++i) {
    items.push_back(own_ast(ctx, Z3_ast_vector_get(ctx->c, v, i)));
  }
  return wrap_expr_list(items);
}

std::vector<Z3_ast> raw(const AstVec& v) {
  std::vector<Z3_ast> out(v.size());
  for (size_t i = 0; i < v.size(); ++i) out[i] = v[i].get();
  return out;
}

void same_ctx(const CtxPtr& a, const CtxPtr& b) {
  if (a.get() != b.get()) {
    fail("Z3 objects belong to different contexts; objects can only be "
         "combined within the context that created them");
  }
}

std::string as_string(SEXP x, const char* what) {
  if (TYPEOF(x) != STRSXP || Rf_xlength(x) != 1 || STRING_ELT(x, 0) == NA_STRING) {
    fail(std::string(what) + " must be a single non-missing string");
  }
  return std::string(Rf_translateCharUTF8(STRING_ELT(x, 0)));
}

Z3_symbol mk_symbol(const CtxPtr& ctx, const std::string& name) {
  Z3_symbol s = Z3_mk_string_symbol(ctx->c, name.c_str());
  check(ctx);
  return s;
}

std::string symbol_string(const CtxPtr& ctx, Z3_symbol s) {
  if (Z3_get_symbol_kind(ctx->c, s) == Z3_INT_SYMBOL) {
    int i = Z3_get_symbol_int(ctx->c, s);
    check(ctx);
    return "k!" + std::to_string(i);
  }
  std::string out = str(Z3_get_symbol_string(ctx->c, s));
  check(ctx);
  return out;
}

Ast sort_of(const Ast& e) {
  return own_ast(e.ctx(), sort_ast(Z3_get_sort(e.c(), e)));
}

Z3_sort_kind sort_kind(const Ast& e) {
  Z3_sort_kind k = Z3_get_sort_kind(e.c(), Z3_get_sort(e.c(), e));
  check(e.ctx());
  return k;
}

std::string sort_name(const CtxPtr& ctx, Z3_sort s) {
  std::string out = str(Z3_sort_to_string(ctx->c, s));
  check(ctx);
  return out;
}

std::string to_string(const Ast& a) {
  std::string out = str(Z3_ast_to_string(a.c(), a));
  check(a.ctx());
  return out;
}

}  // namespace z3r

using namespace z3r;

// [[Rcpp::export(rng = false)]]
SEXP cpp_version() {
  unsigned major, minor, build, rev;
  Z3_get_version(&major, &minor, &build, &rev);
#ifdef Z3_MAJOR_VERSION
  std::string headers = std::to_string(Z3_MAJOR_VERSION) + "." + std::to_string(Z3_MINOR_VERSION) +
                        "." + std::to_string(Z3_BUILD_NUMBER) + "." + std::to_string(Z3_REVISION_NUMBER);
#else
  std::string headers = "unknown";
#endif
  return Rcpp::List::create(
      Rcpp::Named("runtime") = std::to_string(major) + "." + std::to_string(minor) +
                               "." + std::to_string(build) + "." + std::to_string(rev),
      Rcpp::Named("headers") = headers,
      Rcpp::Named("full") = str(Z3_get_full_version()));
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_mk_context(Rcpp::CharacterVector names, Rcpp::CharacterVector values, bool concurrent_dec_ref) {
  std::vector<std::pair<std::string, std::string>> config;
  for (R_xlen_t i = 0; i < names.size(); ++i) {
    config.emplace_back(Rcpp::as<std::string>(names[i]), Rcpp::as<std::string>(values[i]));
  }
  CtxPtr ctx = std::make_shared<Context>(config);
  if (concurrent_dec_ref) {
#if Z3R_AT_LEAST(4, 10, 0)
    Z3_enable_concurrent_dec_ref(ctx->c);
    check(ctx);
#else
    needs_version("concurrent_dec_ref", "4.10.0");
#endif
  }
  return wrap_ctx(ctx);
}

// Turns a trace tag on or off. Only debug builds of Z3 trace; release builds
// ignore both calls.
// [[Rcpp::export(rng = false)]]
void cpp_trace(SEXP tag_, bool enable) {
  std::string tag = as_string(tag_, "tag");
  if (enable) {
    Z3_enable_trace(tag.c_str());
  } else {
    Z3_disable_trace(tag.c_str());
  }
}

namespace z3r {
CtxPtr ctx_of_any(SEXP x) {
  if (is_kind(x, Kind::Context)) return get_ctx(x);
  if (is_kind(x, Kind::Expr)) return get_xptr<Ast>(x, Kind::Expr)->ctx();
  if (is_kind(x, Kind::Sort)) return get_xptr<Ast>(x, Kind::Sort)->ctx();
  if (is_kind(x, Kind::FuncDecl)) return get_xptr<Ast>(x, Kind::FuncDecl)->ctx();
  if (is_kind(x, Kind::Solver)) return get_xptr<SolverHolder>(x, Kind::Solver)->s.ctx();
  if (is_kind(x, Kind::Optimize)) return get_xptr<OptimizeHolder>(x, Kind::Optimize)->o.ctx();
  if (is_kind(x, Kind::Model)) return get_xptr<Ref<Z3_model>>(x, Kind::Model)->ctx();
  if (is_kind(x, Kind::Goal)) return get_xptr<Ref<Z3_goal>>(x, Kind::Goal)->ctx();
  if (is_kind(x, Kind::Tactic)) return get_xptr<Ref<Z3_tactic>>(x, Kind::Tactic)->ctx();
  if (is_kind(x, Kind::Probe)) return get_xptr<Ref<Z3_probe>>(x, Kind::Probe)->ctx();
  if (is_kind(x, Kind::Fixedpoint)) return fixedpoint_ctx(x);
  if (is_kind(x, Kind::Rcf)) return rcf_ctx(x);
  if (is_kind(x, Kind::AstMap)) return get_xptr<Ref<Z3_ast_map>>(x, Kind::AstMap)->ctx();
#if Z3R_AT_LEAST(4, 9, 0)
  if (is_kind(x, Kind::Parser)) return get_xptr<Ref<Z3_parser_context>>(x, Kind::Parser)->ctx();
#endif
#if Z3R_AT_LEAST(4, 12, 2)
  if (is_kind(x, Kind::Simplifier)) return get_xptr<Ref<Z3_simplifier>>(x, Kind::Simplifier)->ctx();
#endif
  fail("expected a z3r object");
}
}  // namespace z3r

// Context of any z3r object that carries one.
// [[Rcpp::export(rng = false)]]
SEXP cpp_context_of(SEXP x) {
  if (is_kind(x, Kind::Context)) return x;
  return wrap_ctx(ctx_of_any(x));
}

// TRUE when two z3r objects use the same Z3 context.
// [[Rcpp::export(rng = false)]]
bool cpp_same_context(SEXP a, SEXP b) {
  return ctx_of_any(a).get() == ctx_of_any(b).get();
}

// Whether an external pointer still points to a live object.
// [[Rcpp::export(rng = false)]]
bool cpp_is_valid(SEXP x) {
  return TYPEOF(x) == EXTPTRSXP && R_ExternalPtrAddr(x) != nullptr;
}
