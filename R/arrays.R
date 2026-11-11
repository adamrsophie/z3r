#' Arrays
#'
#' Z3 arrays are total functions from a domain sort to a range sort, with
#' no length. `z3_select(a, i)` reads index `i`; `z3_store(a, i, v)` is a
#' new array equal to `a` except that index `i` maps to `v`.
#' `z3_const_array()` maps every index to the same value.
#'
#' @param ctx A [z3_context()].
#' @param name Name of the array constant.
#' @param domain,range `z3_sort` objects for indices and elements.
#' @param a An array expression.
#' @param i An index (expression or R scalar).
#' @param v A value (expression or R scalar).
#' @param value The value of every element of a constant array.
#' @return A `z3_expr`.
#' @examples
#' ctx <- z3_context()
#' a <- z3_array(ctx, "a", z3_int_sort(ctx), z3_int_sort(ctx))
#' i <- z3_int(ctx, "i")
#' s <- z3_solver(ctx)
#' z3_add(s, z3_select(z3_store(a, i, 10), i) != 10)
#' z3_check(s)
#' zeros <- z3_const_array(z3_int_sort(ctx), 0)
#' z3_simplify(z3_select(zeros, 5))
#' @export
z3_array <- function(ctx, name, domain, range) {
  cpp_mk_const(ctx, name, cpp_array_sort(domain, range))
}

#' @rdname z3_array
#' @export
z3_select <- function(a, i) cpp_op("select", list(a, i), integer())

#' @rdname z3_array
#' @export
z3_store <- function(a, i, v) cpp_op("store", list(a, i, v), integer())

#' @rdname z3_array
#' @export
z3_const_array <- function(domain, value) cpp_const_array(domain, value)
