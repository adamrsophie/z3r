#' Check and record the result
#'
#' `z3_check_result()` checks a solver or optimizer like [z3_check()] and
#' returns everything about the answer in one object, for batches and
#' reports that should not rebuild it from the solver's changing state:
#'
#' * `status`: `"sat"`, `"unsat"` or `"unknown"`, and `reason` (why Z3
#'   gave up, for `"unknown"`, else `NULL`).
#' * `model` (for `"sat"`), `core` (the unsatisfiable core, for `"unsat"`)
#'   and `proof` (for `"unsat"` in a context made with
#'   `z3_context(proof = TRUE)`); each is `NULL` where it does not apply.
#' * `objectives`: for an optimizer, its objective terms.
#' * `seconds`: the wall-clock time of the check.
#' * `statistics`: Z3's statistics after the check.
#' * `params`: the parameters set on the solver, as Z3's text.
#' * `fingerprint`: a structural hash of the assertions, tracked labels
#'   and assumptions. Two checks of the same formulas have the same
#'   fingerprint in one Z3 version; it is not a cryptographic hash. It is
#'   `NA` when z3r cannot list the assertions (a solver with a simplifier
#'   that SMT-LIB was parsed into).
#' * `build`: [z3_build_info()].
#'
#' `z3_build_info()` reports how z3r was built and which Z3 it runs with:
#' the Z3 version (headers and loaded library), where configure found Z3
#' (`provider` is `"download"` for the pinned release configure downloads,
#' else `"system"`; `source` says how it was found), the downloaded
#' archive and its SHA-256 checksum, the path of the loaded Z3 library and
#' the compiler.
#'
#' @param x A `z3_solver` or `z3_optimize`.
#' @param ... Assumptions, as for [z3_check()].
#' @param timeout Timeout for this check in milliseconds, or `NULL`.
#' @return `z3_check_result()` returns an object of class
#'   `z3_check_result`, a list with the elements above. `z3_build_info()`
#'   returns a list.
#' @examples
#' ctx <- z3_context()
#' x <- z3_int(ctx, "x")
#' s <- z3_solver(ctx)
#' z3_add(s, x > 2, x < 5)
#' r <- z3_check_result(s)
#' r
#' r$model$x
#' r$fingerprint
#' z3_build_info()$provider
#' @export
z3_check_result <- function(x, ..., timeout = NULL) {
  if (!inherits(x, c("z3_solver", "z3_optimize"))) {
    stop("x must be a z3_solver or z3_optimize", call. = FALSE)
  }
  assumptions <- flatten_args(...)
  opt <- inherits(x, "z3_optimize")
  started <- proc.time()[["elapsed"]]
  status <- z3_check(x, assumptions, timeout = timeout)
  seconds <- proc.time()[["elapsed"]] - started
  maybe <- function(expr) tryCatch(expr, error = function(e) NULL)
  formulas <- tryCatch(c(z3_assertions(x), if (opt) cpp_optimize_tracked(x) else cpp_solver_tracked(x),
                         Filter(is_z3_expr, assumptions)),
                       error = function(e) NULL)
  structure(list(
    status = status,
    reason = if (status == "unknown") z3_reason_unknown(x) else NULL,
    model = if (status == "sat") maybe(z3_model(x)) else NULL,
    core = if (status == "unsat") maybe(z3_unsat_core(x)) else NULL,
    proof = if (status == "unsat" && !opt) maybe(z3_proof(x)) else NULL,
    objectives = if (opt) z3_objectives(x) else NULL,
    seconds = seconds,
    statistics = z3_statistics(x),
    params = cpp_kept_params(x),
    fingerprint = if (is.null(formulas)) NA_character_ else cpp_fingerprint(formulas),
    build = z3_build_info()
  ), class = "z3_check_result")
}

#' @export
print.z3_check_result <- function(x, ...) {
  cat(sprintf("<z3_check_result: %s in %.3f s>\n", x$status, x$seconds))
  if (!is.null(x$reason)) cat("reason:", x$reason, "\n")
  if (!is.null(x$model)) cat("model:", length(x$model), "constants\n")
  if (!is.null(x$core)) cat("core:", length(x$core), "labels or assumptions\n")
  if (nzchar(x$params)) cat("parameters set:", x$params, "\n")
  cat("fingerprint:", x$fingerprint, "\n")
  cat(sprintf("Z3 %s (%s)\n", x$build$z3_runtime, x$build$provider))
  invisible(x)
}

#' @rdname z3_check_result
#' @export
z3_build_info <- function() {
  v <- z3_version()
  info <- cpp_build_info()
  c(list(z3r = as.character(utils::packageVersion("z3r")),
         z3_headers = v$headers, z3_runtime = v$runtime), info)
}
