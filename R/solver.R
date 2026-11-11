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
z3_solver <- function(ctx, logic = NULL, ...) {
  s <- cpp_mk_solver(ctx, logic)
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
  invisible(x)
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
#' or `rlimit` with [z3_set_params()] there.
#'
#' @param x A `z3_solver` or `z3_optimize`.
#' @param ... Assumptions: Boolean expressions or lists of them.
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
#' @export
z3_check <- function(x, ...) UseMethod("z3_check")

#' @export
z3_check.z3_solver <- function(x, ...) cpp_solver_check(x, flatten_args(...))

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

#' Incremental solving: scopes
#'
#' `z3_push()` opens a backtracking scope; `z3_pop()` removes every
#' assertion added since the matching push. `z3_reset()` removes all
#' assertions and scopes. `z3_num_scopes()` reports the number of open
#' scopes.
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
#' @param x A `z3_solver` or `z3_optimize`.
#' @return A named list of numbers (counts, memory in megabytes, time in
#'   seconds) reported by Z3 for the last check.
#' @examples
#' ctx <- z3_context()
#' s <- z3_solver(ctx)
#' z3_add(s, z3_int(ctx, "x") > 0)
#' z3_check(s)
#' names(z3_statistics(s))
#' @export
z3_statistics <- function(x) UseMethod("z3_statistics")

#' @export
z3_statistics.z3_solver <- function(x) cpp_solver_statistics(x)

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
