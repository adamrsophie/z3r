#' Create an optimizer
#'
#' An optimizer is a solver that also minimizes or maximizes objectives and
#' satisfies as many weighted soft constraints as possible. It is a
#' separate object from [z3_solver()] because Z3's optimization engine has
#' different capabilities: it does not support tactics, and over nonlinear
#' arithmetic or quantifiers it may return `"unknown"` or a non-optimal
#' answer.
#'
#' Use [z3_add()] for hard constraints, [z3_add_soft()] for soft
#' constraints, [z3_minimize()] and [z3_maximize()] for objectives, then
#' [z3_check()], [z3_model()] and [z3_optimum()].
#'
#' With several objectives, the default priority is lexicographic: the
#' first objective is optimized first. Use
#' `z3_set_params(opt, priority = "box")` to optimize each objective
#' independently, or `priority = "pareto"` to enumerate Pareto-optimal
#' points with repeated calls to [z3_check()].
#'
#' @param ctx A [z3_context()].
#' @param ... Optimizer parameters, passed to [z3_set_params()].
#' @return An object of class `z3_optimize`.
#' @examples
#' ctx <- z3_context()
#' x <- z3_int(ctx, "x")
#' y <- z3_int(ctx, "y")
#' opt <- z3_optimize(ctx)
#' z3_add(opt, x >= 0, y >= 0, x + 2 * y <= 14, 3 * x - y >= 0, x - y <= 2)
#' obj <- z3_maximize(opt, 3 * x + 4 * y)
#' z3_check(opt)
#' z3_optimum(obj)
#' z3_model(opt)
#' @export
z3_optimize <- function(ctx, ...) {
  o <- cpp_mk_optimize(ctx)
  if (...length() > 0L) z3_set_params(o, ...)
  o
}

#' @export
print.z3_optimize <- function(x, ...) {
  if (!cpp_is_valid(x)) {
    cat("<z3_optimize: invalid (restored from a saved session)>\n")
    return(invisible(x))
  }
  status <- cpp_optimize_last_status(x)
  cat(sprintf("<z3_optimize: %d assertion%s, %d objective%s, %s>\n",
              length(cpp_optimize_assertions(x)),
              if (length(cpp_optimize_assertions(x)) == 1L) "" else "s",
              length(cpp_optimize_objectives(x)),
              if (length(cpp_optimize_objectives(x)) == 1L) "" else "s",
              if (is.na(status)) "not checked" else paste("last check:", status)))
  print_kept_params(x)
  invisible(x)
}

#' @export
z3_add.z3_optimize <- function(x, ...) {
  cpp_optimize_assert(x, flatten_args(...))
  invisible(x)
}

#' @export
z3_check.z3_optimize <- function(x, ..., timeout = NULL) {
  cpp_optimize_check(x, flatten_args(...), check_timeout(timeout))
}

#' @export
z3_model.z3_optimize <- function(x) cpp_optimize_model(x)

#' @export
z3_assert_and_track.z3_optimize <- function(x, constraint, name) {
  invisible(cpp_optimize_assert_and_track(x, constraint, name))
}

#' @export
z3_unsat_core.z3_optimize <- function(x) name_exprs(cpp_optimize_unsat_core(x))

#' @export
z3_reason_unknown.z3_optimize <- function(x) cpp_optimize_reason_unknown(x)

#' @export
z3_push.z3_optimize <- function(x) {
  cpp_optimize_push(x)
  invisible(x)
}

#' @export
z3_pop.z3_optimize <- function(x, n = 1) {
  for (i in seq_len(as_count(n, "n"))) cpp_optimize_pop(x)
  invisible(x)
}

#' @export
z3_assertions.z3_optimize <- function(x) cpp_optimize_assertions(x)

#' @export
z3_statistics.z3_optimize <- function(x) cpp_optimize_statistics(x)

#' @export
z3_set_params.z3_optimize <- function(x, ...) {
  cpp_optimize_set_params(x, params_list(...))
  invisible(x)
}

#' @export
z3_param_help.z3_optimize <- function(x) cpp_optimize_param_help(x)

#' Objectives
#'
#' Adds an objective to an optimizer. The returned handle is used with
#' [z3_optimum()] after a satisfiable [z3_check()].
#'
#' @param opt A `z3_optimize`.
#' @param x An Int, Real or bit-vector expression. Bit-vectors are
#'   optimized as unsigned numbers.
#' @return A `z3_objective` handle.
#' @inherit z3_optimize examples
#' @export
z3_minimize <- function(opt, x) new_objective(opt, x, FALSE)

#' @rdname z3_minimize
#' @export
z3_maximize <- function(opt, x) new_objective(opt, x, TRUE)

new_objective <- function(opt, x, maximize) {
  if (!inherits(opt, "z3_optimize")) stop("opt must be a z3_optimize", call. = FALSE)
  id <- cpp_optimize_objective(opt, x, maximize)
  structure(list(optimizer = opt, index = as.integer(id[[1]]), serial = id[[2]], expr = x,
                 maximize = maximize),
            class = "z3_objective")
}

#' @export
print.z3_objective <- function(x, ...) {
  if (!is.null(x$soft)) {
    cat(sprintf("<z3_objective: violated weight of soft constraints \"%s\">\n", x$soft))
  } else {
    cat(sprintf("<z3_objective: %s %s>\n", if (x$maximize) "maximize" else "minimize",
                format(x$expr)))
  }
  invisible(x)
}

#' Soft constraints
#'
#' Adds a constraint that the optimizer should satisfy if possible. Z3
#' minimizes the total weight of violated soft constraints in each group
#' (`id`); groups act as separate objectives.
#'
#' @param opt A `z3_optimize`.
#' @param constraint A Boolean expression.
#' @param weight Positive weight: a number or an exact decimal or fraction
#'   string.
#' @param id Name of the group of soft constraints.
#' @return Invisibly, a `z3_objective` handle for the group: after a
#'   satisfiable check, [z3_optimum()] gives the total weight of its
#'   violated constraints.
#' @examples
#' ctx <- z3_context()
#' p <- z3_bools(ctx, c("a", "b", "c"))
#' opt <- z3_optimize(ctx)
#' z3_add(opt, z3_or(!p$a, !p$b), z3_or(!p$b, !p$c))
#' z3_add_soft(opt, p$a, 2)
#' z3_add_soft(opt, p$b, 3)
#' z3_add_soft(opt, p$c, 2)
#' z3_check(opt)
#' z3_model_values(z3_model(opt))
#' @export
z3_add_soft <- function(opt, constraint, weight = 1, id = "soft") {
  if (!inherits(opt, "z3_optimize")) stop("opt must be a z3_optimize", call. = FALSE)
  if (is.numeric(weight)) {
    if (length(weight) != 1L || !is.finite(weight) || weight <= 0) {
      stop("weight must be a single positive number", call. = FALSE)
    }
    w <- format(weight, digits = 15, scientific = FALSE, trim = TRUE)
    if (as.numeric(w) != weight) w <- format(weight, digits = 17, scientific = FALSE, trim = TRUE)
    weight <- w
  }
  weight <- check_numeral(weight, "^([0-9]+[.]?[0-9]*|[.][0-9]+|[0-9]+/[0-9]+)$",
                          "a positive decimal or fraction")
  if (!is.character(id) || length(id) != 1L || is.na(id)) {
    stop("id must be a single string", call. = FALSE)
  }
  h <- cpp_optimize_assert_soft(opt, constraint, weight, id)
  # The handle reads the total weight of the group's violated constraints.
  invisible(structure(list(optimizer = opt, index = as.integer(h[[1]]), serial = h[[2]], expr = NULL,
                           maximize = FALSE, soft = id),
                      class = "z3_objective"))
}

#' Optimal value of an objective
#'
#' Returns the optimum found by the last [z3_check()] of the objective's
#' optimizer that returned `"sat"`, or the best value found so far by one
#' that returned `"unknown"`.
#'
#' An optimum is not always a finite, attained number:
#'
#' * `"optimal"`: the bound is finite and attained by the model.
#' * `"unbounded"`: the objective can grow (maximize) or shrink (minimize)
#'   without limit; `value` is `NULL`.
#' * `"not attained"`: the objective approaches `value` but never reaches
#'   it, as when maximizing `x` subject to `x < 3` over the reals. Z3
#'   represents this as `value - epsilon` or `value + epsilon`.
#' * `"unknown"`: the check stopped early, for example at a timeout, and
#'   returned `"unknown"`. `value` is the objective's value in the best
#'   model Z3 had found, which is not proven optimal; it is `NULL` when Z3
#'   had no model that satisfies the hard constraints and the check's
#'   assumptions. `lower` and `upper` are the bounds known when it stopped.
#'   Before Z3 finds its first model these can be trivial bounds, such as
#'   the total weight of the soft constraints. When Z3 stopped before
#'   setting up the objective it reports zero for both bounds; z3r then
#'   returns `NA` for both, which also happens, rarely, when the bound is
#'   exactly zero.
#'
#' Over nonlinear arithmetic the result can be non-optimal; Z3 does not
#' guarantee optimality there.
#'
#' @param objective A `z3_objective` from [z3_minimize()],
#'   [z3_maximize()] or [z3_add_soft()].
#' @return A list of class `z3_optimum` with elements `status`, `value`
#'   (a `z3_expr` or `NULL`), `lower` and `upper` (bound text from Z3, or
#'   `NA`).
#' @inherit z3_optimize examples
#' @export
z3_optimum <- function(objective) {
  if (!inherits(objective, "z3_objective")) {
    stop("objective must be a z3_objective from z3_minimize() or z3_maximize()",
         call. = FALSE)
  }
  b <- cpp_optimize_bounds(objective$optimizer, objective$index, objective$serial,
                           objective$maximize)
  lower <- bound_text(b$lower)
  upper <- bound_text(b$upper)
  if (b$status == "unknown") {
    # Z3 reports zero bounds when it stopped before setting up the objective.
    if (is_zero_bound(b$lower) && is_zero_bound(b$upper)) lower <- upper <- NA_character_
    return(new_optimum("unknown", b$incumbent, objective$maximize, 0, lower, upper))
  }
  # The optimum is the bound Z3 proved: the upper bound when maximizing.
  bound <- if (objective$maximize) b$upper else b$lower
  inf <- sign(z3_value(bound$infinity, exact = FALSE))
  eps <- sign(z3_value(bound$epsilon, exact = FALSE))
  status <- if (inf != 0) "unbounded" else if (eps != 0) "not attained" else "optimal"
  value <- if (objective$maximize) b$upper_value else b$lower_value
  if (eps != 0) value <- bound$value  # the finite part of value +/- epsilon
  if (inf != 0) value <- NULL
  new_optimum(status, value, objective$maximize, eps, lower, upper)
}

new_optimum <- function(status, value, maximize, epsilon, lower, upper) {
  structure(list(
    status = status,
    value = value,
    maximize = maximize,
    epsilon = epsilon,
    lower = lower,
    upper = upper
  ), class = "z3_optimum")
}

is_zero_bound <- function(b) {
  all(vapply(b, function(e) sign(z3_value(e, exact = FALSE)) == 0, logical(1)))
}

bound_text <- function(b) {
  inf <- sign(z3_value(b$infinity, exact = FALSE))
  if (inf != 0) return(if (inf > 0) "+oo" else "-oo")
  eps <- sign(z3_value(b$epsilon, exact = FALSE))
  v <- format(b$value)
  if (eps > 0) paste(v, "+ epsilon") else if (eps < 0) paste(v, "- epsilon") else v
}

#' @export
print.z3_optimum <- function(x, ...) {
  what <- switch(x$status,
    optimal = format(x$value),
    unbounded = if (x$maximize) "+oo (unbounded)" else "-oo (unbounded)",
    `not attained` = sprintf("%s (%s, not attained)", format(x$value),
                             if (x$maximize) "supremum" else "infimum"),
    unknown = if (is.null(x$value)) "unknown (no value found before the check stopped)"
              else sprintf("%s (best so far, not proven optimal)", format(x$value))
  )
  cat("<z3_optimum> ", what, "\n", sep = "")
  invisible(x)
}
