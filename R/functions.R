#' Uninterpreted functions
#'
#' Declares a function symbol with typed arguments and result. Z3 treats it
#' as an arbitrary function: the only built-in fact is that equal arguments
#' give equal results.
#'
#' The result is an R function, so `f(x)` builds the application. R values
#' passed as arguments are converted to the declared argument sorts.
#'
#' @param ctx A [z3_context()].
#' @param name Name of the function.
#' @param domain A list of `z3_sort` objects, one per argument (or a single
#'   sort for a one-argument function).
#' @param range The result `z3_sort`.
#' @return A function of class `z3_function`. Its declaration is available
#'   with [z3_decl()].
#' @examples
#' ctx <- z3_context()
#' I <- z3_int_sort(ctx)
#' f <- z3_function(ctx, "f", list(I), I)
#' x <- z3_int(ctx, "x")
#' s <- z3_solver(ctx)
#' z3_add(s, f(f(x)) == x, f(x) == 1, x != 1)
#' z3_check(s)
#' z3_model(s)
#' @export
z3_function <- function(ctx, name, domain, range) {
  if (inherits(domain, "z3_sort")) domain <- list(domain)
  decl_function(cpp_mk_func_decl(ctx, name, domain, range))
}


#' @export
print.z3_function <- function(x, ...) {
  cat("<z3_function> ", cpp_decl_info(attr(x, "decl"))$text, "\n", sep = "")
  invisible(x)
}

#' @export
print.z3_func_decl <- function(x, ...) {
  cat("<z3_func_decl> ", cpp_decl_info(x)$text, "\n", sep = "")
  invisible(x)
}

#' Quantifiers
#'
#' `z3_forall(vars, body)` states that `body` holds for all values of the
#' bound variables; `z3_exists()` states that it holds for some. The bound
#' variables are ordinary constants (made with [z3_int()] and friends);
#' inside the quantifier they are bound, and occurrences of the same
#' constants outside the quantifier are unaffected.
#'
#' Quantified formulas are outside the decidable fragments Z3 handles
#' completely, so checks may return `"unknown"`. Patterns (also called
#' triggers) guide Z3's instantiation heuristics: each pattern is a list of
#' terms that mention all bound variables. `z3_pattern()` makes one pattern
#' from its terms; it prints in Z3's syntax. A pattern cannot be combined
#' with `no_patterns`.
#'
#' `z3_quantifier_db()` binds de Bruijn variables, made with
#' [z3_bound_var()], instead of constants, as Z3 does internally. The
#' variables are described by `sorts` and `names`: index 0 refers to the
#' last of them, index 1 to the one before it, and so on. Higher indices
#' refer to enclosing binders, so quantifiers can be nested. The body (and
#' any pattern) uses the variables directly. With `type = "lambda"` the
#' result is an array, as from [z3_lambda()]. `z3_forall()`, `z3_exists()`
#' and [z3_lambda()] refuse a term with a free de Bruijn variable: Z3
#' releases before 5.3.0 would bind it to the new binder and later ones to
#' an enclosing one, so nest such terms in `z3_quantifier_db()` only.
#'
#' @param vars A constant or a list of constants to bind.
#' @param body A Boolean expression (any expression for a lambda).
#' @param patterns Optional list of patterns; each pattern is a list of
#'   terms or a `z3_pattern`. A single `z3_pattern` is also accepted.
#' @param no_patterns Optional list of terms that must not be used as
#'   patterns.
#' @param weight Instantiation weight; higher weights make the quantifier
#'   less eagerly instantiated.
#' @param qid,skid Optional quantifier identifier (shown in statistics and
#'   traces) and prefix for Skolem constants.
#' @param sorts A sort or a list of sorts of the bound variables.
#' @param names Names of the bound variables, one per sort.
#' @param type `"forall"`, `"exists"` or `"lambda"`.
#' @param ... Terms of the pattern: expressions or lists of them.
#' @return A Boolean `z3_expr`; an array `z3_expr` for a lambda. A
#'   `z3_pattern` for `z3_pattern()`.
#' @examples
#' ctx <- z3_context()
#' I <- z3_int_sort(ctx)
#' f <- z3_function(ctx, "f", I, I)
#' x <- z3_int(ctx, "x")
#' s <- z3_solver(ctx)
#' z3_add(s, z3_forall(x, f(x) >= x), f(3) < 3)
#' z3_check(s)
#'
#' p <- z3_pattern(f(x))
#' p
#' z3_forall(x, f(x) > 0, patterns = p)
#'
#' # The same quantifier over a de Bruijn variable
#' v0 <- z3_bound_var(ctx, 0, I)
#' z3_quantifier_db(I, "x", f(v0) > 0, patterns = z3_pattern(f(v0)))
#' sq <- z3_quantifier_db(I, "i", v0 * v0, type = "lambda")
#' z3_simplify(z3_select(sq, 7))
#' @export
z3_forall <- function(vars, body, patterns = list(), no_patterns = list(), weight = 0,
                      qid = "", skid = "") {
  cpp_quantifier(TRUE, as_list(vars), body, as_patterns(patterns), as_list(no_patterns),
                 as_count(weight, "weight"), qid, skid)
}

#' @rdname z3_forall
#' @export
z3_exists <- function(vars, body, patterns = list(), no_patterns = list(), weight = 0,
                      qid = "", skid = "") {
  cpp_quantifier(FALSE, as_list(vars), body, as_patterns(patterns), as_list(no_patterns),
                 as_count(weight, "weight"), qid, skid)
}

#' @rdname z3_forall
#' @export
z3_quantifier_db <- function(sorts, names, body, type = c("forall", "exists", "lambda"),
                             patterns = list(), no_patterns = list(), weight = 0,
                             qid = "", skid = "") {
  type <- match.arg(type)
  if (!is.character(names)) stop("names must be a character vector", call. = FALSE)
  cpp_quantifier_db(type, as_list(sorts), names, body, as_patterns(patterns),
                    as_list(no_patterns), as_count(weight, "weight"), qid, skid)
}

#' @rdname z3_forall
#' @export
z3_pattern <- function(...) {
  terms <- flatten_args(...)
  if (!length(terms) || !all(vapply(terms, is_z3_expr, NA))) {
    stop("a pattern needs one or more z3_expr terms", call. = FALSE)
  }
  structure(terms, class = "z3_pattern")
}

#' @export
format.z3_pattern <- function(x, ...) cpp_pattern_to_string(unclass(x))

#' @export
print.z3_pattern <- function(x, ...) {
  cat("<z3_pattern> ", format(x), "\n", sep = "")
  invisible(x)
}

# Patterns for Z3: a list of patterns, each a list of terms. A single
# z3_pattern is one pattern, and a single term (alone or in the list) is a
# pattern of one term.
as_patterns <- function(patterns) {
  if (inherits(patterns, "z3_pattern")) return(list(patterns))
  if (is_z3_expr(patterns)) return(list(list(patterns)))
  lapply(patterns, function(p) if (is_z3_expr(p)) list(p) else p)
}

#' Recursive functions
#'
#' `z3_rec_function()` declares a function whose definition may refer to
#' itself. Define it with `z3_rec_define(f, args, body)`, where `args` are
#' constants standing for the parameters. Z3 unfolds the definition on
#' demand during solving. `z3_rec_define()` needs Z3 4.8.16 or later:
#' earlier releases store a definition made through the API as a macro, so
#' solving with it does not terminate.
#'
#' @param ctx A [z3_context()].
#' @param name Name of the function.
#' @param domain A sort or a list of argument sorts.
#' @param range Result sort.
#' @param f A function from `z3_rec_function()`.
#' @param args A constant or list of constants, one per argument.
#' @param body The function body.
#' @return `z3_rec_function()` returns a `z3_function`; `z3_rec_define()`
#'   returns `f` invisibly.
#' @examples
#' ctx <- z3_context()
#' I <- z3_int_sort(ctx)
#' fact <- z3_rec_function(ctx, "fact", I, I)
#' n <- z3_int(ctx, "n")
#' if (z3_version_at_least("4.8.16")) {
#'   z3_rec_define(fact, n, z3_ite(n <= 0, 1, n * fact(n - 1)))
#'   s <- z3_solver(ctx)
#'   x <- z3_int(ctx, "x")
#'   z3_add(s, fact(x) == 120)
#'   z3_check(s)
#'   z3_eval(z3_model(s), x)
#' }
#' @export
z3_rec_function <- function(ctx, name, domain, range) {
  decl_function(cpp_mk_rec_func_decl(ctx, name, as_list_sorts(domain), range))
}

#' @rdname z3_rec_function
#' @export
z3_rec_define <- function(f, args, body) {
  cpp_add_rec_def(z3_decl(f), as_list(args), body)
  invisible(f)
}

#' @rdname z3_function
#' @param prefix Prefix for the generated name of `z3_fresh_function()`.
#' @export
z3_fresh_function <- function(ctx, prefix, domain, range) {
  decl_function(cpp_mk_fresh_func_decl(ctx, prefix, as_list_sorts(domain), range))
}

as_list_sorts <- function(domain) if (inherits(domain, "z3_sort")) list(domain) else domain

#' Special relations
#'
#' Binary relations with built-in axioms, decided by dedicated Z3
#' procedures: `z3_linear_order()`, `z3_partial_order()`,
#' `z3_piecewise_linear_order()` and `z3_tree_order()` over a sort, and
#' `z3_transitive_closure()` of a binary relation. `id` distinguishes
#' several relations of the same kind and sort.
#'
#' @param sort The sort the relation is over.
#' @param id A non-negative integer.
#' @param f A binary Boolean `z3_function`.
#' @return A `z3_function` taking two arguments.
#' @examples
#' ctx <- z3_context()
#' U <- z3_uninterpreted_sort(ctx, "U")
#' le <- z3_partial_order(U)
#' a <- z3_const(ctx, "a", U)
#' b <- z3_const(ctx, "b", U)
#' s <- z3_solver(ctx)
#' z3_add(s, le(a, b), le(b, a), a != b)
#' z3_check(s)
#' @export
z3_linear_order <- function(sort, id = 0) decl_function(cpp_special_relation("linear", sort, as_count(id, "id")))

#' @rdname z3_linear_order
#' @export
z3_partial_order <- function(sort, id = 0) decl_function(cpp_special_relation("partial", sort, as_count(id, "id")))

#' @rdname z3_linear_order
#' @export
z3_piecewise_linear_order <- function(sort, id = 0) {
  decl_function(cpp_special_relation("piecewise", sort, as_count(id, "id")))
}

#' @rdname z3_linear_order
#' @export
z3_tree_order <- function(sort, id = 0) decl_function(cpp_special_relation("tree", sort, as_count(id, "id")))

#' @rdname z3_linear_order
#' @export
z3_transitive_closure <- function(f) decl_function(cpp_transitive_closure(z3_decl(f)))
