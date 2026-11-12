// AST maps (dictionaries keyed by Z3 terms) and AST vector utilities.
#include "z3r.h"

using namespace z3r;

namespace {

typedef Ref<Z3_ast_map> AstMap;

SEXP wrap_map(AstMap m) { return make_xptr(new AstMap(std::move(m)), Kind::AstMap); }

const AstMap& ast_map(SEXP x) { return *get_xptr<AstMap>(x, Kind::AstMap); }

// Keys and values are expressions of the map's context. R scalars are not
// lifted: a number would be ambiguous between Int and Real keys.
const Ast& term(const CtxPtr& ctx, SEXP x, const char* what) {
  if (!is_kind(x, Kind::Expr)) fail(std::string(what) + " must be a Z3 expression (z3_expr)");
  const Ast& a = get_expr(x);
  same_ctx(ctx, a.ctx());
  return a;
}

}  // namespace

// [[Rcpp::export(rng = false)]]
SEXP cpp_mk_ast_map(SEXP ctx_) {
  CtxPtr ctx = get_ctx(ctx_);
  return wrap_map(own(ctx, Z3_mk_ast_map(ctx->c)));
}

// Stores value under key, replacing any value already there.
// [[Rcpp::export(rng = false)]]
void cpp_ast_map_insert(SEXP m_, SEXP key_, SEXP value_) {
  const AstMap& m = ast_map(m_);
  const CtxPtr& ctx = m.ctx();
  const Ast& k = term(ctx, key_, "the key");
  const Ast& v = term(ctx, value_, "the value");
  Z3_ast_map_insert(ctx->c, m, k, v);
  check(ctx);
}

// [[Rcpp::export(rng = false)]]
bool cpp_ast_map_contains(SEXP m_, SEXP key_) {
  const AstMap& m = ast_map(m_);
  const CtxPtr& ctx = m.ctx();
  bool has = Z3_ast_map_contains(ctx->c, m, term(ctx, key_, "the key"));
  check(ctx);
  return has;
}

// Value stored under key, or NULL when the key is absent (Z3_ast_map_find
// reports an absent key as an error).
// [[Rcpp::export(rng = false)]]
SEXP cpp_ast_map_find(SEXP m_, SEXP key_) {
  const AstMap& m = ast_map(m_);
  const CtxPtr& ctx = m.ctx();
  const Ast& k = term(ctx, key_, "the key");
  bool has = Z3_ast_map_contains(ctx->c, m, k);
  check(ctx);
  if (!has) return R_NilValue;
  return wrap_expr(own_ast(ctx, Z3_ast_map_find(ctx->c, m, k)));
}

// Removing an absent key does nothing.
// [[Rcpp::export(rng = false)]]
void cpp_ast_map_erase(SEXP m_, SEXP key_) {
  const AstMap& m = ast_map(m_);
  const CtxPtr& ctx = m.ctx();
  Z3_ast_map_erase(ctx->c, m, term(ctx, key_, "the key"));
  check(ctx);
}

// [[Rcpp::export(rng = false)]]
void cpp_ast_map_reset(SEXP m_) {
  const AstMap& m = ast_map(m_);
  Z3_ast_map_reset(m.c(), m);
  check(m.ctx());
}

// [[Rcpp::export(rng = false)]]
int cpp_ast_map_size(SEXP m_) {
  const AstMap& m = ast_map(m_);
  unsigned n = Z3_ast_map_size(m.c(), m);
  check(m.ctx());
  return static_cast<int>(n);
}

// Keys and the values stored under them, in the same order.
// [[Rcpp::export(rng = false)]]
SEXP cpp_ast_map_entries(SEXP m_) {
  const AstMap& m = ast_map(m_);
  const CtxPtr& ctx = m.ctx();
  Z3_context c = ctx->c;
  Ref<Z3_ast_vector> keys = own(ctx, Z3_ast_map_keys(c, m));
  unsigned n = Z3_ast_vector_size(c, keys);
  check(ctx);
  AstVec ks, vs;
  for (unsigned i = 0; i < n; ++i) {
    Ast k = own_ast(ctx, Z3_ast_vector_get(c, keys, i));
    vs.push_back(own_ast(ctx, Z3_ast_map_find(c, m, k)));
    ks.push_back(k);
  }
  Rcpp::List kl = wrap_expr_list(ks);
  Rcpp::List vl = wrap_expr_list(vs);
  return Rcpp::List::create(Rcpp::Named("keys") = kl, Rcpp::Named("values") = vl);
}

// [[Rcpp::export(rng = false)]]
Rcpp::String cpp_ast_map_to_string(SEXP m_) {
  const AstMap& m = ast_map(m_);
  std::string out = str(Z3_ast_map_to_string(m.c(), m));
  check(m.ctx());
  return out;
}

// Z3's text for a vector of terms.
// [[Rcpp::export(rng = false)]]
Rcpp::String cpp_ast_vector_to_string(Rcpp::List terms) {
  CtxPtr ctx = ctx_from_list(terms);
  Ref<Z3_ast_vector> v = make_ast_vector(ctx, expr_list(ctx, terms, "terms"));
  std::string out = str(Z3_ast_vector_to_string(ctx->c, v));
  check(ctx);
  return out;
}
