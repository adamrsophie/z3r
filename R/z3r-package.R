#' z3r: Native R Interface to the Z3 Theorem Prover
#'
#' z3r binds R to the C API of the Z3 satisfiability modulo theories (SMT)
#' solver. Symbolic variables are created in a [z3_context()], combined with
#' ordinary R operators into constraints, and handed to a [z3_solver()] or a
#' [z3_optimize()] object.
#'
#' z3r is an independent third-party package. It is not developed, endorsed
#' or supported by Microsoft or by the Z3 development team.
#'
#' @section Getting started:
#' ```
#' ctx <- z3_context()
#' x <- z3_int(ctx, "x")
#' y <- z3_int(ctx, "y")
#' s <- z3_solver(ctx)
#' z3_add(s, x + y == 10, x > 2 * y, y > 0)
#' z3_check(s)
#' z3_model(s)
#' ```
#'
#' See `vignette("z3r")` for an introduction to SMT solving from R.
#'
#' @keywords internal
#' @useDynLib z3r, .registration = TRUE
#' @importFrom Rcpp sourceCpp
"_PACKAGE"
