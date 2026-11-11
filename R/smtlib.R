#' SMT-LIB2 import and export
#'
#' Exchange constraints with other SMT solvers and tools in the SMT-LIB 2
#' format, using Z3's own printer and parser.
#'
#' * `z3_to_smtlib()` writes a solver's or optimizer's assertions (and
#'   objectives) as an SMT-LIB2 script with the needed declarations, or a
#'   list of Boolean expressions as an SMT-LIB2 benchmark.
#' * `z3_from_smtlib()` parses SMT-LIB2 text and returns the asserted
#'   formulas as a list of `z3_expr`. Declarations in the text create
#'   constants in `ctx`; a declared constant is the same object as one made
#'   with the same name and sort, for example by [z3_int()]. Commands other
#'   than declarations and assertions (such as `check-sat`) are ignored.
#' * `z3_read_smtlib()` does the same for a file.
#' * `z3_add_smtlib()` parses text directly into a solver or optimizer.
#' * `z3_eval_smtlib()` runs SMT-LIB2 commands in Z3's interpreter and
#'   returns its output text, exactly as the `z3` command-line tool would
#'   print it. Its declarations are kept in the context.
#'
#' Parse errors are reported as R errors.
#'
#' @param x A `z3_solver`, a `z3_optimize`, or a list of Boolean `z3_expr`.
#' @param logic SMT-LIB logic for the benchmark header when `x` is a list.
#' @param ctx A [z3_context()].
#' @param text SMT-LIB2 text, a single string (multiple strings are joined
#'   with newlines).
#' @param file Path to an SMT-LIB2 file.
#' @return `z3_to_smtlib()` and `z3_eval_smtlib()` return a string;
#'   `z3_from_smtlib()` and `z3_read_smtlib()` return a list of `z3_expr`;
#'   `z3_add_smtlib()` returns `x` invisibly.
#' @examples
#' ctx <- z3_context()
#' x <- z3_int(ctx, "x")
#' s <- z3_solver(ctx)
#' z3_add(s, x > 2, x < 5)
#' txt <- z3_to_smtlib(s)
#' cat(txt)
#' fs <- z3_from_smtlib(ctx, txt)
#' s2 <- z3_solver(ctx)
#' z3_add(s2, fs)
#' z3_check(s2)
#' z3_eval_smtlib(ctx, "(declare-const y Int) (assert (> y 3)) (check-sat)")
#' @export
z3_to_smtlib <- function(x, logic = "") {
  if (inherits(x, "z3_solver")) return(cpp_solver_to_string(x))
  if (inherits(x, "z3_optimize")) return(cpp_optimize_to_string(x))
  if (is_z3_expr(x)) x <- list(x)
  if (is.list(x) && length(x) > 0L) return(cpp_benchmark(x, logic))
  stop("x must be a z3_solver, a z3_optimize or a non-empty list of expressions",
       call. = FALSE)
}

#' @rdname z3_to_smtlib
#' @export
z3_from_smtlib <- function(ctx, text) {
  cpp_parse_smtlib2(ctx, paste(text, collapse = "\n"), FALSE)
}

#' @rdname z3_to_smtlib
#' @export
z3_read_smtlib <- function(ctx, file) {
  if (!is.character(file) || length(file) != 1L || !file.exists(file)) {
    stop("file must be the path of an existing file", call. = FALSE)
  }
  z3_from_smtlib(ctx, readLines(file, warn = FALSE, encoding = "UTF-8"))
}

#' @rdname z3_to_smtlib
#' @export
z3_add_smtlib <- function(x, text) {
  text <- paste(text, collapse = "\n")
  if (inherits(x, "z3_solver")) {
    cpp_solver_from_string(x, text)
  } else if (inherits(x, "z3_optimize")) {
    cpp_optimize_from_string(x, text)
  } else {
    stop("x must be a z3_solver or a z3_optimize", call. = FALSE)
  }
  invisible(x)
}

#' @rdname z3_to_smtlib
#' @export
z3_eval_smtlib <- function(ctx, text) {
  cpp_eval_smtlib2(ctx, paste(text, collapse = "\n"))
}
