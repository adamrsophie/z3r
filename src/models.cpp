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
Rcpp::String cpp_model_to_string(SEXP m_) {
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
    names[i] = Rcpp::String(symbol_string(ctx, Z3_get_decl_name(c, d)));
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
    names[i] = Rcpp::String(symbol_string(ctx, Z3_get_decl_name(c, d)));
    Ref<Z3_func_interp> fi = own(ctx, Z3_model_get_func_interp(c, m, d));
    unsigned arity = Z3_func_interp_get_arity(c, fi);
    unsigned ne = Z3_func_interp_get_num_entries(c, fi);
    check(ctx);
    Rcpp::List entries(ne);
    for (unsigned j = 0; j < ne; ++j) {
      Ref<Z3_func_entry> e = own(ctx, Z3_func_interp_get_entry(c, fi, j));
      if (Z3_func_entry_get_num_args(c, e) != arity) {
        fail("Z3 returned a function entry whose number of arguments differs from the arity of its "
             "interpretation");
      }
      AstVec args;
      for (unsigned k = 0; k < arity; ++k) args.push_back(own_ast(ctx, Z3_func_entry_get_arg(c, e, k)));
      Ast value = own_ast(ctx, Z3_func_entry_get_value(c, e));
      entries[j] = Rcpp::List::create(Rcpp::Named("args") = wrap_expr_list(args),
                                      Rcpp::Named("value") = wrap_expr(value));
    }
    // Partial models (model.partial) have no default value.
    Z3_ast raw_else = Z3_func_interp_get_else(c, fi);
    check(ctx);
    Rcpp::RObject els = raw_else ? wrap_expr(own_ast(ctx, raw_else)) : R_NilValue;
    out[i] = Rcpp::List::create(Rcpp::Named("decl") = wrap_decl(decl),
                                Rcpp::Named("entries") = entries,
                                Rcpp::Named("else") = els);
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

// Converts a value for a model to the sort it must have: an Int becomes a
// Real where a Real is expected; any other mismatch is an error (Z3 would
// accept an ill-sorted interpretation).
static Ast model_value(const CtxPtr& ctx, SEXP x, Z3_sort sort, const char* what) {
  return coerce_sort(lift(ctx, x, sort), sort, what);
}

// The default value of an interpretation of f with `arity` arguments may use
// bound variable i for argument i + 1; checks their indices and sorts (Z3
// would coerce them).
static void check_default(Z3_context c, Z3_func_decl f, unsigned arity, const Ast& els) {
  std::vector<Z3_sort> fv = free_var_sorts(c, els);
  for (unsigned j = 0; j < fv.size(); ++j) {
    if (!fv[j]) continue;
    if (j >= arity || !Z3_is_eq_sort(c, fv[j], Z3_get_domain(c, f, j))) {
      fail("the default value uses bound variable " + std::to_string(j) +
           ", which is not an argument of this function with that sort");
    }
  }
}

// Z3 caches the values of shared subterms between evaluations in a model
// and clears that cache only when the model completion flag changes, so a
// changed model could report old values. Evaluating a constant once with
// each flag changes it, and clears the cache.
static void clear_eval_cache(const Ref<Z3_model>& m) {
  Z3_context c = m.c();
  Ast t = own_ast(m.ctx(), Z3_mk_true(c));
  Z3_ast out = nullptr;
  Z3_model_eval(c, m, t, true, &out);
  check(m.ctx());
  Z3_model_eval(c, m, t, false, &out);
  check(m.ctx());
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
  Ast v = model_value(ctx, value, Z3_get_sort(c, k), "the value");
  Z3_add_const_interp(c, m, d, v);
  check(ctx);
  clear_eval_cache(m);
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
  if (arity == 0) fail("this declaration has no arguments; use z3_model_set() for constants");
  Z3_sort range = Z3_get_range(c, f);
  Ast els = model_value(ctx, otherwise, range, "the default value");
  check_default(c, f, arity, els);
  auto ground = [&](const Ast& e, const char* what) {
    std::vector<Z3_sort> v = free_var_sorts(c, e);
    for (Z3_sort srt : v) {
      if (srt) fail(std::string(what) + " must not contain bound variables");
    }
    return e;
  };
  // Convert and check every entry before the model changes.
  std::vector<Ref<Z3_ast_vector>> keys;
  AstVec vals;
  for (R_xlen_t i = 0; i < entries.size(); ++i) {
    Rcpp::List e(entries[i]);
    Rcpp::List args = Rcpp::as<Rcpp::List>(e["args"]);
    if (static_cast<unsigned>(args.size()) != arity) fail("entry has the wrong number of arguments");
    AstVec av;
    for (unsigned j = 0; j < arity; ++j) {
      av.push_back(ground(model_value(ctx, args[j], Z3_get_domain(c, f, j), "an entry argument"), "an entry argument"));
    }
    keys.push_back(make_ast_vector(ctx, av));
    vals.push_back(ground(model_value(ctx, e["value"], range, "an entry value"), "an entry value"));
  }
  Ref<Z3_func_interp> fi = own(ctx, Z3_add_func_interp(c, m, f, els));
  for (size_t i = 0; i < keys.size(); ++i) {
    Z3_func_interp_add_entry(c, fi, keys[i], vals[i]);
    check(ctx);
  }
  clear_eval_cache(m);
}

// Replaces the default value of the model's interpretation of a function,
// keeping its entries.
// [[Rcpp::export(rng = false)]]
void cpp_model_set_else(SEXP m_, SEXP decl_, SEXP otherwise) {
  const Ref<Z3_model>& m = model(m_);
  const CtxPtr& ctx = m.ctx();
  Z3_context c = ctx->c;
  const Ast& decl = get_decl(decl_);
  same_ctx(ctx, decl.ctx());
  Z3_func_decl f = as_decl(decl);
  if (Z3_get_arity(c, f) == 0) fail("this declaration has no arguments; use z3_model_set() for constants");
  bool has = Z3_model_has_interp(c, m, f);
  check(ctx);
  // Z3_model_get_func_interp returns null without an error in that case.
  if (!has) fail("the model has no interpretation of this function; define one with z3_model_set_function()");
  Ref<Z3_func_interp> fi = own(ctx, Z3_model_get_func_interp(c, m, f));
  unsigned arity = Z3_func_interp_get_arity(c, fi);
  check(ctx);
  Ast els = model_value(ctx, otherwise, Z3_get_range(c, f), "the default value");
  check_default(c, f, arity, els);
#if !Z3R_AT_LEAST(4, 12, 2)
  // Before Z3 4.12.2, set_else replaces existing entries with the ones it
  // reads off a default of the form ite(var == value, result, rest); put
  // them back afterwards (adding an entry overwrites one).
  std::vector<Ref<Z3_ast_vector>> keys;
  AstVec vals;
  for (unsigned j = 0, ne = Z3_func_interp_get_num_entries(c, fi); j < ne; ++j) {
    Ref<Z3_func_entry> e = own(ctx, Z3_func_interp_get_entry(c, fi, j));
    Ref<Z3_ast_vector> args = own(ctx, Z3_mk_ast_vector(c));
    for (unsigned k = 0, na = Z3_func_entry_get_num_args(c, e); k < na; ++k) {
      Z3_ast_vector_push(c, args, Z3_func_entry_get_arg(c, e, k));
    }
    keys.push_back(args);
    vals.push_back(own_ast(ctx, Z3_func_entry_get_value(c, e)));
  }
#endif
  Z3_func_interp_set_else(c, fi, els);
  check(ctx);
#if !Z3R_AT_LEAST(4, 12, 2)
  for (size_t j = 0; j < keys.size(); ++j) {
    Z3_func_interp_add_entry(c, fi, keys[j], vals[j]);
    check(ctx);
  }
#endif
  clear_eval_cache(m);
}

// [[Rcpp::export(rng = false)]]
bool cpp_model_has(SEXP m_, SEXP x) {
  const Ref<Z3_model>& m = model(m_);
  const CtxPtr& ctx = m.ctx();
  Z3_context c = ctx->c;
  Z3_func_decl d;
  if (is_kind(x, Kind::FuncDecl)) {
    same_ctx(ctx, get_decl(x).ctx());
    d = as_decl(get_decl(x));
  } else {
    const Ast& k = get_expr(x);
    same_ctx(ctx, k.ctx());
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

// ---------------------------------------------------------------------------
// Model-based projection, model extrapolation and light quantifier
// elimination (z3_spacer.h).

bool cpp_is_const(SEXP x);

namespace {

std::vector<Z3_app> projection_vars(const CtxPtr& ctx, Rcpp::List vars, AstVec& keep) {
  std::vector<Z3_app> out;
  for (R_xlen_t i = 0; i < vars.size(); ++i) {
    SEXP v = vars[i];
    if (!is_kind(v, Kind::Expr) || !cpp_is_const(v)) {
      fail("variables must be constants such as those made by z3_int()");
    }
    const Ast& a = get_expr(v);
    same_ctx(ctx, a.ctx());
    keep.push_back(a);
    out.push_back(Z3_to_app(ctx->c, a));
  }
  return out;
}

// Projection and extrapolation assume the model satisfies the formula and
// return meaningless results otherwise.
void require_true_in(const Ref<Z3_model>& m, const Ast& fml) {
  Z3_context c = m.c();
  Z3_ast out = nullptr;
  bool ok = Z3_model_eval(c, m, fml, true, &out);
  check(m.ctx());
  if (!ok || !out || Z3_get_bool_value(c, out) != Z3_L_TRUE) fail("the formula must be true in the model");
}

}  // namespace

// method: "plain", "skolem" or "witness". The last two also return the
// variable bindings Z3 records in an AST map.
// [[Rcpp::export(rng = false)]]
SEXP cpp_model_project(SEXP m_, Rcpp::List vars, SEXP body_, std::string method) {
  const Ref<Z3_model>& m = model(m_);
  const CtxPtr& ctx = m.ctx();
  Z3_context c = ctx->c;
  Ast body = lift(ctx, body_, Z3_mk_bool_sort(c));
  if (!is_bool(body)) fail("the formula to project must be Boolean");
  require_true_in(m, body);
  AstVec keep;
  std::vector<Z3_app> b = projection_vars(ctx, vars, keep);
  unsigned n = static_cast<unsigned>(b.size());
  const Z3_app* bp = b.empty() ? nullptr : b.data();
  if (method == "plain") return wrap_expr(own_ast(ctx, Z3_qe_model_project(c, m, n, bp, body)));
  Ref<Z3_ast_map> map = own(ctx, Z3_mk_ast_map(c));
  Z3_ast r = nullptr;
  if (method == "skolem") {
    r = Z3_qe_model_project_skolem(c, m, n, bp, body, map);
  } else {
#if Z3R_AT_LEAST(4, 13, 4)
    r = Z3_qe_model_project_with_witness(c, m, n, bp, body, map);
#else
    needs_version("z3_model_project(method = \"witness\")", "4.13.4");
#endif
  }
  Ast res = own_ast(ctx, r);
  Ref<Z3_ast_vector> keys = own(ctx, Z3_ast_map_keys(c, map));
  unsigned k = Z3_ast_vector_size(c, keys);
  AstVec from, to;
  for (unsigned i = 0; i < k; ++i) {
    Ast key = own_ast(ctx, Z3_ast_vector_get(c, keys, i));
    to.push_back(own_ast(ctx, Z3_ast_map_find(c, map, key)));
    from.push_back(key);
  }
  return Rcpp::List::create(Rcpp::Named("formula") = wrap_expr(res), Rcpp::Named("vars") = wrap_expr_list(from),
                            Rcpp::Named("terms") = wrap_expr_list(to));
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_model_extrapolate(SEXP m_, SEXP fml_) {
  const Ref<Z3_model>& m = model(m_);
  const CtxPtr& ctx = m.ctx();
  Ast fml = lift(ctx, fml_, Z3_mk_bool_sort(ctx->c));
  if (!is_bool(fml)) fail("the formula must be Boolean");
  require_true_in(m, fml);
  return wrap_expr(own_ast(ctx, Z3_model_extrapolate(ctx->c, m, fml)));
}

// Returns the result and the variables that were not eliminated.
// [[Rcpp::export(rng = false)]]
SEXP cpp_qe_lite(Rcpp::List vars, SEXP body_) {
  const Ast& body = get_expr(body_);
  const CtxPtr& ctx = body.ctx();
  Z3_context c = ctx->c;
  if (!is_bool(body)) fail("the formula must be Boolean");
  AstVec keep;
  projection_vars(ctx, vars, keep);
  Ref<Z3_ast_vector> v = make_ast_vector(ctx, keep);
  Ast res = own_ast(ctx, Z3_qe_lite(c, v, body));
  AstVec left;
  unsigned k = Z3_ast_vector_size(c, v);
  for (unsigned i = 0; i < k; ++i) left.push_back(own_ast(ctx, Z3_ast_vector_get(c, v, i)));
  return Rcpp::List::create(Rcpp::Named("formula") = wrap_expr(res), Rcpp::Named("remaining") = wrap_expr_list(left));
}
