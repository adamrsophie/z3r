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
#'   result) and an `else` value for all other arguments.
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
z3_model_values <- function(model) {
  lapply(as.list(model), function(v) {
    info <- cpp_value_info(v, 20L)
    if (info$kind %in% c("bool", "int", "real", "bv", "string")) z3_value(v) else v
  })
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
  for (f in names(funcs)) {
    entries <- vapply(funcs[[f]]$entries, function(e) {
      args <- vapply(e$args, cpp_expr_to_string, character(1))
      sprintf("%s(%s) = %s", f, paste(args, collapse = ", "), cpp_expr_to_string(e$value))
    }, character(1))
    lines <- c(lines, entries,
               sprintf("%s(...) = %s otherwise", f, cpp_expr_to_string(funcs[[f]]$`else`)))
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
