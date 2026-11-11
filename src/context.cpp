#include "z3r.h"

#if defined(__has_include)
#if __has_include(<z3_version.h>)
#include <z3_version.h>
#endif
#endif

namespace z3r {

void fail(const std::string& msg) { throw Rcpp::exception(msg.c_str(), false); }

Context::Context() {
  Z3_config cfg = Z3_mk_config();
  if (!cfg) fail("could not create a Z3 configuration");
  Z3_set_param_value(cfg, "model", "true");
  c = Z3_mk_context_rc(cfg);
  Z3_del_config(cfg);
  if (!c) fail("could not create a Z3 context");
  // The default handler calls exit(); errors are checked explicitly instead.
  Z3_set_error_handler(c, nullptr);
}

Context::~Context() { Z3_del_context(c); }

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
  }
  return "a Z3 object";
}

CtxPtr get_ctx(SEXP x) { return get_xptr<ContextHolder>(x, Kind::Context)->ctx; }

SEXP wrap_ctx(const CtxPtr& ctx) {
  ContextHolder* h = new ContextHolder();
  h->ctx = ctx;
  return make_xptr(h, Kind::Context);
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
SEXP cpp_mk_context() {
  CtxPtr ctx = std::make_shared<Context>();
  return wrap_ctx(ctx);
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
