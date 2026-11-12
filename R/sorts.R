#' Z3 sorts
#'
#' Sorts are the types of Z3 expressions. Every constant and every
#' expression has exactly one sort, and Z3 operations only combine
#' expressions of compatible sorts.
#'
#' * `z3_int_sort()`: mathematical integers of unbounded size.
#' * `z3_real_sort()`: exact real numbers (rationals and, in models of
#'   nonlinear problems, algebraic numbers). Not floating point.
#' * `z3_bool_sort()`: the Booleans.
#' * `z3_bv_sort()`: fixed-width bit-vectors (machine integers).
#' * `z3_string_sort()`: Unicode strings.
#' * `z3_array_sort()`: total maps from a domain sort to a range sort.
#' * `z3_uninterpreted_sort()`: an abstract sort with no structure except
#'   equality.
#'
#' @param ctx A [z3_context()].
#' @param width Bit-vector width in bits, a positive integer.
#' @param domain,range Sorts of array indices and array elements.
#' @param name Name of the uninterpreted sort.
#' @return An object of class `z3_sort`.
#' @seealso [z3_sort()] to get the sort of an expression, [z3_const()].
#' @examples
#' ctx <- z3_context()
#' z3_int_sort(ctx)
#' z3_bv_sort(ctx, 8)
#' z3_array_sort(z3_int_sort(ctx), z3_bool_sort(ctx))
#' @name z3_sorts
NULL

#' @rdname z3_sorts
#' @export
z3_int_sort <- function(ctx) cpp_int_sort(ctx)

#' @rdname z3_sorts
#' @export
z3_real_sort <- function(ctx) cpp_real_sort(ctx)

#' @rdname z3_sorts
#' @export
z3_bool_sort <- function(ctx) cpp_bool_sort(ctx)

#' @rdname z3_sorts
#' @export
z3_bv_sort <- function(ctx, width) cpp_bv_sort(ctx, as_count(width, "width", positive = TRUE))

#' @rdname z3_sorts
#' @export
z3_string_sort <- function(ctx) cpp_string_sort(ctx)

#' @rdname z3_sorts
#' @export
z3_array_sort <- function(domain, range) {
  if (inherits(domain, "z3_sort")) return(cpp_array_sort(domain, range))
  cpp_array_sort_n(as_list(domain), range)
}

#' @rdname z3_sorts
#' @export
z3_uninterpreted_sort <- function(ctx, name) cpp_uninterpreted_sort(ctx, name)

#' Sort of an expression
#'
#' @param x A `z3_expr`.
#' @return For `z3_sort()`, the `z3_sort` of `x`. For `z3_sort_kind()`, one
#'   of `"bool"`, `"int"`, `"real"`, `"bitvector"`, `"array"`, `"string"`,
#'   `"uninterpreted"` or another lower-case Z3 sort kind; `x` may be an
#'   expression or a sort.
#' @examples
#' ctx <- z3_context()
#' x <- z3_int(ctx, "x")
#' z3_sort(x)
#' z3_sort_kind(x / 2)
#' @export
z3_sort <- function(x) cpp_sort_of(x)

#' @rdname z3_sort
#' @export
z3_sort_kind <- function(x) cpp_sort_kind(x)

#' @export
format.z3_sort <- function(x, ...) {
  if (!cpp_is_valid(x)) return("<invalid z3_sort>")
  cpp_sort_to_string(x)
}

#' @export
print.z3_sort <- function(x, ...) {
  cat("<z3_sort> ", format(x), "\n", sep = "")
  invisible(x)
}

#' @export
as.character.z3_sort <- function(x, ...) format(x)

#' @export
Ops.z3_sort <- function(e1, e2) {
  if (.Generic == "==") return(cpp_sort_equal(e1, e2))
  if (.Generic == "!=") return(!cpp_sort_equal(e1, e2))
  stop("only == and != are defined for Z3 sorts", call. = FALSE)
}
