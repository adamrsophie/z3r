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

// ---------------------------------------------------------------------------
// Building and transforming models

// [[Rcpp::export(rng = false)]]
SEXP cpp_mk_model(SEXP ctx_) {
  CtxPtr ctx = get_ctx(ctx_);
  return make_xptr(new Ref<Z3_model>(own(ctx, Z3_mk_model(ctx->c))), Kind::Model);
}

// Assigns a value to a constant (an application of a nullary declaration).
// [[Rcpp::export(rng = false)]]
void cpp_model_set_const(SEXP m_, SEXP constant, SEXP value) {
  const Ref<Z3_model>& m = model(m_);
  const CtxPtr& ctx = m.ctx();
  Z3_context c = ctx->c;
  const Ast& k = get_expr(constant);
  same_ctx(ctx, k.ctx());
  if (!Z3_is_app(c, k) || Z3_get_app_num_args(c, Z3_to_app(c, k)) != 0) {
    fail("only constants can be assigned in a model");
  }
  Z3_func_decl d = Z3_get_app_decl(c, Z3_to_app(c, k));
  Ast v = lift(ctx, value, Z3_get_sort(c, k));
  if (sort_kind(k) == Z3_REAL_SORT && sort_kind(v) == Z3_INT_SORT) v = to_real(v);
  if (!Z3_is_eq_sort(c, Z3_get_sort(c, k), Z3_get_sort(c, v))) fail("the value has the wrong sort");
  Z3_add_const_interp(c, m, d, v);
  check(ctx);
}

// Defines a function interpretation from (arguments, value) entries and a
// default value.
// [[Rcpp::export(rng = false)]]
void cpp_model_set_func(SEXP m_, SEXP decl_, Rcpp::List entries, SEXP otherwise) {
  const Ref<Z3_model>& m = model(m_);
  const CtxPtr& ctx = m.ctx();
  Z3_context c = ctx->c;
  const Ast& decl = get_decl(decl_);
  same_ctx(ctx, decl.ctx());
  Z3_func_decl f = as_decl(decl);
  unsigned arity = Z3_get_arity(c, f);
  Z3_sort range = Z3_get_range(c, f);
  Ast els = lift(ctx, otherwise, range);
  Ref<Z3_func_interp> fi = own(ctx, Z3_add_func_interp(c, m, f, els));
  for (R_xlen_t i = 0; i < entries.size(); ++i) {
    Rcpp::List e(entries[i]);
    Rcpp::List args = Rcpp::as<Rcpp::List>(e["args"]);
    if (static_cast<unsigned>(args.size()) != arity) fail("entry has the wrong number of arguments");
    AstVec av;
    for (unsigned j = 0; j < arity; ++j) av.push_back(lift(ctx, args[j], Z3_get_domain(c, f, j)));
    Ref<Z3_ast_vector> vec = make_ast_vector(ctx, av);
    Ast val = lift(ctx, e["value"], range);
    Z3_func_interp_add_entry(c, fi, vec, val);
    check(ctx);
  }
}

// [[Rcpp::export(rng = false)]]
bool cpp_model_has(SEXP m_, SEXP x) {
  const Ref<Z3_model>& m = model(m_);
  const CtxPtr& ctx = m.ctx();
  Z3_context c = ctx->c;
  Z3_func_decl d;
  if (is_kind(x, Kind::FuncDecl)) {
    d = as_decl(get_decl(x));
  } else {
    const Ast& k = get_expr(x);
    if (!Z3_is_app(c, k)) fail("expected a constant or a function declaration");
    d = Z3_get_app_decl(c, Z3_to_app(c, k));
  }
  bool has = Z3_model_has_interp(c, m, d);
  check(ctx);
  return has;
}

// Uninterpreted sorts of the model and their finite universes.
// [[Rcpp::export(rng = false)]]
SEXP cpp_model_universes(SEXP m_) {
  const Ref<Z3_model>& m = model(m_);
  const CtxPtr& ctx = m.ctx();
  Z3_context c = ctx->c;
  unsigned n = Z3_model_get_num_sorts(c, m);
  Rcpp::List out(n);
  Rcpp::CharacterVector names(n);
  for (unsigned i = 0; i < n; ++i) {
    Z3_sort s = Z3_model_get_sort(c, m, i);
    Ast sa = own_ast(ctx, sort_ast(s));
    names[i] = sort_name(ctx, s);
    out[i] = wrap_ast_vector(ctx, Z3_model_get_sort_universe(c, m, s));
  }
  out.names() = names;
  return out;
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_model_translate(SEXP m_, SEXP target_) {
  const Ref<Z3_model>& m = model(m_);
  CtxPtr target = get_ctx(target_);
  Z3_model t = Z3_model_translate(m.c(), m, target->c);
  check(m.ctx());
  return make_xptr(new Ref<Z3_model>(own(target, t)), Kind::Model);
}

// Function declaration behind an (_ as-array f) array value, or NULL.
// [[Rcpp::export(rng = false)]]
SEXP cpp_as_array_decl(SEXP x) {
  const Ast& a = get_expr(x);
  if (!Z3_is_as_array(a.c(), a)) return R_NilValue;
  return wrap_decl(own_ast(a.ctx(), decl_ast(Z3_get_as_array_func_decl(a.c(), a))));
}
