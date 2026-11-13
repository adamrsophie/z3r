#' Evaluate an expression in a model
#'
#' Substitutes the model's assignments into `x` and simplifies.
#'
#' With `completion = FALSE` (the default), constants that the model does
#' not mention stay symbolic, so the result shows which parts of `x` the
#' model leaves unconstrained. With `completion = TRUE`, Z3 assigns such
#' constants a default value (for example 0 or `FALSE`) and records it in
#' the model, so the result is always a value.
#'
#' @param model A `z3_model`.
#' @param x A `z3_expr`, or an R scalar.
#' @param completion Whether to complete the model; see Details.
#' @return A `z3_expr`. Use [z3_value()] to convert it to an R value.
#' @examples
#' ctx <- z3_context()
#' x <- z3_int(ctx, "x")
#' y <- z3_int(ctx, "y")
#' s <- z3_solver(ctx)
#' z3_add(s, x == 3)
#' z3_check(s)
#' m <- z3_model(s)
#' z3_eval(m, x + 1)
#' z3_eval(m, x + y)
#' z3_eval(m, x + y, completion = TRUE)
#' @export
z3_eval <- function(model, x, completion = FALSE) {
  if (!isTRUE(completion) && !isFALSE(completion)) {
    stop("completion must be TRUE or FALSE", call. = FALSE)
  }
  cpp_model_eval(model, x, completion)
}

#' Contents of a model
#'
#' * `as.list()` returns the model's constant assignments as a named list
#'   of `z3_expr` values. Nothing is converted, so no precision is lost.
#' * `z3_model_values()` converts each value with [z3_value()]. Values that
#'   have no exact R representation (algebraic numbers, arrays, values of
#'   uninterpreted sorts) are kept as `z3_expr` objects.
#' * `z3_model_functions()` returns the interpretations of uninterpreted
#'   functions: for each function, a list of `entries` (argument values and
#'   result) and an `else` value for all other arguments (`NULL` in a
#'   partial model, which has none).
#'
#' The model only contains the constants that matter for the assertions;
#' see [z3_eval()] with `completion = TRUE` for the others.
#'
#' @param x,model A `z3_model`.
#' @param ... Unused.
#' @return A named list.
#' @examples
#' ctx <- z3_context()
#' x <- z3_int(ctx, "x")
#' r <- z3_real(ctx, "r")
#' s <- z3_solver(ctx)
#' z3_add(s, x == z3_int_val(ctx, "1180591620717411303424"), 3 * r == 1)
#' z3_check(s)
#' m <- z3_model(s)
#' as.list(m)
#' z3_model_values(m)
#' @export
as.list.z3_model <- function(x, ...) {
  k <- cpp_model_consts(x)
  stats::setNames(k$values, k$names)
}

#' @rdname as.list.z3_model
#' @export
z3_model_values <- function(model) lapply(as.list(model), value_or_expr)

# An R value where one represents the Z3 value exactly, else the expression.
value_or_expr <- function(v) {
  info <- cpp_value_info(v, 20L)
  if (info$kind %in% c("bool", "int", "real", "bv", "string", "char", "ff", "rm") ||
      (info$kind == "fp" && info$exact_double)) z3_value(v) else v
}

#' @rdname as.list.z3_model
#' @export
z3_model_functions <- function(model) cpp_model_funcs(model)

#' @export
length.z3_model <- function(x) length(cpp_model_consts(x)$names)

#' @export
format.z3_model <- function(x, ...) {
  if (!cpp_is_valid(x)) return("<invalid z3_model>")
  k <- cpp_model_consts(x)
  vals <- vapply(k$values, cpp_expr_to_string, character(1))
  lines <- if (length(vals)) paste(k$names, "=", vals) else character()
  funcs <- cpp_model_funcs(x)
  # By position: overloaded functions share a name.
  for (i in seq_along(funcs)) {
    f <- names(funcs)[i]
    entries <- vapply(funcs[[i]]$entries, function(e) {
      args <- vapply(e$args, cpp_expr_to_string, character(1))
      sprintf("%s(%s) = %s", f, paste(args, collapse = ", "), cpp_expr_to_string(e$value))
    }, character(1))
    els <- funcs[[i]]$`else`
    lines <- c(lines, entries,
               if (!is.null(els)) sprintf("%s(...) = %s otherwise", f, cpp_expr_to_string(els)))
  }
  lines
}

#' @export
print.z3_model <- function(x, ...) {
  lines <- format(x)
  cat("<z3_model>\n")
  if (length(lines)) cat(paste0("  ", lines), sep = "\n")
  invisible(x)
}

#' @export
as.character.z3_model <- function(x, ...) cpp_model_to_string(x)

#' @rdname z3_model
#' @export
z3_model.z3_context <- function(x) cpp_mk_model(x)

#' Build and query models
#'
#' `z3_model(ctx)` creates an empty model. `z3_model_set()` assigns a value
#' to a constant; `z3_model_set_function()` defines a function by a table of
#' entries and a default value. `z3_model_set_default()` replaces the
#' default (`else`) value of a function the model already interprets and
#' keeps its entries; it works on models returned by solvers too. The
#' default value of a function may use [z3_bound_var()] `i` for argument
#' `i + 1`. A hand-made model can be used with [z3_eval()] to evaluate
#' expressions under chosen values.
#'
#' `z3_model_has()` tests whether a model interprets a constant or
#' function. `z3_model_universes()` returns, for each uninterpreted sort of
#' the model, the finite list of values the model uses for it.
#' `z3_as_array_function()` returns the function behind an array value of
#' the form `(_ as-array f)`, or `NULL`.
#'
#' @param model A `z3_model`.
#' @param constant A constant (`z3_expr`).
#' @param value A value of the constant's sort.
#' @param f A `z3_function` or `z3_func_decl`.
#' @param entries A list of entries, each `list(args = list(...), value = v)`.
#' @param otherwise The value for all other arguments.
#' @param x A constant, function, or (for `z3_as_array_function()`) an
#'   array value.
#' @return `z3_model_set()`, `z3_model_set_function()` and
#'   `z3_model_set_default()` return `model` invisibly; the others return
#'   the requested information.
#' @examples
#' ctx <- z3_context()
#' x <- z3_int(ctx, "x")
#' f <- z3_function(ctx, "f", z3_int_sort(ctx), z3_int_sort(ctx))
#' m <- z3_model(ctx)
#' z3_model_set(m, x, 5)
#' z3_model_set_function(m, f, list(list(args = list(5), value = 50)), 0)
#' z3_eval(m, f(x) + f(1))
#' z3_model_set_default(m, f, z3_bound_var(ctx, 0, z3_int_sort(ctx)) * 2)
#' z3_eval(m, f(x) + f(1))
#' @export
z3_model_set <- function(model, constant, value) {
  cpp_model_set_const(model, constant, value)
  invisible(model)
}

#' @rdname z3_model_set
#' @export
z3_model_set_function <- function(model, f, entries = list(), otherwise) {
  cpp_model_set_func(model, z3_decl(f), entries, otherwise)
  invisible(model)
}

#' @rdname z3_model_set
#' @export
z3_model_set_default <- function(model, f, otherwise) {
  cpp_model_set_else(model, z3_decl(f), otherwise)
  invisible(model)
}

#' @rdname z3_model_set
#' @export
z3_model_has <- function(model, x) {
  if (inherits(x, "z3_function")) x <- z3_decl(x)
  cpp_model_has(model, x)
}

#' @rdname z3_model_set
#' @export
z3_model_universes <- function(model) cpp_model_universes(model)

#' @rdname z3_model_set
#' @export
z3_as_array_function <- function(x) cpp_as_array_decl(x)

#' Model-based projection and light quantifier elimination
#'
#' Spacer's utilities for eliminating variables from a formula.
#'
#' * `z3_model_project(model, vars, formula)` performs model-based
#'   projection: it returns a formula free of `vars` that is true in
#'   `model` and implies `exists vars. formula`. `formula` must be true in
#'   `model` (an error otherwise). Variables Z3 cannot eliminate, such as
#'   some arrays, stay in the result. With `method = "witness"` (Z3 4.13.4) the result is a list
#'   with the projected `formula` and, in `vars` and `terms`, terms for the
#'   eliminated variables that keep the formula true in the model when
#'   substituted for them; with `method = "skolem"`, the list holds the
#'   Skolem terms recorded by arithmetic projection.
#' * `z3_model_extrapolate(model, formula)` returns a conjunction of
#'   literals that is true in `model` and implies `formula` (an implicant
#'   of `formula` taken from the model); `formula` must be true in
#'   `model`.
#' * `z3_qe_lite(vars, formula)` eliminates, where it is cheap to do so,
#'   the variables `vars` from `exists vars. formula`. It returns a list
#'   with the result `formula` and the variables that `remaining` free in
#'   it.
#'
#' @param model A `z3_model`.
#' @param vars A constant or a list of constants.
#' @param formula A Boolean expression.
#' @param method `"plain"`, `"witness"` or `"skolem"`.
#' @return An expression, or a list as described above.
#' @examples
#' ctx <- z3_context()
#' x <- z3_int(ctx, "x")
#' y <- z3_int(ctx, "y")
#' f <- x > 2 & y == x + 1
#' sol <- z3_solver(ctx)
#' z3_add(sol, f)
#' z3_check(sol)
#' m <- z3_model(sol)
#' z3_model_project(m, x, f)
#' z3_model_extrapolate(m, f | y < 0)
#' z3_qe_lite(x, y == x + 1 & x > 2)
#' @export
z3_model_project <- function(model, vars, formula, method = c("plain", "witness", "skolem")) {
  method <- match.arg(method)
  cpp_model_project(model, as_list(vars), formula, method)
}

#' @rdname z3_model_project
#' @export
z3_model_extrapolate <- function(model, formula) cpp_model_extrapolate(model, formula)

#' @rdname z3_model_project
#' @export
z3_qe_lite <- function(vars, formula) cpp_qe_lite(as_list(vars), formula)
