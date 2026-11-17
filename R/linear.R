#' Linear constraints from a coefficient matrix
#'
#' `z3_linear()` builds the rows of a linear system `A %*% x op b` as Z3
#' constraints in one native call, which is much faster than building them
#' with R operators row by row.
#'
#' `A` is either a dense matrix (numeric, integer or character) or a
#' sparse matrix in triplet form: a data frame (or list) with columns
#' `row`, `col` and `value`, together with `nrow`. Coefficients and
#' right-hand sides are converted exactly, as R operands of Z3 terms are:
#' integers and whole doubles exactly, other doubles by the
#' `z3r.doubles` policy, and strings such as `"1/3"` as exact rationals.
#' Use character values (or `gmp` numbers in a list) for exact fractions.
#' Missing values are refused. Zero coefficients are left out.
#'
#' No R recycling is applied: `x` must have one variable per column, and
#' `op` and `b` must have length 1 (used for every row) or one element per
#' row.
#'
#' @param A A matrix, or a data frame or list with `row`, `col` and
#'   `value` (1-based row and column numbers).
#' @param x A list of Int or Real terms, one per column of `A`.
#' @param op Comparison: `"<="`, `"<"`, `">="`, `">"`, `"=="` or `"!="`.
#' @param b Right-hand sides: a vector or list.
#' @param nrow Number of rows, for a triplet `A` (by default the largest
#'   row number).
#' @return A list of Boolean `z3_expr`, one per row, named by the row
#'   names of a dense `A`.
#' @examples
#' ctx <- z3_context()
#' x <- z3_ints(ctx, c("a", "b", "c"))
#' A <- rbind(c(1, 2, 0), c(0, 1, -1))
#' cons <- z3_linear(A, x, "<=", c(10, 0))
#' cons
#' s <- z3_solver(ctx)
#' z3_add(s, cons, z3_linear(diag(3), x, ">=", 1))
#' z3_check(s)
#' # Sparse triplets with exact fractions:
#' y <- z3_reals(ctx, c("p", "q"))
#' z3_linear(data.frame(row = c(1, 1), col = 1:2, value = c("1/3", "2/3")),
#'           y, "==", "1")
#' @export
z3_linear <- function(A, x, op = "<=", b, nrow = NULL) {
  x <- as_list(x)
  if (!length(x) || !all(vapply(x, is_z3_expr, logical(1)))) {
    stop("x must be a non-empty list of Z3 terms", call. = FALSE)
  }
  if (is.matrix(A)) {
    if (ncol(A) != length(x)) {
      stop(sprintf("A has %d columns but x has %d variables", ncol(A), length(x)), call. = FALSE)
    }
    if (anyNA(A)) stop("A must not contain missing values", call. = FALSE)
    m <- nrow(A)
    nz <- which(if (is.character(A)) A != "0" else A != 0, arr.ind = TRUE)
    nz <- nz[order(nz[, 1L], nz[, 2L]), , drop = FALSE]
    row <- as.integer(nz[, 1L])
    col <- as.integer(nz[, 2L])
    val <- as.list(A[nz])
    rn <- rownames(A)
  } else if (is.list(A) && all(c("row", "col", "value") %in% names(A))) {
    row <- A$row
    col <- A$col
    if (!is.numeric(row) || !is.numeric(col) || anyNA(row) || anyNA(col) ||
        any(row != floor(row)) || any(col != floor(col)) || length(row) != length(col) ||
        length(A$value) != length(row)) {
      stop("row and col must be whole numbers of the same length as value", call. = FALSE)
    }
    if (any(vapply(A$value, function(v) !is_z3_expr(v) && anyNA(v), NA))) {
      stop("A must not contain missing values", call. = FALSE)
    }
    m <- if (is.null(nrow)) as.integer(max(c(0, row))) else as_count(nrow, "nrow")
    row <- as.integer(row)
    col <- as.integer(col)
    val <- as.list(A$value)
    rn <- NULL
  } else {
    stop("A must be a matrix, or a data frame or list with row, col and value", call. = FALSE)
  }
  if (!is.character(op) || anyNA(op) || !(length(op) %in% c(1L, m))) {
    stop(sprintf("op must be a string or have one element per row (%d)", m), call. = FALSE)
  }
  b <- as.list(b)
  if (!(length(b) %in% c(1L, m))) {
    stop(sprintf("b must have length 1 or one element per row (%d)", m), call. = FALSE)
  }
  if (is.matrix(A) && !is.numeric(A) && !is.character(A)) {
    stop("A must be a numeric or character matrix", call. = FALSE)
  }
  if (any(vapply(b, function(v) !is_z3_expr(v) && (length(v) != 1L || is.na(v)), logical(1)))) {
    stop("b must hold single non-missing values", call. = FALSE)
  }
  # Strings are exact numbers here (elsewhere they are string literals).
  ctx <- z3_context_of(x[[1]])
  num <- function(v) {
    if (!is.character(v)) return(v)
    if (grepl("^[+-]?[0-9]+$", v)) z3_int_val(ctx, v) else z3_real_val(ctx, v)
  }
  out <- cpp_linear(x, row, col, lapply(val, num), m, op, lapply(b, num))
  if (!is.null(rn)) names(out) <- rn
  out
}
