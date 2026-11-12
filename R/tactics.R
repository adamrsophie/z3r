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

#' Probes and conditional tactics
#'
#' A probe measures a goal and returns a number (Booleans are 1 or 0), such
#' as `"num-consts"`, `"size"` or `"is-qflia"`. `z3_probes()` lists them.
#' Probes compare and combine with `<`, `>`, `<=`, `>=`, `==`, `&`, `|` and
#' `!`; numbers are turned into constant probes.
#'
#' * `z3_tactic_when(p, t)` applies `t` when the probe is true and skips
#'   otherwise; `z3_tactic_cond(p, t1, t2)` chooses between two tactics;
#'   `z3_tactic_fail_if(p)` fails when the probe is true.
#' * `z3_tactic_skip()`, `z3_tactic_fail()` and
#'   `z3_tactic_fail_if_not_decided()` are the trivial tactics.
#' * `z3_tactic_par_or()` runs tactics in parallel and keeps the first to
#'   succeed; `z3_tactic_par_then(t1, t2)` applies `t2` to every subgoal of
#'   `t1` in parallel.
#' * `z3_tactic_with()` fixes parameters of a tactic; `z3_tactic_params()`
#'   lists them.
#'
#' Parallel tactics run inside Z3 on Z3's own threads; R is not involved.
#'
#' @param ctx A [z3_context()].
#' @param name Probe name.
#' @param p A `z3_probe`.
#' @param goal A `z3_goal`.
#' @param t,t1,t2 `z3_tactic` objects.
#' @param ... Tactics for `z3_tactic_par_or()`; named parameters for
#'   `z3_tactic_with()`.
#' @return A `z3_probe`, a `z3_tactic`, a number (`z3_probe_apply()`), or
#'   a data frame (`z3_probes()`, `z3_tactic_params()`).
#' @examples
#' ctx <- z3_context()
#' x <- z3_int(ctx, "x")
#' g <- z3_goal(ctx)
#' z3_add(g, x > 0, x < 10)
#' z3_probe_apply(z3_probe(ctx, "num-consts"), g)
#' small <- z3_probe(ctx, "size") < 5
#' t <- z3_tactic_cond(small, z3_tactic(ctx, "smt"), z3_tactic(ctx, "skip"))
#' z3_apply_tactic(t, g)
#' @name z3_probe
NULL

#' @rdname z3_probe
#' @export
z3_probe <- function(ctx, name) cpp_mk_probe(ctx, name)

#' @rdname z3_probe
#' @export
z3_probes <- function(ctx) cpp_probe_names(ctx)

#' @rdname z3_probe
#' @export
z3_probe_apply <- function(p, goal) cpp_probe_apply(p, goal)

#' @export
Ops.z3_probe <- function(e1, e2) {
  if (missing(e2)) {
    if (.Generic != "!") stop("unary probes only support !", call. = FALSE)
    return(cpp_probe_combine("not", e1, NULL))
  }
  ctx <- z3_context_of(if (inherits(e1, "z3_probe")) e1 else e2)
  if (is.numeric(e1)) e1 <- cpp_probe_const(ctx, e1)
  if (is.numeric(e2)) e2 <- cpp_probe_const(ctx, e2)
  if (.Generic == "!=") return(cpp_probe_combine("not", cpp_probe_combine("==", e1, e2), NULL))
  cpp_probe_combine(.Generic, e1, e2)
}

#' @export
print.z3_probe <- function(x, ...) {
  cat("<z3_probe>\n")
  invisible(x)
}

#' @rdname z3_probe
#' @export
z3_tactic_when <- function(p, t) cpp_tactic_probe("when", p, t, NULL)

#' @rdname z3_probe
#' @export
z3_tactic_cond <- function(p, t1, t2) cpp_tactic_probe("cond", p, t1, t2)

#' @rdname z3_probe
#' @export
z3_tactic_fail_if <- function(p) cpp_tactic_probe("fail_if", p, NULL, NULL)

#' @rdname z3_probe
#' @export
z3_tactic_skip <- function(ctx) cpp_tactic_basic(ctx, "skip")

#' @rdname z3_probe
#' @export
z3_tactic_fail <- function(ctx) cpp_tactic_basic(ctx, "fail")

#' @rdname z3_probe
#' @export
z3_tactic_fail_if_not_decided <- function(ctx) cpp_tactic_basic(ctx, "fail_if_not_decided")

#' @rdname z3_probe
#' @export
z3_tactic_par_or <- function(...) cpp_tactic_par("par_or", flatten_args(...))

#' @rdname z3_probe
#' @export
z3_tactic_par_then <- function(t1, t2) cpp_tactic_par("par_then", list(t1, t2))

#' @rdname z3_probe
#' @export
z3_tactic_with <- function(t, ...) cpp_tactic_using_params(t, params_list(...))

#' @rdname z3_probe
#' @export
z3_tactic_params <- function(t) cpp_tactic_param_help(t)

#' Goal properties
#'
#' `z3_goal_info()` reports the number of formulas, expression count, depth
#' (number of tactic transformations), precision (`"precise"`, `"under"`,
#' `"over"`, `"under_over"`) and whether the goal is known to be
#' inconsistent. `z3_goal_reset()` removes all formulas.
#' `z3_goal_convert_model()` turns a model of a subgoal into a model of the
#' original goal, undoing the tactic's transformations.
#'
#' @param goal A `z3_goal`.
#' @param model A `z3_model` of a subgoal.
#' @return See Details.
#' @examples
#' ctx <- z3_context()
#' x <- z3_int(ctx, "x")
#' y <- z3_int(ctx, "y")
#' g <- z3_goal(ctx)
#' z3_add(g, x == y + 1, y > 2)
#' sub <- z3_apply_tactic(z3_tactic(ctx, "solve-eqs"), g)[[1]]
#' s <- z3_solver(ctx)
#' z3_add(s, z3_goal_formulas(sub))
#' z3_check(s)
#' m <- z3_goal_convert_model(sub, z3_model(s))
#' z3_eval(m, x)
#' @export
z3_goal_info <- function(goal) cpp_goal_info(goal)

#' @rdname z3_goal_info
#' @export
z3_goal_reset <- function(goal) {
  cpp_goal_reset(goal)
  invisible(goal)
}

#' @rdname z3_goal_info
#' @export
z3_goal_convert_model <- function(goal, model) cpp_goal_convert_model(goal, model)

#' Simplifiers
#'
#' Simplifiers (Z3 4.12.2 and later) are preprocessing steps that, unlike
#' tactics, work with incremental solving. `z3_simplifiers()` lists them;
#' `z3_simplifier_then()` chains two; `z3_simplifier_with()` fixes
#' parameters; `z3_solver_add_simplifier()` returns a new solver that
#' preprocesses its assertions with the simplifier.
#'
#' @param ctx A [z3_context()].
#' @param name Simplifier name, such as `"solve-eqs"` or `"elim-unconstrained"`.
#' @param s1,s2,s `z3_simplifier` objects.
#' @param solver A `z3_solver`.
#' @param ... Named parameters.
#' @return A `z3_simplifier`, a `z3_solver`, or a data frame.
#' @examples
#' ctx <- z3_context()
#' if (z3_version_at_least("4.12.2")) {
#'   simp <- z3_simplifier_then(z3_simplifier(ctx, "solve-eqs"),
#'                              z3_simplifier(ctx, "elim-unconstrained"))
#'   s <- z3_solver_add_simplifier(z3_solver(ctx), simp)
#'   x <- z3_int(ctx, "x")
#'   z3_add(s, x == 2)
#'   z3_check(s)
#' }
#' @export
z3_simplifier <- function(ctx, name) cpp_mk_simplifier(ctx, name)

#' @rdname z3_simplifier
#' @export
z3_simplifiers <- function(ctx) cpp_simplifier_names(ctx)

#' @rdname z3_simplifier
#' @export
z3_simplifier_then <- function(s1, s2) cpp_simplifier_then(s1, s2)

#' @rdname z3_simplifier
#' @export
z3_simplifier_with <- function(s, ...) cpp_simplifier_using_params(s, params_list(...))

#' @rdname z3_simplifier
#' @export
z3_solver_add_simplifier <- function(solver, s) cpp_solver_add_simplifier(solver, s)

#' @export
print.z3_simplifier <- function(x, ...) {
  cat("<z3_simplifier>\n")
  invisible(x)
}
