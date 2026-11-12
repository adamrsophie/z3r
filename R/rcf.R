#' Real closed fields
#'
#' Exact computation with real algebraic numbers, the transcendental
#' constants pi and e, and infinitesimals, independent of any solver. Values
#' are `z3_rcf` objects with arithmetic (`+ - * /`, `^` with a whole
#' exponent, unary `-`) and comparison operators.
#'
#' * `z3_rcf(ctx, "1/3")` makes a rational; `z3_rcf_pi()`, `z3_rcf_e()` and
#'   `z3_rcf_infinitesimal()` the special elements.
#' * `z3_rcf_roots(coeffs)` finds all real roots of the polynomial
#'   `coeffs[[1]] + coeffs[[2]] x + ...`, exactly.
#' * `format()` prints the number; `z3_rcf_decimal(x, digits)` gives
#'   decimal digits; `z3_rcf_fraction()` the numerator and denominator;
#'   `z3_rcf_info()` (Z3 4.12.3) classifies the number and, for algebraic
#'   numbers, gives the defining polynomial, isolating interval and sign
#'   conditions.
#'
#' @param ctx A [z3_context()].
#' @param value An exact rational as a string, or an R whole number.
#' @param coeffs A list of `z3_rcf` coefficients, constant term first.
#' @param x A `z3_rcf`.
#' @param digits Digits after the decimal point.
#' @param compact Use the compact notation for extensions.
#' @param ... Unused.
#' @return A `z3_rcf`, a list of them, a string or a list.
#' @examples
#' ctx <- z3_context()
#' two <- z3_rcf(ctx, 2)
#' r <- z3_rcf_roots(list(-two, z3_rcf(ctx, 0), z3_rcf(ctx, 1)))  # x^2 - 2
#' r
#' z3_rcf_decimal(r[[2]], 20)
#' r[[2]]^2 == two
#' z3_rcf_pi(ctx) > z3_rcf(ctx, "314159/100000")
#' eps <- z3_rcf_infinitesimal(ctx)
#' eps > 0 & eps < z3_rcf(ctx, "1/1000000")
#' @export
z3_rcf <- function(ctx, value) {
  if (is.numeric(value)) {
    if (length(value) != 1L || !is.finite(value) || value != floor(value) || abs(value) > 2^53) {
      stop("numeric values must be whole numbers up to 2^53; give rationals as strings", call. = FALSE)
    }
    value <- format(value, scientific = FALSE)
  }
  value <- check_numeral(value, "^[+-]?[0-9]+(/[0-9]+)?$", "a rational such as \"-3/7\"")
  cpp_rcf_mk(ctx, "rational", value)
}

#' @rdname z3_rcf
#' @export
z3_rcf_pi <- function(ctx) cpp_rcf_mk(ctx, "pi", NULL)

#' @rdname z3_rcf
#' @export
z3_rcf_e <- function(ctx) cpp_rcf_mk(ctx, "e", NULL)

#' @rdname z3_rcf
#' @export
z3_rcf_infinitesimal <- function(ctx) cpp_rcf_mk(ctx, "infinitesimal", NULL)

#' @rdname z3_rcf
#' @export
z3_rcf_roots <- function(coeffs) cpp_rcf_roots(coeffs)

#' @rdname z3_rcf
#' @export
z3_rcf_decimal <- function(x, digits = 10) cpp_rcf_string(x, FALSE, as_count(digits, "digits"))

#' @rdname z3_rcf
#' @export
z3_rcf_fraction <- function(x) cpp_rcf_fraction(x)

#' @rdname z3_rcf
#' @export
z3_rcf_info <- function(x) cpp_rcf_info(x)

#' @rdname z3_rcf
#' @export
format.z3_rcf <- function(x, compact = FALSE, ...) {
  if (!cpp_is_valid(x)) return("<invalid z3_rcf>")
  cpp_rcf_string(x, isTRUE(compact), -1L)
}

#' @export
print.z3_rcf <- function(x, ...) {
  cat("<z3_rcf> ", format(x), "\n", sep = "")
  invisible(x)
}

#' @export
Ops.z3_rcf <- function(e1, e2) {
  if (missing(e2)) {
    if (.Generic == "-") return(cpp_rcf_op("neg", e1, NULL, 0L))
    if (.Generic == "+") return(e1)
    stop(sprintf("unary %s is not defined for real closed field numbers", .Generic), call. = FALSE)
  }
  if (.Generic == "^") {
    if (!is.numeric(e2) || length(e2) != 1L || e2 != floor(e2)) {
      stop("exponents must be whole numbers", call. = FALSE)
    }
    if (e2 < 0) return(cpp_rcf_op("inv", cpp_rcf_op("power", e1, NULL, as.integer(-e2)), NULL, 0L))
    return(cpp_rcf_op("power", e1, NULL, as.integer(e2)))
  }
  if (.Generic %in% c("&", "|", "%%", "%/%")) {
    stop(sprintf("%s is not defined for real closed field numbers", .Generic), call. = FALSE)
  }
  ctx <- z3_context_of(if (inherits(e1, "z3_rcf")) e1 else e2)
  if (!inherits(e1, "z3_rcf")) e1 <- z3_rcf(ctx, e1)
  if (!inherits(e2, "z3_rcf")) e2 <- z3_rcf(ctx, e2)
  cpp_rcf_op(.Generic, e1, e2, 0L)
}

#' Algebraic number arithmetic
#'
#' Exact arithmetic on Z3 numeral expressions, including the irrational
#' algebraic numbers that appear in models of nonlinear real problems (see
#' [z3_value()]). The results are again exact numerals.
#'
#' * `z3_algebraic(op, a, b)` with `op` one of `"+"`, `"-"`, `"*"`, `"/"`,
#'   or a comparison (`"<"`, `">"`, `"<="`, `">="`, `"=="`, `"!="`, which
#'   return `TRUE`/`FALSE`).
#' * `z3_algebraic_root(a, k)` is the k-th root; `z3_algebraic_power(a, k)`
#'   the k-th power; `z3_algebraic_sign(a)` -1, 0 or 1.
#' * `z3_algebraic_bounds(a, precision)` gives rational lower and upper
#'   bounds within 10^-precision.
#' * `z3_algebraic_poly(a)` gives the coefficients of the defining
#'   polynomial and the index of the root it denotes.
#' * `z3_algebraic_roots(p, values)` finds the roots of polynomial `p` in
#'   its last free variable after substituting `values` for the others, and
#'   `z3_algebraic_eval(p, values)` gives the sign of `p` at `values`. The
#'   variables of `p` are bound variables made with [z3_bound_var()].
#' * `z3_subresultants(p, q, x)` returns the subresultants of polynomials
#'   `p` and `q` with respect to `x`.
#'
#' @param op Operation name.
#' @param a,b Numerals or algebraic numbers (R numbers are accepted for
#'   `b`).
#' @param k A positive whole number.
#' @param p,q Polynomial expressions.
#' @param values A list of values.
#' @param x The variable.
#' @return A `z3_expr`, a logical, an integer, or a list.
#' @examples
#' ctx <- z3_context()
#' two <- z3_real_val(ctx, 2)
#' r <- z3_algebraic_root(two, 2)
#' r
#' z3_algebraic("*", r, r)
#' z3_algebraic("<", r, z3_real_val(ctx, "1.5"))
#' z3_algebraic_poly(r)
#' @export
z3_algebraic <- function(op, a, b) cpp_algebraic_op(op, a, b, 0L)

#' @rdname z3_algebraic
#' @export
z3_algebraic_root <- function(a, k) cpp_algebraic_op("root", a, NULL, as_count(k, "k", TRUE))

#' @rdname z3_algebraic
#' @export
z3_algebraic_power <- function(a, k) cpp_algebraic_op("power", a, NULL, as_count(k, "k"))

#' @rdname z3_algebraic
#' @export
z3_algebraic_sign <- function(a) cpp_algebraic_op("sign", a, NULL, 0L)

#' @rdname z3_algebraic
#' @export
z3_algebraic_poly <- function(a) cpp_algebraic_poly(a)

#' @rdname z3_algebraic
#' @export
z3_algebraic_roots <- function(p, values = list()) cpp_algebraic_roots(p, as_list(values), FALSE)

#' @rdname z3_algebraic
#' @export
z3_algebraic_eval <- function(p, values = list()) cpp_algebraic_roots(p, as_list(values), TRUE)

#' @rdname z3_algebraic
#' @export
z3_subresultants <- function(p, q, x) cpp_polynomial_subresultants(p, q, x)

#' @rdname z3_algebraic
#' @param precision Number of decimal digits of the interval width.
#' @export
z3_algebraic_bounds <- function(a, precision = 10) {
  cpp_algebraic_bounds(a, as_count(precision, "precision"))
}
