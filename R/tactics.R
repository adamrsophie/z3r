#' Tactics and goals
#'
#' A goal is a set of formulas. A tactic transforms a goal into zero or
#' more subgoals whose conjunction is equivalent (or equisatisfiable) to
#' the original; for example `"simplify"` rewrites formulas and
#' `"solve-eqs"` eliminates variables. Tactics compose with
#' `z3_tactic_then()` (apply one after the other) and `z3_tactic_or_else()`
#' (try the first, fall back to the second). `z3_tactics()` lists the
#' tactics available in the installed Z3. A tactic can also drive a solver,
#' see `z3_solver_from_tactic()`.
#'
#' @param ctx A [z3_context()].
#' @param name Name of a built-in tactic.
#' @param t,t1,t2 `z3_tactic` objects.
#' @param max Maximum number of repetitions.
#' @param ms Time limit in milliseconds.
#' @param goal A `z3_goal`.
#' @param models,unsat_cores,proofs Whether the goal should support model
#'   conversion, unsat cores and proofs.
#' @param ... For `z3_apply_tactic()`, tactic parameters.
#' @return `z3_tactic()` and the combinators return a `z3_tactic`;
#'   `z3_goal()` returns a `z3_goal`; `z3_apply_tactic()` returns a list of
#'   subgoals; `z3_goal_formulas()` returns a list of `z3_expr`;
#'   `z3_goal_status()` returns `"sat"`, `"unsat"` or `"undecided"`.
#' @examples
#' ctx <- z3_context()
#' x <- z3_int(ctx, "x")
#' y <- z3_int(ctx, "y")
#' g <- z3_goal(ctx)
#' z3_add(g, x == y + 1, y > 2, x < 10)
#' t <- z3_tactic_then(z3_tactic(ctx, "simplify"), z3_tactic(ctx, "solve-eqs"))
#' sub <- z3_apply_tactic(t, g)
#' z3_goal_formulas(sub[[1]])
#' @export
z3_tactic <- function(ctx, name) cpp_mk_tactic(ctx, name)

#' @rdname z3_tactic
#' @export
z3_tactic_then <- function(t1, t2) cpp_tactic_combine("then", t1, t2)

#' @rdname z3_tactic
#' @export
z3_tactic_or_else <- function(t1, t2) cpp_tactic_combine("or_else", t1, t2)

#' @rdname z3_tactic
#' @export
z3_tactic_repeat <- function(t, max = 4294967295) cpp_tactic_modify("repeat", t, max)

#' @rdname z3_tactic
#' @export
z3_tactic_try_for <- function(t, ms) cpp_tactic_modify("try_for", t, ms)

#' @rdname z3_tactic
#' @export
z3_tactics <- function(ctx) cpp_tactic_names(ctx)

#' @rdname z3_tactic
#' @export
z3_solver_from_tactic <- function(t) cpp_mk_solver_from_tactic(t)

#' @rdname z3_tactic
#' @export
z3_goal <- function(ctx, models = TRUE, unsat_cores = FALSE, proofs = FALSE) {
  cpp_mk_goal(ctx, isTRUE(models), isTRUE(unsat_cores), isTRUE(proofs))
}

#' @export
z3_add.z3_goal <- function(x, ...) {
  cpp_goal_assert(x, flatten_args(...))
  invisible(x)
}

#' @export
z3_assertions.z3_goal <- function(x) cpp_goal_formulas(x)

#' @rdname z3_tactic
#' @export
z3_goal_formulas <- function(goal) cpp_goal_formulas(goal)

#' @rdname z3_tactic
#' @export
z3_goal_status <- function(goal) cpp_goal_status(goal)

#' @rdname z3_tactic
#' @export
z3_apply_tactic <- function(t, goal, ...) cpp_tactic_apply(t, goal, params_list(...))

#' @export
print.z3_goal <- function(x, ...) {
  cat("<z3_goal>\n", cpp_goal_to_string(x), "\n", sep = "")
  invisible(x)
}

#' @export
print.z3_tactic <- function(x, ...) {
  cat("<z3_tactic>\n")
  invisible(x)
}
