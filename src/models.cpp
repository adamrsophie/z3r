// Models: evaluation and inspection.
#include "z3r.h"

using namespace z3r;

static const Ref<Z3_model>& model(SEXP x) { return *get_xptr<Ref<Z3_model>>(x, Kind::Model); }

// [[Rcpp::export(rng = false)]]
SEXP cpp_model_eval(SEXP m_, SEXP x, bool completion) {
  const Ref<Z3_model>& m = model(m_);
  const CtxPtr& ctx = m.ctx();
  Ast a = lift(ctx, x, nullptr);
  Z3_ast out = nullptr;
  bool ok = Z3_model_eval(ctx->c, m, a, completion, &out);
  check(ctx);
  if (!ok || !out) fail("Z3 could not evaluate the expression in this model");
  return wrap_expr(own_ast(ctx, out));
}

// [[Rcpp::export(rng = false)]]
std::string cpp_model_to_string(SEXP m_) {
  const Ref<Z3_model>& m = model(m_);
  std::string out = str(Z3_model_to_string(m.c(), m));
  check(m.ctx());
  return out;
}

// Constants interpreted by the model, as a list of (name, constant, value).
// [[Rcpp::export(rng = false)]]
SEXP cpp_model_consts(SEXP m_) {
  const Ref<Z3_model>& m = model(m_);
  const CtxPtr& ctx = m.ctx();
  Z3_context c = ctx->c;
  unsigned n = Z3_model_get_num_consts(c, m);
  check(ctx);
  Rcpp::CharacterVector names(n);
  Rcpp::List consts(n), values(n);
  for (unsigned i = 0; i < n; ++i) {
    Z3_func_decl d = Z3_model_get_const_decl(c, m, i);
    Ast decl = own_ast(ctx, decl_ast(d));
    names[i] = symbol_string(ctx, Z3_get_decl_name(c, d));
    consts[i] = wrap_expr(own_ast(ctx, Z3_mk_app(c, d, 0, nullptr)));
    values[i] = wrap_expr(own_ast(ctx, Z3_model_get_const_interp(c, m, d)));
  }
  return Rcpp::List::create(Rcpp::Named("names") = names, Rcpp::Named("consts") = consts,
                            Rcpp::Named("values") = values);
}

// Function interpretations: for each function, its entries and else value.
// [[Rcpp::export(rng = false)]]
SEXP cpp_model_funcs(SEXP m_) {
  const Ref<Z3_model>& m = model(m_);
  const CtxPtr& ctx = m.ctx();
  Z3_context c = ctx->c;
  unsigned n = Z3_model_get_num_funcs(c, m);
  check(ctx);
  Rcpp::List out(n);
  Rcpp::CharacterVector names(n);
  for (unsigned i = 0; i < n; ++i) {
    Z3_func_decl d = Z3_model_get_func_decl(c, m, i);
    Ast decl = own_ast(ctx, decl_ast(d));
    names[i] = symbol_string(ctx, Z3_get_decl_name(c, d));
    Ref<Z3_func_interp> fi = own(ctx, Z3_model_get_func_interp(c, m, d));
    unsigned ne = Z3_func_interp_get_num_entries(c, fi);
    Rcpp::List entries(ne);
    for (unsigned j = 0; j < ne; ++j) {
      Z3_func_entry e = Z3_func_interp_get_entry(c, fi, j);
      check(ctx);
      Z3_func_entry_inc_ref(c, e);
      unsigned na = Z3_func_entry_get_num_args(c, e);
      AstVec args;
      for (unsigned k = 0; k < na; ++k) args.push_back(own_ast(ctx, Z3_func_entry_get_arg(c, e, k)));
      Ast value = own_ast(ctx, Z3_func_entry_get_value(c, e));
      Z3_func_entry_dec_ref(c, e);
      entries[j] = Rcpp::List::create(Rcpp::Named("args") = wrap_expr_list(args),
                                      Rcpp::Named("value") = wrap_expr(value));
    }
    Ast els = own_ast(ctx, Z3_func_interp_get_else(c, fi));
    out[i] = Rcpp::List::create(Rcpp::Named("decl") = wrap_decl(decl),
                                Rcpp::Named("entries") = entries,
                                Rcpp::Named("else") = wrap_expr(els));
  }
  out.names() = names;
  return out;
}
