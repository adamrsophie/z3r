// Inspecting and transforming expressions; context-wide settings.
#include "z3r.h"
#include "decl_kinds.h"

using namespace z3r;

namespace z3r {

Ref<Z3_ast_vector> make_ast_vector(const CtxPtr& ctx, const AstVec& v) {
  Ref<Z3_ast_vector> out = own(ctx, Z3_mk_ast_vector(ctx->c));
  for (const Ast& a : v) {
    same_ctx(ctx, a.ctx());
    Z3_ast_vector_push(ctx->c, out, a);
    check(ctx);
  }
  return out;
}

AstVec expr_list(const CtxPtr& ctx, SEXP lst, const char* what) {
  AstVec out;
  R_xlen_t n = Rf_xlength(lst);
  for (R_xlen_t i = 0; i < n; ++i) {
    SEXP e = VECTOR_ELT(lst, i);
    if (!is_kind(e, Kind::Expr)) fail(std::string(what) + " must be Z3 expressions");
    const Ast& a = get_expr(e);
    same_ctx(ctx, a.ctx());
    out.push_back(a);
  }
  return out;
}

}  // namespace z3r

static const char* ast_kind_name(Z3_ast_kind k) {
  switch (k) {
    case Z3_NUMERAL_AST: return "numeral";
    case Z3_APP_AST: return "app";
    case Z3_VAR_AST: return "var";
    case Z3_QUANTIFIER_AST: return "quantifier";
    case Z3_SORT_AST: return "sort";
    case Z3_FUNC_DECL_AST: return "func_decl";
    default: return "unknown";
  }
}

// [[Rcpp::export(rng = false)]]
std::string cpp_ast_kind(SEXP x) {
  const Ast& a = get_expr(x);
  Z3_ast_kind k = Z3_get_ast_kind(a.c(), a);
  check(a.ctx());
  return ast_kind_name(k);
}

// Application head and arguments. Numerals are applications in Z3.
// [[Rcpp::export(rng = false)]]
SEXP cpp_app_parts(SEXP x) {
  const Ast& a = get_expr(x);
  const CtxPtr& ctx = a.ctx();
  Z3_context c = ctx->c;
  if (!Z3_is_app(c, a)) fail("the expression is not an application");
  Z3_app app = Z3_to_app(c, a);
  Ast decl = own_ast(ctx, decl_ast(Z3_get_app_decl(c, app)));
  unsigned n = Z3_get_app_num_args(c, app);
  AstVec args;
  for (unsigned i = 0; i < n; ++i) args.push_back(own_ast(ctx, Z3_get_app_arg(c, app, i)));
  return Rcpp::List::create(Rcpp::Named("decl") = wrap_decl(decl),
                            Rcpp::Named("args") = wrap_expr_list(args));
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_decl_details(SEXP d_) {
  const Ast& d = get_decl(d_);
  const CtxPtr& ctx = d.ctx();
  Z3_context c = ctx->c;
  Z3_func_decl f = as_decl(d);
  Z3_decl_kind k = Z3_get_decl_kind(c, f);
  unsigned np = Z3_get_decl_num_parameters(c, f);
  Rcpp::List params(np);
  for (unsigned i = 0; i < np; ++i) {
    switch (Z3_get_decl_parameter_kind(c, f, i)) {
      case Z3_PARAMETER_INT: params[i] = Z3_get_decl_int_parameter(c, f, i); break;
      case Z3_PARAMETER_DOUBLE: params[i] = Z3_get_decl_double_parameter(c, f, i); break;
      case Z3_PARAMETER_RATIONAL: params[i] = str(Z3_get_decl_rational_parameter(c, f, i)); break;
      case Z3_PARAMETER_SYMBOL:
        params[i] = symbol_string(ctx, Z3_get_decl_symbol_parameter(c, f, i));
        break;
      case Z3_PARAMETER_SORT:
        params[i] = wrap_sort(own_ast(ctx, sort_ast(Z3_get_decl_sort_parameter(c, f, i))));
        break;
      case Z3_PARAMETER_AST:
        params[i] = wrap_expr(own_ast(ctx, Z3_get_decl_ast_parameter(c, f, i)));
        break;
      case Z3_PARAMETER_FUNC_DECL:
        params[i] = wrap_decl(own_ast(ctx, decl_ast(Z3_get_decl_func_decl_parameter(c, f, i))));
        break;
      default: params[i] = R_NilValue;
    }
    check(ctx);
  }
  return Rcpp::List::create(Rcpp::Named("kind") = decl_kind_name(k),
                            Rcpp::Named("interpreted") = (k != Z3_OP_UNINTERPRETED),
                            Rcpp::Named("parameters") = params);
}

// [[Rcpp::export(rng = false)]]
int cpp_var_index(SEXP x) {
  const Ast& a = get_expr(x);
  if (Z3_get_ast_kind(a.c(), a) != Z3_VAR_AST) fail("the expression is not a bound variable");
  unsigned i = Z3_get_index_value(a.c(), a);
  check(a.ctx());
  return static_cast<int>(i);
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_quantifier_parts(SEXP x) {
  const Ast& a = get_expr(x);
  const CtxPtr& ctx = a.ctx();
  Z3_context c = ctx->c;
  if (Z3_get_ast_kind(c, a) != Z3_QUANTIFIER_AST) fail("the expression is not a quantifier");
  std::string type = Z3_is_quantifier_forall(c, a) ? "forall"
                     : Z3_is_quantifier_exists(c, a) ? "exists"
                     : Z3_is_lambda(c, a) ? "lambda" : "unknown";
  unsigned n = Z3_get_quantifier_num_bound(c, a);
  Rcpp::CharacterVector names(n);
  Rcpp::List sorts(n);
  for (unsigned i = 0; i < n; ++i) {
    names[i] = symbol_string(ctx, Z3_get_quantifier_bound_name(c, a, i));
    sorts[i] = wrap_sort(own_ast(ctx, sort_ast(Z3_get_quantifier_bound_sort(c, a, i))));
  }
  unsigned np = Z3_get_quantifier_num_patterns(c, a);
  Rcpp::List patterns(np);
  for (unsigned i = 0; i < np; ++i) {
    Z3_pattern p = Z3_get_quantifier_pattern_ast(c, a, i);
    Ast pa = own_ast(ctx, Z3_pattern_to_ast(c, p));
    unsigned nt = Z3_get_pattern_num_terms(c, p);
    AstVec terms;
    for (unsigned j = 0; j < nt; ++j) terms.push_back(own_ast(ctx, Z3_get_pattern(c, p, j)));
    patterns[i] = wrap_expr_list(terms);
  }
  unsigned nnp = Z3_get_quantifier_num_no_patterns(c, a);
  AstVec nopats;
  for (unsigned i = 0; i < nnp; ++i) nopats.push_back(own_ast(ctx, Z3_get_quantifier_no_pattern_ast(c, a, i)));
  Ast body = own_ast(ctx, Z3_get_quantifier_body(c, a));
  unsigned weight = Z3_get_quantifier_weight(c, a);
  check(ctx);
  sorts.names() = names;
  Rcpp::List out = Rcpp::List::create(Rcpp::Named("type") = type, Rcpp::Named("names") = names,
                                      Rcpp::Named("sorts") = sorts, Rcpp::Named("body") = wrap_expr(body),
                                      Rcpp::Named("weight") = static_cast<int>(weight),
                                      Rcpp::Named("patterns") = patterns,
                                      Rcpp::Named("no_patterns") = wrap_expr_list(nopats));
#if Z3R_AT_LEAST(4, 12, 3)
  out["id"] = symbol_string(ctx, Z3_get_quantifier_id(c, a));
  out["skolem_id"] = symbol_string(ctx, Z3_get_quantifier_skolem_id(c, a));
#endif
  return out;
}

// De Bruijn variable for building quantifier bodies and function macros.
// [[Rcpp::export(rng = false)]]
SEXP cpp_mk_bound(int index, SEXP sort_) {
  const Ast& s = get_sort(sort_);
  if (index < 0) fail("index must be a non-negative integer");
  return wrap_expr(own_ast(s.ctx(), Z3_mk_bound(s.c(), static_cast<unsigned>(index), as_sort(s))));
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_substitute(SEXP x, Rcpp::List from, Rcpp::List to) {
  const Ast& a = get_expr(x);
  const CtxPtr& ctx = a.ctx();
  Z3_context c = ctx->c;
  if (from.size() != to.size()) fail("from and to must have the same length");
  AstVec f = expr_list(ctx, from, "from");
  AstVec t;
  for (R_xlen_t i = 0; i < to.size(); ++i) {
    Ast v = lift(ctx, to[i], Z3_get_sort(c, f[i]));
    if (sort_kind(f[i]) == Z3_REAL_SORT && sort_kind(v) == Z3_INT_SORT) v = to_real(v);
    if (!Z3_is_eq_sort(c, Z3_get_sort(c, f[i]), Z3_get_sort(c, v))) {
      fail("replacement " + std::to_string(i + 1) + " has a different sort from the term it replaces");
    }
    t.push_back(v);
  }
  std::vector<Z3_ast> rf = raw(f), rt = raw(t);
  return wrap_expr(own_ast(ctx, Z3_substitute(c, a, static_cast<unsigned>(rf.size()),
                                              rf.empty() ? nullptr : rf.data(),
                                              rt.empty() ? nullptr : rt.data())));
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_substitute_vars(SEXP x, Rcpp::List to) {
  const Ast& a = get_expr(x);
  AstVec t = expr_list(a.ctx(), to, "to");
  std::vector<Z3_ast> rt = raw(t);
  return wrap_expr(own_ast(a.ctx(), Z3_substitute_vars(a.c(), a, static_cast<unsigned>(rt.size()),
                                                       rt.empty() ? nullptr : rt.data())));
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_substitute_funs(SEXP x, Rcpp::List from, Rcpp::List to) {
#if Z3R_AT_LEAST(4, 9, 0)
  const Ast& a = get_expr(x);
  const CtxPtr& ctx = a.ctx();
  if (from.size() != to.size()) fail("from and to must have the same length");
  std::vector<Z3_func_decl> f;
  AstVec keep;
  for (R_xlen_t i = 0; i < from.size(); ++i) {
    const Ast& d = get_decl(from[i]);
    same_ctx(ctx, d.ctx());
    keep.push_back(d);
    f.push_back(as_decl(d));
  }
  AstVec t = expr_list(ctx, to, "to");
  std::vector<Z3_ast> rt = raw(t);
  return wrap_expr(own_ast(ctx, Z3_substitute_funs(ctx->c, a, static_cast<unsigned>(f.size()),
                                                   f.empty() ? nullptr : f.data(),
                                                   rt.empty() ? nullptr : rt.data())));
#else
  needs_version("z3_substitute_funs()", "4.9.0");
#endif
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_translate(SEXP x, SEXP target_) {
  const Ast& a = get_expr(x);
  CtxPtr target = get_ctx(target_);
  Z3_ast t = Z3_translate(a.c(), a, target->c);
  check(a.ctx());
  return wrap_expr(own_ast(target, t));
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_simplify_ex(SEXP x, SEXP params) {
  const Ast& a = get_expr(x);
  const CtxPtr& ctx = a.ctx();
  Ref<Z3_param_descrs> d = own(ctx, Z3_simplify_get_param_descrs(ctx->c));
  Ref<Z3_params> p = make_params(ctx, d, params);
  return wrap_expr(own_ast(ctx, Z3_simplify_ex(ctx->c, a, p)));
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_simplify_help(SEXP ctx_) {
  CtxPtr ctx = get_ctx(ctx_);
  return describe_params(ctx, Z3_simplify_get_param_descrs(ctx->c));
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_ast_id(SEXP x) {
  const Ast& a = get_expr(x);
  unsigned id = Z3_get_ast_id(a.c(), a);
  unsigned h = Z3_get_ast_hash(a.c(), a);
  check(a.ctx());
  return Rcpp::List::create(Rcpp::Named("id") = static_cast<double>(id),
                            Rcpp::Named("hash") = static_cast<double>(h));
}

// [[Rcpp::export(rng = false)]]
bool cpp_is_well_sorted(SEXP x) {
  const Ast& a = get_expr(x);
  bool ok = Z3_is_well_sorted(a.c(), a);
  check(a.ctx());
  return ok;
}

// [[Rcpp::export(rng = false)]]
bool cpp_is_ground(SEXP x) {
#if Z3R_AT_LEAST(4, 14, 0)
  const Ast& a = get_expr(x);
  bool g = Z3_is_ground(a.c(), a);
  check(a.ctx());
  return g;
#else
  needs_version("z3_is_ground()", "4.14.0");
#endif
}

// [[Rcpp::export(rng = false)]]
int cpp_depth(SEXP x) {
#if Z3R_AT_LEAST(4, 14, 0)
  const Ast& a = get_expr(x);
  unsigned d = Z3_get_depth(a.c(), a);
  check(a.ctx());
  return static_cast<int>(d);
#else
  needs_version("z3_depth()", "4.14.0");
#endif
}

// [[Rcpp::export(rng = false)]]
void cpp_set_print_mode(SEXP ctx_, std::string mode) {
  CtxPtr ctx = get_ctx(ctx_);
  Z3_ast_print_mode m;
  if (mode == "smtlib2") m = Z3_PRINT_SMTLIB2_COMPLIANT;
  else if (mode == "low_level") m = Z3_PRINT_LOW_LEVEL;
  else if (mode == "smtlib_full") m = Z3_PRINT_SMTLIB_FULL;
  else fail("mode must be \"smtlib2\", \"low_level\" or \"smtlib_full\"");
  Z3_set_ast_print_mode(ctx->c, m);
  check(ctx);
}

// Whether Z3 warning messages are on (they are by default).
static bool warnings_enabled = true;

// Global parameters affect every context created afterwards (and some
// existing ones). Z3 reports unknown names through its error handler, which
// is not available without a context, so names are validated in R.
// [[Rcpp::export(rng = false)]]
void cpp_global_param_set(std::string name, std::string value) {
  Z3_global_param_set(name.c_str(), value.c_str());
}

// [[Rcpp::export(rng = false)]]
SEXP cpp_global_param_get(std::string name) {
  Z3_string v = nullptr;
  // Z3 prints a warning for unknown names; the NA result already says so.
  Z3_toggle_warning_messages(false);
  bool ok = Z3_global_param_get(name.c_str(), &v);
  Z3_toggle_warning_messages(warnings_enabled);
  if (!ok || !v) return Rcpp::CharacterVector::create(NA_STRING);
  return Rcpp::CharacterVector::create(std::string(v));
}

// [[Rcpp::export(rng = false)]]
void cpp_global_param_reset() { Z3_global_param_reset_all(); }

// [[Rcpp::export(rng = false)]]
SEXP cpp_global_param_help(SEXP ctx_) {
#if Z3R_AT_LEAST(4, 9, 0)
  CtxPtr ctx = get_ctx(ctx_);
  return describe_params(ctx, Z3_get_global_param_descrs(ctx->c));
#else
  needs_version("z3_global_param_help()", "4.9.0");
#endif
}

// [[Rcpp::export(rng = false)]]
bool cpp_open_log(std::string file) { return Z3_open_log(file.c_str()); }

// [[Rcpp::export(rng = false)]]
void cpp_close_log() { Z3_close_log(); }

// [[Rcpp::export(rng = false)]]
void cpp_toggle_warnings(bool enabled) {
  warnings_enabled = enabled;
  Z3_toggle_warning_messages(enabled);
}
