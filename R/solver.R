#' Create a solver
#'
#' A solver holds a stack of assertions and decides whether they are
#' satisfiable. Add constraints with [z3_add()], decide with [z3_check()],
#' and inspect the result with [z3_model()] or [z3_unsat_core()].
#'
#' @param ctx A [z3_context()].
#' @param logic Optional SMT-LIB logic name such as `"QF_LIA"` (quantifier
#'   free linear integer arithmetic). A logic lets Z3 pick a specialized
#'   strategy; constraints outside the logic may then fail or return
#'   `"unknown"`.
#' @param simple If `TRUE`, create Z3's simple solver: an incremental SMT
#'   solver without the tactic-based preprocessing of the default solver.
#'   Some features, such as [z3_trail()], need it.
#' @param ... Solver parameters, passed to [z3_set_params()].
#' @return An object of class `z3_solver`.
#' @examples
#' ctx <- z3_context()
#' x <- z3_int(ctx, "x")
#' y <- z3_int(ctx, "y")
#' s <- z3_solver(ctx)
#' z3_add(s, x + y == 10, x > 2 * y, y > 0)
#' z3_check(s)
#' z3_model(s)
#' @export
z3_solver <- function(ctx, logic = NULL, simple = FALSE, ...) {
  if (isTRUE(simple) && !is.null(logic)) {
    stop("a simple solver cannot also be specialized to a logic", call. = FALSE)
  }
  s <- if (isTRUE(simple)) cpp_mk_simple_solver(ctx) else cpp_mk_solver(ctx, logic)
  if (...length() > 0L) z3_set_params(s, ...)
  s
}

#' @export
print.z3_solver <- function(x, ...) {
  if (!cpp_is_valid(x)) {
    cat("<z3_solver: invalid (restored from a saved session)>\n")
    return(invisible(x))
  }
  n <- length(cpp_solver_assertions(x))
  status <- cpp_solver_last_status(x)
  cat(sprintf("<z3_solver: %d assertion%s, %d scope%s, %s>\n",
              n, if (n == 1L) "" else "s",
              cpp_solver_num_scopes(x), if (cpp_solver_num_scopes(x) == 1L) "" else "s",
              if (is.na(status)) "not checked" else paste("last check:", status)))
  print_kept_params(x)
  invisible(x)
}

# Z3 has no getter for parameters; z3r shows the ones it applied.
print_kept_params <- function(x) {
  p <- cpp_kept_params(x)
  if (nzchar(p)) cat("parameters set: ", p, "\n", sep = "")
}

#' Add constraints
#'
#' Asserts Boolean constraints into a solver, optimizer or goal. All
#' constraints are checked before any is added, so a call that fails adds
#' nothing.
#'
#' @param x A `z3_solver`, `z3_optimize` or `z3_goal`.
#' @param ... Boolean `z3_expr` constraints, or lists of them.
#' @return `x`, invisibly.
#' @examples
#' ctx <- z3_context()
#' x <- z3_int(ctx, "x")
#' s <- z3_solver(ctx)
#' z3_add(s, x > 0, x < 10)
#' z3_add(s, lapply(2:4, function(k) x != k))
#' z3_assertions(s)
#' @export
z3_add <- function(x, ...) UseMethod("z3_add")

#' @export
z3_add.z3_solver <- function(x, ...) {
  cpp_solver_assert(x, flatten_args(...))
  invisible(x)
}

#' Check satisfiability
#'
#' Decides whether the assertions of a solver or optimizer, together with
#' the optional assumptions, are satisfiable.
#'
#' The result is one of three strings:
#'
#' * `"sat"`: a model exists; retrieve it with [z3_model()].
#' * `"unsat"`: no model exists; [z3_unsat_core()] explains why when
#'   assumptions or tracked assertions were used.
#' * `"unknown"`: Z3 gave up, for example because of a timeout, a resource
#'   limit, or a theory (such as nonlinear integer arithmetic or
#'   quantifiers) for which Z3 has no complete decision procedure.
#'   `"unknown"` is not `"unsat"`; see [z3_reason_unknown()].
#'
#' Assumptions are Boolean expressions that hold for this check only. They
#' are not added to the solver.
#'
#' @section Timeouts:
#' `timeout` limits this check to that many milliseconds, replacing any
#' timeout of the solver or optimizer for this call only; a check that runs
#' out of time returns `"unknown"`. Afterwards z3r sets the object's
#' parameters back, also when the check fails or is interrupted: the
#' timeout set with [z3_set_params()] returns, and without one the object
#' gets the context's timeout: the one set through z3r
#' (`z3_context(timeout = )` or [z3_update_param()]), or else a global
#' `timeout` that was in force when the context was made, or none. Z3
#' cannot remove a parameter, so the object then keeps that value as its
#' own: a later change of the context's timeout, or of a global `timeout`
#' or `solver.timeout` set with [z3_global_param_set()], no longer reaches
#' it. Printing the object shows the parameters z3r has set on it.
#'
#' @section Interrupting a check:
#' While a check runs, Z3 installs its own handler for the interrupt signal
#' (Z3's `ctrl_c` parameter, on by default). Pressing Ctrl-C (or the stop
#' button of a front end that sends an interrupt signal) cancels the check
#' and leaves the solver usable. For a `z3_solver`, z3r then raises an
#' ordinary R interrupt, so `tryCatch(..., interrupt = )` works as for any
#' R computation. A `z3_optimize` reports the same reason (`"canceled"`)
#' for an interrupt, a timeout and a resource limit, so an interrupted
#' optimizer check returns `"unknown"` instead. Front ends that do not send
#' a signal (such as RGui on Windows) cannot interrupt Z3; set a `timeout`
#' or `rlimit` with [z3_set_params()] there. R code running inside the
#' check (a callback) can stop it with [z3_interrupt()].
#'
#' @param x A `z3_solver` or `z3_optimize`.
#' @param ... Assumptions: Boolean expressions or lists of them.
#' @param timeout `NULL`, or a time limit in milliseconds (a positive whole
#'   number) for this check only; see the Timeouts section.
#' @return `"sat"`, `"unsat"` or `"unknown"`.
#' @examples
#' ctx <- z3_context()
#' x <- z3_int(ctx, "x")
#' p <- z3_bool(ctx, "p")
#' s <- z3_solver(ctx)
#' z3_add(s, z3_implies(p, x > 10), x < 5)
#' z3_check(s)
#' z3_check(s, p)
#' z3_unsat_core(s)
#' z3_check(s, timeout = 5000)
#' @export
z3_check <- function(x, ..., timeout = NULL) UseMethod("z3_check")

#' @export
z3_check.z3_solver <- function(x, ..., timeout = NULL) {
  cpp_solver_check(x, flatten_args(...), check_timeout(timeout))
}

check_timeout <- function(timeout) {
  if (is.null(timeout)) NULL else as_count(timeout, "timeout", positive = TRUE)
}

#' Model of the last satisfiable check
#'
#' Returns the model found by the most recent [z3_check()]. A model is only
#' available when that check returned `"sat"` and the solver has not been
#' changed since (by [z3_add()], [z3_push()], [z3_pop()] or [z3_reset()]),
#' so a model never describes constraints it was not computed for.
#'
#' The returned model is a snapshot: it stays valid after the solver
#' changes.
#'
#' @param x A `z3_solver` or `z3_optimize`.
#' @return A `z3_model`.
#' @seealso [z3_eval()], [z3_model_values()].
#' @examples
#' ctx <- z3_context()
#' x <- z3_int(ctx, "x")
#' s <- z3_solver(ctx)
#' z3_add(s, x * x == 49, x < 0)
#' z3_check(s)
#' m <- z3_model(s)
#' z3_eval(m, x)
#' @export
z3_model <- function(x) UseMethod("z3_model")

#' @export
z3_model.z3_solver <- function(x) cpp_solver_model(x)

#' Track assertions for unsatisfiable cores
#'
#' Asserts `constraint` under a Boolean label. When the solver is
#' unsatisfiable, [z3_unsat_core()] returns the labels of a subset of the
#' tracked assertions that is itself unsatisfiable.
#'
#' @param x A `z3_solver` or `z3_optimize`.
#' @param constraint A Boolean expression.
#' @param name The label: a string naming a new Boolean constant. Use
#'   names that are not used for other constants.
#' @return The label, a Boolean `z3_expr`, invisibly.
#' @examples
#' ctx <- z3_context()
#' x <- z3_int(ctx, "x")
#' s <- z3_solver(ctx)
#' z3_assert_and_track(s, x > 10, "lower_bound")
#' z3_assert_and_track(s, x < 5, "upper_bound")
#' z3_assert_and_track(s, x != 7, "not_seven")
#' z3_check(s)
#' names(z3_unsat_core(s))
#' @export
z3_assert_and_track <- function(x, constraint, name) UseMethod("z3_assert_and_track")

#' @export
z3_assert_and_track.z3_solver <- function(x, constraint, name) {
  invisible(cpp_solver_assert_and_track(x, constraint, name))
}

#' Unsatisfiable core
#'
#' After [z3_check()] returns `"unsat"`, returns a subset of the
#' assumptions and tracked-assertion labels that is still unsatisfiable
#' together with the untracked assertions. Z3 does not guarantee that the
#' core is minimal. Without assumptions or tracked assertions the core is
#' empty.
#'
#' @param x A `z3_solver` or `z3_optimize`.
#' @return A list of Boolean `z3_expr` objects, named by their text.
#' @inherit z3_assert_and_track examples
#' @export
z3_unsat_core <- function(x) UseMethod("z3_unsat_core")

#' @export
z3_unsat_core.z3_solver <- function(x) name_exprs(cpp_solver_unsat_core(x))

name_exprs <- function(lst) {
  names(lst) <- vapply(lst, cpp_expr_to_string, character(1))
  lst
}

#' Reason for an unknown result
#'
#' @param x A `z3_solver` or `z3_optimize`.
#' @return A string such as `"timeout"` or `"incomplete quantifiers"`.
#' @examples
#' ctx <- z3_context()
#' s <- z3_solver(ctx)
#' z3_reason_unknown(s)
#' @export
z3_reason_unknown <- function(x) UseMethod("z3_reason_unknown")

#' @export
z3_reason_unknown.z3_solver <- function(x) cpp_solver_reason_unknown(x)

#' Interrupt a running check
#'
#' Stops the check that is running on `solver`. It is meant for R code that
#' runs inside that check: the callbacks of a user propagator
#' ([z3_propagator()]) or of [z3_on_clause()], which cannot otherwise use
#' their own solver, and the model callback of an optimizer
#' ([z3_on_model()]). The check then returns `"unknown"`; the solver or
#' optimizer stays usable. For a solver [z3_reason_unknown()] reports
#' `"interrupted"`. An optimizer keeps the best model found so far, whose
#' objective values [z3_optimum()] reports. Z3 notices the request at its
#' next opportunity, so a callback may still run a few more times. When no
#' check is running, `z3_interrupt()` does nothing.
#'
#' Z3 can interrupt a single solver but not a single optimizer; for an
#' optimizer `z3_interrupt()` interrupts its context while the optimizer's
#' check runs.
#'
#' @param solver A `z3_solver` or `z3_optimize`.
#' @return `solver`, invisibly.
#' @examples
#' ctx <- z3_context()
#' if (z3_version_at_least("4.12.3")) {
#'   p <- z3_bools(ctx, paste0("p", 1:3))
#'   s <- z3_solver(ctx)
#'   z3_add(s, z3_or(p), z3_or(!p$p1, !p$p2))
#'   # Stop the search at the first clause Z3 reports.
#'   z3_on_clause(s, function(proof_hint, deps, literals) z3_interrupt(s))
#'   z3_check(s)
#' }
#'
#' # Stop an optimizer at its first model.
#' x <- z3_int(ctx, "x")
#' opt <- z3_optimize(ctx)
#' z3_add(opt, x >= 0, x <= 100)
#' obj <- z3_maximize(opt, x)
#' z3_on_model(opt, function(model) z3_interrupt(opt))
#' z3_check(opt)
#' z3_optimum(obj)
#' @export
z3_interrupt <- function(solver) {
  if (inherits(solver, "z3_optimize")) {
    cpp_optimize_interrupt(solver)
  } else if (inherits(solver, "z3_solver")) {
    cpp_solver_interrupt(solver)
  } else {
    stop("solver must be a z3_solver or z3_optimize", call. = FALSE)
  }
  invisible(solver)
}

#' Incremental solving: scopes
#'
#' `z3_push()` opens a backtracking scope; `z3_pop()` removes every
#' assertion added since the matching push. `z3_reset()` removes all
#' assertions and scopes, and also user propagators and clause callbacks
#' ([z3_propagator()]). `z3_num_scopes()` reports the number of open
#' scopes. Popping an optimizer removes the objectives added in the scope;
#' their handles can no longer be used.
#'
#' @param x A `z3_solver` (or a `z3_optimize` for push and pop).
#' @param n Number of scopes to pop.
#' @return `x`, invisibly; `z3_num_scopes()` returns an integer.
#' @examples
#' ctx <- z3_context()
#' x <- z3_int(ctx, "x")
#' s <- z3_solver(ctx)
#' z3_add(s, x > 0)
#' z3_push(s)
#' z3_add(s, x < 0)
#' z3_check(s)
#' z3_pop(s)
#' z3_check(s)
#' @export
z3_push <- function(x) UseMethod("z3_push")

#' @export
z3_push.z3_solver <- function(x) {
  cpp_solver_push(x)
  invisible(x)
}

#' @rdname z3_push
#' @export
z3_pop <- function(x, n = 1) UseMethod("z3_pop")

#' @export
z3_pop.z3_solver <- function(x, n = 1) {
  cpp_solver_pop(x, as_count(n, "n"))
  invisible(x)
}

#' @rdname z3_push
#' @export
z3_reset <- function(x) {
  if (!inherits(x, "z3_solver")) stop("x must be a z3_solver", call. = FALSE)
  cpp_solver_reset(x)
  invisible(x)
}

#' @rdname z3_push
#' @export
z3_num_scopes <- function(x) {
  if (!inherits(x, "z3_solver")) stop("x must be a z3_solver", call. = FALSE)
  cpp_solver_num_scopes(x)
}

#' Assertions of a solver, optimizer or goal
#'
#' @param x A `z3_solver`, `z3_optimize` or `z3_goal`.
#' @return A list of Boolean `z3_expr` objects.
#' @examples
#' ctx <- z3_context()
#' x <- z3_int(ctx, "x")
#' s <- z3_solver(ctx)
#' z3_add(s, x > 0, x < 3)
#' z3_assertions(s)
#' @export
z3_assertions <- function(x) UseMethod("z3_assertions")

#' @export
z3_assertions.z3_solver <- function(x) cpp_solver_assertions(x)

#' Solver statistics
#'
#' `z3_statistics()` returns the statistics as numbers;
#' `z3_statistics_text()` returns them as Z3 writes them, one
#' `:name value` pair per entry.
#'
#' @param x A `z3_solver`, `z3_optimize` or `z3_fixedpoint`.
#' @return `z3_statistics()` returns a named list of numbers (counts,
#'   memory in megabytes, time in seconds) reported by Z3 for the last
#'   check. `z3_statistics_text()` returns a string.
#' @examples
#' ctx <- z3_context()
#' s <- z3_solver(ctx)
#' z3_add(s, z3_int(ctx, "x") > 0)
#' z3_check(s)
#' names(z3_statistics(s))
#' cat(z3_statistics_text(s), "\n")
#' @export
z3_statistics <- function(x) UseMethod("z3_statistics")

#' @export
z3_statistics.z3_solver <- function(x) cpp_solver_statistics(x)

#' @rdname z3_statistics
#' @export
z3_statistics_text <- function(x) {
  if (!inherits(x, c("z3_solver", "z3_optimize", "z3_fixedpoint"))) {
    stop("x must be a z3_solver, z3_optimize or z3_fixedpoint", call. = FALSE)
  }
  cpp_statistics_text(x)
}

#' Solver and optimizer parameters
#'
#' `z3_set_params()` sets Z3 parameters such as `timeout` (milliseconds),
#' `rlimit` (a deterministic resource limit), `random_seed`, or
#' `unsat_core`. Each value's type is checked against Z3's description of
#' the parameter. `z3_param_help()` lists the accepted parameters.
#'
#' A deterministic `rlimit` is preferable to `timeout` when results must be
#' reproducible across machines.
#'
#' @param x A `z3_solver` or `z3_optimize`.
#' @param ... Named parameter values.
#' @return `z3_set_params()` returns `x` invisibly. `z3_param_help()`
#'   returns a data frame with columns `name`, `kind` and `description`.
#' @examples
#' ctx <- z3_context()
#' s <- z3_solver(ctx)
#' z3_set_params(s, timeout = 1000, random_seed = 42)
#' head(z3_param_help(s))
#' @export
z3_set_params <- function(x, ...) UseMethod("z3_set_params")

#' @export
z3_set_params.z3_solver <- function(x, ...) {
  cpp_solver_set_params(x, params_list(...))
  invisible(x)
}

#' @rdname z3_set_params
#' @export
z3_param_help <- function(x) UseMethod("z3_param_help")

#' @export
z3_param_help.z3_solver <- function(x) cpp_solver_param_help(x)

params_list <- function(...) {
  p <- list(...)
  if (length(p) && (is.null(names(p)) || any(names(p) == ""))) {
    stop("all parameters must be named", call. = FALSE)
  }
  p
}

#' Proofs, consequences and cubes
#'
#' * `z3_proof()` returns Z3's proof of unsatisfiability after a check
#'   returned `"unsat"`. The context must have been created with
#'   `z3_context(proof = TRUE)`.
#' * `z3_consequences()` computes, for each variable, a literal that is
#'   implied by the assertions together with a subset of the assumptions,
#'   as `implies(and(assumptions), literal)` formulas. It is a check: it
#'   returns the status and updates the solver's last result.
#' * `z3_cube()` returns the next cube (a conjunction of literals as a list)
#'   for cube-and-conquer search over `vars`; an empty list means the search
#'   space is exhausted, and a list holding only `FALSE` means unsat.
#' * `z3_units()`, `z3_non_units()` and `z3_trail()` expose literals the
#'   solver has derived; `z3_trail()` also returns decision levels.
#'
#' @param x A `z3_solver`.
#' @param assumptions,variables,vars Lists of expressions.
#' @param backtrack_level Backtracking level for cube generation.
#' @return See Details.
#' @examples
#' ctx <- z3_context(proof = TRUE)
#' p <- z3_bool(ctx, "p")
#' q <- z3_bool(ctx, "q")
#' s <- z3_solver(ctx)
#' z3_add(s, z3_implies(p, q), p, !q)
#' z3_check(s)
#' z3_proof(s)
#' s2 <- z3_solver(ctx)
#' z3_add(s2, z3_implies(p, q))
#' z3_consequences(s2, list(p), list(q))
#' @export
z3_proof <- function(x) cpp_solver_proof(x)

#' @rdname z3_proof
#' @export
z3_consequences <- function(x, assumptions, variables) {
  cpp_solver_consequences(x, as_list(assumptions), as_list(variables))
}

#' @rdname z3_proof
#' @export
z3_cube <- function(x, vars = list(), backtrack_level = 4294967295) {
  if (!is.numeric(backtrack_level) || length(backtrack_level) != 1L ||
      backtrack_level < 0 || backtrack_level > 4294967295) {
    stop("backtrack_level must be a non-negative whole number", call. = FALSE)
  }
  # Z3 uses UINT_MAX to mean "no backtracking limit"; pass it as a double
  # through an int-sized value.
  lvl <- if (backtrack_level > .Machine$integer.max) -1L else as.integer(backtrack_level)
  cpp_solver_cube(x, as_list(vars), lvl)
}

#' @rdname z3_proof
#' @export
z3_units <- function(x) cpp_solver_literals(x, "units")

#' @rdname z3_proof
#' @export
z3_non_units <- function(x) cpp_solver_literals(x, "non_units")

#' @rdname z3_proof
#' @export
z3_trail <- function(x) cpp_solver_literals(x, "trail")

#' Congruence classes
#'
#' After a satisfiable check, terms that the solver found equal belong to
#' one congruence class. `z3_congruence_root()` returns a class
#' representative and `z3_congruence_next()` the next member of the class,
#' cycling back to the start. `z3_congruence_explain()` returns the
#' explanation of why two terms are equal. Root and next need Z3 4.12.0;
#' explain needs 4.14.0. Only some solver configurations track classes.
#'
#' @param x A `z3_solver`.
#' @param a,b Expressions.
#' @return A `z3_expr`.
#' @examples
#' ctx <- z3_context()
#' a <- z3_int(ctx, "a")
#' b <- z3_int(ctx, "b")
#' s <- z3_solver(ctx)
#' z3_add(s, a == b)
#' z3_check(s)
#' if (z3_version_at_least("4.12.0")) z3_congruence_root(s, a)
#' @export
z3_congruence_root <- function(x, a) cpp_solver_congruence(x, "root", a, NULL)

#' @rdname z3_congruence_root
#' @export
z3_congruence_next <- function(x, a) cpp_solver_congruence(x, "next", a, NULL)

#' @rdname z3_congruence_root
#' @export
z3_congruence_explain <- function(x, a, b) cpp_solver_congruence(x, "explain", a, b)

#' Solve for variables
#'
#' Asks the solver to express variables as terms over the others, valid
#' under the returned guards. Z3 may solve for only some of the variables;
#' the result lists those it solved for (`vars`) with their `terms`, in the
#' same order. Needs Z3 4.14.0 or later. Only the plain SMT solver
#' (`z3_solver(ctx, simple = TRUE)`) solves for variables; the default
#' solver returns empty lists.
#'
#' @param x A `z3_solver`.
#' @param vars A list of constants.
#' @return A list with elements `vars`, `terms` and `guards`, lists of
#'   `z3_expr`; `vars[[i]]` equals `terms[[i]]` under the guards.
#' @examples
#' ctx <- z3_context()
#' x <- z3_int(ctx, "x")
#' y <- z3_int(ctx, "y")
#' s <- z3_solver(ctx, simple = TRUE)
#' z3_add(s, x + 2 * y == 10)
#' z3_check(s)
#' if (z3_version_at_least("4.14.0")) z3_solve_for(s, list(x))
#' @export
z3_solve_for <- function(x, vars) cpp_solver_solve_for(x, as_list(vars))

#' Initial values (phase hints)
#'
#' Suggests a value for a variable that the solver tries first. It does not
#' constrain the result. Needs Z3 4.13.2 or later. Z3 supports it on the
#' plain SMT solver (`z3_solver(ctx, simple = TRUE)`) and on optimizers,
#' not on the default tactic-based solver.
#'
#' @param x A `z3_solver` or `z3_optimize`.
#' @param var A constant.
#' @param value Its suggested value.
#' @return `x`, invisibly.
#' @examples
#' ctx <- z3_context()
#' x <- z3_int(ctx, "x")
#' s <- z3_solver(ctx, simple = TRUE)
#' z3_add(s, x > 0)
#' if (z3_version_at_least("4.13.2")) z3_set_initial_value(s, x, 42)
#' z3_check(s)
#' @export
z3_set_initial_value <- function(x, var, value) {
  if (inherits(x, "z3_solver")) cpp_solver_initial_value(x, var, value)
  else if (inherits(x, "z3_optimize")) cpp_optimize_initial_value(x, var, value)
  else stop("x must be a z3_solver or z3_optimize", call. = FALSE)
  invisible(x)
}

#' Help text and DIMACS export
#'
#' `z3_help()` returns Z3's description of the parameters of a solver,
#' optimizer, fixedpoint engine, tactic or simplifier; for a context, it
#' describes the parameters of [z3_simplify()]. With `names_only = TRUE`
#' it returns only the parameter names, as Z3 lists them.
#' `z3_to_dimacs()` writes the assertions of a propositional solver or goal
#' in DIMACS CNF.
#'
#' @param x A `z3_solver`, `z3_optimize`, `z3_fixedpoint`, `z3_tactic`,
#'   `z3_simplifier`, `z3_context` or (for DIMACS) `z3_goal`.
#' @param names_only If `TRUE`, return only the parameter names.
#' @param include_names Whether to include variable names as comments.
#' @return A string.
#' @examples
#' ctx <- z3_context()
#' s <- z3_solver(ctx)
#' substr(z3_help(s), 1, 60)
#' z3_help(z3_tactic(ctx, "simplify"), names_only = TRUE)
#' substr(z3_help(ctx), 1, 60)
#' p <- z3_bools(ctx, c("a", "b"))
#' z3_add(s, p$a | p$b, !p$a)
#' cat(z3_to_dimacs(s))
#' @export
z3_help <- function(x, names_only = FALSE) {
  kinds <- c("z3_solver", "z3_optimize", "z3_tactic", "z3_simplifier", "z3_fixedpoint", "z3_context")
  if (!inherits(x, kinds)) {
    stop("x must be a solver, optimizer, fixedpoint engine, tactic, simplifier or context", call. = FALSE)
  }
  if (!is.logical(names_only) || length(names_only) != 1L || is.na(names_only)) {
    stop("names_only must be TRUE or FALSE", call. = FALSE)
  }
  if (names_only) return(cpp_param_names(x))
  if (inherits(x, "z3_solver")) return(cpp_solver_help(x))
  if (inherits(x, "z3_optimize")) return(cpp_optimize_help(x))
  if (inherits(x, "z3_tactic")) return(cpp_tactic_help(x))
  if (inherits(x, "z3_simplifier")) return(cpp_simplifier_help(x))
  if (inherits(x, "z3_fixedpoint")) return(cpp_fixedpoint_help(x))
  cpp_simplify_help_text(x)
}

#' @rdname z3_help
#' @export
z3_to_dimacs <- function(x, include_names = TRUE) {
  if (inherits(x, "z3_solver")) return(cpp_solver_to_dimacs(x, isTRUE(include_names)))
  if (inherits(x, "z3_goal")) return(cpp_goal_to_dimacs(x, isTRUE(include_names)))
  stop("x must be a z3_solver or z3_goal", call. = FALSE)
}

#' @rdname z3_help
#' @param src,dst `z3_solver` objects.
#' @details `z3_import_model_converter(src, dst)` gives `dst` the model
#'   converter of `src`, so that models of `dst` are translated the way
#'   `src` would translate them (useful when `dst` solves a preprocessed
#'   copy of `src`'s problem).
#' @export
z3_import_model_converter <- function(src, dst) {
  cpp_solver_import_model_converter(src, dst)
  invisible(dst)
}

#' Scoped solving and model enumeration
#'
#' `z3_with_scope(x, code)` evaluates `code` inside a [z3_push()] /
#' [z3_pop()] pair on a solver or optimizer: assertions added by `code` are
#' removed afterwards, also when `code` fails.
#'
#' `z3_all_models(solver, vars, limit)` lists the distinct assignments of
#' `vars` in models of the solver's assertions, at most `limit` of them. It
#' blocks each assignment found and checks again, inside a scope, so the
#' solver is unchanged afterwards. Values are converted as by
#' [z3_model_values()]. The `"status"` attribute says why enumeration
#' stopped: `"unsat"` (all assignments were found), `"limit"` or
#' `"unknown"`.
#'
#' @param x A `z3_solver` or `z3_optimize`.
#' @param code Code to evaluate.
#' @param solver A `z3_solver`.
#' @param vars A constant or a list of constants.
#' @param limit Maximum number of assignments.
#' @return `z3_with_scope()` returns the value of `code`. `z3_all_models()`
#'   returns a list of named lists, one per assignment, with attribute
#'   `"status"`.
#' @examples
#' ctx <- z3_context()
#' x <- z3_int(ctx, "x")
#' y <- z3_int(ctx, "y")
#' s <- z3_solver(ctx)
#' z3_add(s, x >= 0, y >= 0, x + y == 2)
#' z3_with_scope(s, {
#'   z3_add(s, x > 5)
#'   z3_check(s)
#' })
#' z3_check(s)
#' sols <- z3_all_models(s, list(x, y))
#' length(sols)
#' attr(sols, "status")
#' @export
z3_with_scope <- function(x, code) {
  z3_push(x)
  on.exit(z3_pop(x), add = TRUE)
  force(code)
}

#' @rdname z3_with_scope
#' @export
z3_all_models <- function(solver, vars, limit = 100) {
  if (!inherits(solver, "z3_solver")) stop("solver must be a z3_solver", call. = FALSE)
  vars <- as_list(vars)
  if (!length(vars) || !all(vapply(vars, z3_is_const, logical(1)))) {
    stop("vars must be one or more constants", call. = FALSE)
  }
  limit <- as_count(limit, "limit", positive = TRUE)
  found <- list()
  status <- "limit"
  z3_with_scope(solver, {
    while (length(found) < limit) {
      st <- z3_check(solver)
      if (st != "sat") {
        status <- st
        break
      }
      m <- z3_model(solver)
      vals <- lapply(vars, function(v) z3_eval(m, v, completion = TRUE))
      found[[length(found) + 1L]] <- vals
      z3_add(solver, z3_or(Map(function(v, val) v != val, vars, vals)))
    }
  })
  nm <- vapply(vars, format, character(1))
  out <- lapply(found, function(vals) stats::setNames(lapply(vals, value_or_expr), nm))
  structure(out, status = status)
}
