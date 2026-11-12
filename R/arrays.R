#' Arrays
#'
#' Z3 arrays are total functions from a domain sort to a range sort, with
#' no length. `z3_select(a, i)` reads index `i`; `z3_store(a, i, v)` is a
#' new array equal to `a` except that index `i` maps to `v`.
#' `z3_const_array()` maps every index to the same value.
#'
#' Arrays may have several index sorts (give `domain` as a list); then
#' `z3_select(a, i, j)` takes one index per sort and `z3_store(a, list(i, j),
#' v)` takes the indices as a list.
#'
#' * `z3_array_map(f, a, b, ...)` applies a function pointwise.
#' * `z3_array_default(a)` is the value at all but finitely many indices.
#' * `z3_array_ext(a, b)` is an index at which two different arrays differ.
#' * `z3_as_array(f)` turns a function into an array.
#' * `z3_lambda(vars, body)` builds an array from an expression over bound
#'   constants, as in `z3_lambda(i, i * i)`.
#'
#' @param ctx A [z3_context()].
#' @param name Name of the array constant.
#' @param domain,range `z3_sort` objects for indices (or a list of them) and
#'   elements.
#' @param a An array expression.
#' @param i An index (expression or R scalar), or for `z3_store()` on a
#'   multi-index array, a list of indices.
#' @param ... Further indices for multi-index arrays, or arrays for
#'   `z3_array_map()`.
#' @param f A `z3_function` or `z3_func_decl`.
#' @param b Another array.
#' @param vars A constant or list of constants to bind.
#' @param body An expression.
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
  cpp_mk_const(ctx, name, z3_array_sort(domain, range))
}

#' @rdname z3_array
#' @export
z3_select <- function(a, i, ...) cpp_array_access(a, c(list(i), list(...)), NULL, FALSE)

#' @rdname z3_array
#' @export
z3_store <- function(a, i, v) cpp_array_access(a, as_list(i), v, TRUE)

#' @rdname z3_array
#' @export
z3_const_array <- function(domain, value) cpp_const_array(domain, value)

#' @rdname z3_array
#' @export
z3_array_map <- function(f, ...) cpp_array_map(z3_decl(f), flatten_args(...))

#' @rdname z3_array
#' @export
z3_array_default <- function(a) cpp_array_unary("default", a, NULL)

#' @rdname z3_array
#' @export
z3_array_ext <- function(a, b) cpp_array_unary("ext", a, b)

#' @rdname z3_array
#' @export
z3_as_array <- function(f) cpp_as_array(z3_decl(f))

#' @rdname z3_array
#' @export
z3_lambda <- function(vars, body) cpp_lambda(as_list(vars), body)

#' Sets
#'
#' Sets are arrays from an element sort to Bool. `z3_set_sort()` builds the
#' sort; `z3_empty_set()` and `z3_full_set()` the extremes;
#' `z3_set_add()`, `z3_set_del()`, `z3_set_union()`,
#' `z3_set_intersect()`, `z3_set_difference()` and `z3_set_complement()`
#' combine sets; `z3_set_member()` and `z3_set_subset()` are tests.
#'
#' @param element Element sort.
#' @param s,t Set expressions.
#' @param x An element.
#' @param ... Sets, or lists of sets.
#' @return A `z3_sort` or `z3_expr`.
#' @examples
#' ctx <- z3_context()
#' I <- z3_int_sort(ctx)
#' a <- z3_const(ctx, "a", z3_set_sort(I))
#' s <- z3_solver(ctx)
#' z3_add(s, z3_set_member(3, a), z3_set_subset(a, z3_set_add(z3_empty_set(I), 3)))
#' z3_check(s)
#' @export
z3_set_sort <- function(element) cpp_set_sort(element)

#' @rdname z3_set_sort
#' @export
z3_empty_set <- function(element) cpp_set_constant(FALSE, element)

#' @rdname z3_set_sort
#' @export
z3_full_set <- function(element) cpp_set_constant(TRUE, element)

#' @rdname z3_set_sort
#' @export
z3_set_add <- function(s, x) cpp_set_op("add", list(s, x))

#' @rdname z3_set_sort
#' @export
z3_set_del <- function(s, x) cpp_set_op("del", list(s, x))

#' @rdname z3_set_sort
#' @export
z3_set_union <- function(...) cpp_set_op("union", flatten_args(...))

#' @rdname z3_set_sort
#' @export
z3_set_intersect <- function(...) cpp_set_op("intersect", flatten_args(...))

#' @rdname z3_set_sort
#' @export
z3_set_difference <- function(s, t) cpp_set_op("difference", list(s, t))

#' @rdname z3_set_sort
#' @export
z3_set_complement <- function(s) cpp_set_op("complement", list(s))

#' @rdname z3_set_sort
#' @export
z3_set_member <- function(x, s) cpp_set_op("member", list(x, s))

#' @rdname z3_set_sort
#' @export
z3_set_subset <- function(s, t) cpp_set_op("subset", list(s, t))

#' @rdname z3_array
#' @param x An array expression or array sort.
#' @return `z3_array_sort_info()` returns a list with the index sorts
#'   (`domain`) and element sort (`range`).
#' @export
z3_array_sort_info <- function(x) cpp_array_sort_info(x)
