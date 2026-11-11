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
#'   with `z3_decl()`.
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
  decl <- cpp_mk_func_decl(ctx, name, domain, range)
  f <- function(...) cpp_apply_decl(decl, flatten_args(...))
  structure(f, class = c("z3_function", "function"), decl = decl)
}

#' @rdname z3_function
#' @param f A `z3_function`.
#' @export
z3_decl <- function(f) {
  if (!inherits(f, "z3_function")) stop("f must be a z3_function", call. = FALSE)
  attr(f, "decl")
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
#' terms that mention all bound variables.
#'
#' @param vars A constant or a list of constants to bind.
#' @param body A Boolean expression.
#' @param patterns Optional list of patterns; each pattern is a list of
#'   terms.
#' @return A Boolean `z3_expr`.
#' @examples
#' ctx <- z3_context()
#' I <- z3_int_sort(ctx)
#' f <- z3_function(ctx, "f", I, I)
#' x <- z3_int(ctx, "x")
#' s <- z3_solver(ctx)
#' z3_add(s, z3_forall(x, f(x) >= x), f(3) < 3)
#' z3_check(s)
#' @export
z3_forall <- function(vars, body, patterns = list()) {
  if (is_z3_expr(vars)) vars <- list(vars)
  cpp_quantifier(TRUE, vars, body, patterns)
}

#' @rdname z3_forall
#' @export
z3_exists <- function(vars, body, patterns = list()) {
  if (is_z3_expr(vars)) vars <- list(vars)
  cpp_quantifier(FALSE, vars, body, patterns)
}
