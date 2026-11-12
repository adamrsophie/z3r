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
#' z3_congruence_root(s, a)
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
#' same order. Needs Z3 4.14.0 or later.
#'
#' @param x A `z3_solver`.
#' @param vars A list of constants.
#' @return A list with elements `vars`, `terms` and `guards`, lists of
#'   `z3_expr`; `vars[[i]]` equals `terms[[i]]` under the guards.
#' @examples
#' ctx <- z3_context()
#' x <- z3_int(ctx, "x")
#' y <- z3_int(ctx, "y")
#' s <- z3_solver(ctx)
#' z3_add(s, x + 2 * y == 10)
#' z3_solve_for(s, list(x))
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
#' z3_set_initial_value(s, x, 42)
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
#' optimizer, tactic or simplifier. `z3_to_dimacs()` writes the assertions
#' of a propositional solver or goal in DIMACS CNF.
#'
#' @param x A `z3_solver`, `z3_optimize`, `z3_tactic`, `z3_simplifier` or
#'   (for DIMACS) `z3_goal`.
#' @param include_names Whether to include variable names as comments.
#' @return A string.
#' @examples
#' ctx <- z3_context()
#' s <- z3_solver(ctx)
#' substr(z3_help(s), 1, 60)
#' p <- z3_bools(ctx, c("a", "b"))
#' z3_add(s, p$a | p$b, !p$a)
#' cat(z3_to_dimacs(s))
#' @export
z3_help <- function(x) {
  if (inherits(x, "z3_solver")) return(cpp_solver_help(x))
  if (inherits(x, "z3_optimize")) return(cpp_optimize_help(x))
  if (inherits(x, "z3_tactic")) return(cpp_tactic_help(x))
  if (inherits(x, "z3_simplifier")) return(cpp_simplifier_help(x))
  stop("x must be a solver, optimizer, tactic or simplifier", call. = FALSE)
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
