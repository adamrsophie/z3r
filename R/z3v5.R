#' Complex numbers, finite fields and finite sets (Z3 5.x)
#'
#' Theories added in the Z3 5 series. Each needs z3r to be built against a
#' Z3 that has it; otherwise the functions report the version they need.
#'
#' * Complex numbers (Z3 5.3.2): `z3_complex_sort(ctx)`,
#'   `z3_complex(re, im)`, `z3_real_part(z)`, `z3_imag_part(z)`.
#' * Finite fields (Z3 5.3.0): `z3_ff_sort(ctx, p)` is the field of integers
#'   modulo the prime `p` (given as a string for large primes);
#'   `z3_ff_add()`, `z3_ff_mul()`, `z3_ff_neg()` and `z3_ff_bitsum()`
#'   (sum of the arguments weighted by successive powers of two) operate in
#'   it; [z3_val()] makes field elements. `z3_ff_size()` returns `p`.
#' * Finite sets (Z3 5.0.0): `z3_finite_set_sort(element)` is the sort of
#'   finite sets with cardinality, unlike the array-based sets of
#'   [z3_set_sort()]. Operations: `z3_fs_empty()`, `z3_fs_singleton()`,
#'   `z3_fs_union()`, `z3_fs_intersect()`, `z3_fs_difference()`,
#'   `z3_fs_member()`, `z3_fs_subset()`, `z3_fs_size()` (an Int),
#'   `z3_fs_range(lo, hi)` (the integers from `lo` to `hi`), and the
#'   higher-order `z3_fs_map(f, s)` and `z3_fs_filter(f, s)` with array
#'   arguments such as [z3_lambda()]. `z3_fs_element_sort()` gives the
#'   element sort.
#'
#' @param ctx A [z3_context()].
#' @param re,im Real parts.
#' @param z A complex expression.
#' @param p A prime, as a string or whole number.
#' @param element Element sort.
#' @param sort A finite set or finite field sort.
#' @param s,t Finite sets.
#' @param x An element.
#' @param lo,hi Int bounds.
#' @param f An array-valued function.
#' @param ... Field elements.
#' @return A `z3_expr`, a `z3_sort`, or for `z3_ff_size()` a string.
#' @examples
#' ctx <- z3_context()
#' if (z3_version_at_least("5.3.2")) {
#'   z <- z3_const(ctx, "z", z3_complex_sort(ctx))
#'   s <- z3_solver(ctx)
#'   z3_add(s, z3_real_part(z) == 3, z3_imag_part(z) == -4)
#'   z3_check(s)
#' }
#' if (z3_version_at_least("5.0.0")) {
#'   I <- z3_int_sort(ctx)
#'   a <- z3_const(ctx, "a", z3_finite_set_sort(I))
#'   s <- z3_solver(ctx)
#'   z3_add(s, z3_fs_size(a) == 3, z3_fs_subset(a, z3_fs_range(z3_int_val(ctx, 1), 4)))
#'   z3_check(s)
#' }
#' @name z3_v5_theories
NULL

v5 <- function(op, ...) cpp_v5_op(op, list(...))

#' @rdname z3_v5_theories
#' @export
z3_complex_sort <- function(ctx) cpp_v5_sort(ctx, "complex", NULL)

#' @rdname z3_v5_theories
#' @export
z3_complex <- function(re, im) v5("complex", re, im)

#' @rdname z3_v5_theories
#' @export
z3_real_part <- function(z) v5("real_part", z)

#' @rdname z3_v5_theories
#' @export
z3_imag_part <- function(z) v5("imag_part", z)

#' @rdname z3_v5_theories
#' @export
z3_ff_sort <- function(ctx, p) {
  if (is.numeric(p)) p <- format(p, scientific = FALSE)
  cpp_v5_sort(ctx, "finite_field", check_numeral(p, "^[0-9]+$", "a prime such as \"7\""))
}

#' @rdname z3_v5_theories
#' @export
z3_ff_size <- function(sort) cpp_v5_sort_info(sort)

#' @rdname z3_v5_theories
#' @export
z3_ff_add <- function(...) cpp_v5_op("ff_add", flatten_args(...))

#' @rdname z3_v5_theories
#' @export
z3_ff_mul <- function(...) cpp_v5_op("ff_mul", flatten_args(...))

#' @rdname z3_v5_theories
#' @export
z3_ff_neg <- function(x) v5("ff_neg", x)

#' @rdname z3_v5_theories
#' @export
z3_ff_bitsum <- function(...) cpp_v5_op("ff_bitsum", flatten_args(...))

#' @rdname z3_v5_theories
#' @export
z3_finite_set_sort <- function(element) cpp_v5_sort(z3_context_of(element), "finite_set", element)

#' @rdname z3_v5_theories
#' @export
z3_fs_element_sort <- function(sort) cpp_v5_sort_info(sort)

#' @rdname z3_v5_theories
#' @export
z3_fs_empty <- function(sort) cpp_fs_empty(sort)

#' @rdname z3_v5_theories
#' @export
z3_fs_singleton <- function(x) v5("fs_singleton", x)

#' @rdname z3_v5_theories
#' @export
z3_fs_union <- function(s, t) v5("fs_union", s, t)

#' @rdname z3_v5_theories
#' @export
z3_fs_intersect <- function(s, t) v5("fs_intersect", s, t)

#' @rdname z3_v5_theories
#' @export
z3_fs_difference <- function(s, t) v5("fs_difference", s, t)

#' @rdname z3_v5_theories
#' @export
z3_fs_member <- function(x, s) v5("fs_member", x, s)

#' @rdname z3_v5_theories
#' @export
z3_fs_subset <- function(s, t) v5("fs_subset", s, t)

#' @rdname z3_v5_theories
#' @export
z3_fs_size <- function(s) v5("fs_size", s)

#' @rdname z3_v5_theories
#' @export
z3_fs_range <- function(lo, hi) v5("fs_range", lo, hi)

#' @rdname z3_v5_theories
#' @export
z3_fs_map <- function(f, s) v5("fs_map", f, s)

#' @rdname z3_v5_theories
#' @export
z3_fs_filter <- function(f, s) v5("fs_filter", f, s)
