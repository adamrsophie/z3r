#' Sequences, regular expressions and characters
#'
#' Z3 sequences are finite lists over any element sort; strings are
#' sequences of characters. The string functions ([z3_length()],
#' [z3_concat()], [z3_contains()], [z3_prefixof()], [z3_suffixof()],
#' [z3_substr()], [z3_str_at()], [z3_indexof()], [z3_replace()]) work on any
#' sequence.
#'
#' * Sorts: `z3_seq_sort(element)`, `z3_re_sort(seq_sort)`,
#'   `z3_char_sort(ctx)`, and `z3_sort_basis()` for the element sort.
#' * Sequences: `z3_seq_empty(sort)`, `z3_seq_unit(x)` (one element),
#'   `z3_seq_nth(s, i)` (element `i`, 0-based), `z3_last_indexof(s, t)`
#'   (Z3 before 4.8.15 misses a match at the last position),
#'   `z3_replace_all()` (Z3 4.15.5), `z3_seq_power(s, n)` (`s` repeated `n`
#'   times, Z3 5.3.0), and the higher-order `z3_seq_map()`,
#'   `z3_seq_mapi()`, `z3_seq_foldl()`, `z3_seq_foldli()` (Z3 4.13.2), whose
#'   function argument is an array such as one made with [z3_lambda()].
#' * Regular expressions: `z3_re()` turns a string (or sequence) into a
#'   regular expression matching exactly it; `z3_in_re(s, re)` tests
#'   membership. Combinators: `z3_re_star()`, `z3_re_plus()`,
#'   `z3_re_option()`, `z3_re_union()`, `z3_re_concat()`,
#'   `z3_re_intersect()`, `z3_re_complement()`, `z3_re_diff()` (Z3 4.8.14),
#'   `z3_re_loop(re, lo, hi)`, `z3_re_power(re, n)` (Z3 4.8.15),
#'   `z3_re_range("a", "z", ctx)` (a character range), and the constants `z3_re_empty()`,
#'   `z3_re_full()` and `z3_re_allchar()` of a regular expression sort.
#'   Where a regular expression is expected, a string is accepted and
#'   converted with `z3_re()`. `z3_replace_re()` and `z3_replace_re_all()`
#'   replace matches (Z3 4.15.5); Z3's solver is incomplete for these and
#'   may answer `"unknown"`.
#' * Characters: `z3_char_val(ctx, code)` (Z3 4.8.15), `z3_char_to_int()`,
#'   `z3_char_to_bv()`, `z3_char_from_bv()`, `z3_char_is_digit()`,
#'   `z3_char_le()`, and `z3_str_from_code()`, `z3_str_to_code()` (Z3
#'   4.8.15) between one-character strings and code points.
#'
#' @param ctx A [z3_context()].
#' @param element,sort,seq_sort Sorts.
#' @param s,t,dst Sequence (or string) expressions.
#' @param x An element, character, or Int expression.
#' @param i,n Int expressions or R whole numbers.
#' @param re,re1,re2 Regular expressions (or strings).
#' @param lo,hi Bounds: single-character strings for `z3_re_range()`,
#'   counts for `z3_re_loop()`.
#' @param code A Unicode code point.
#' @param f An array-valued function (for example a [z3_lambda()]).
#' @param acc Initial accumulator for folds.
#' @param ... Regular expressions or strings.
#' @return A `z3_expr` or `z3_sort`.
#' @examples
#' ctx <- z3_context()
#' s <- z3_string(ctx, "s")
#' digits <- z3_re_plus(z3_re_range("0", "9", ctx))
#' sol <- z3_solver(ctx)
#' z3_add(sol, z3_in_re(s, z3_re_concat("id-", digits)), z3_length(s) == 6)
#' z3_check(sol)
#' z3_value(z3_eval(z3_model(sol), s))
#'
#' I <- z3_int_sort(ctx)
#' q <- z3_const(ctx, "q", z3_seq_sort(I))
#' sol <- z3_solver(ctx)
#' z3_add(sol, z3_length(q) == 3, z3_seq_nth(q, 0) == 7,
#'        z3_contains(q, z3_concat(z3_seq_unit(z3_int_val(ctx, 1)), z3_seq_unit(z3_int_val(ctx, 2)))))
#' z3_check(sol)
#' z3_eval(z3_model(sol), q)
#' @name z3_sequences
NULL

seq_op <- function(op, ...) cpp_seq_op(op, list(...), integer())

#' @rdname z3_sequences
#' @export
z3_seq_sort <- function(element) cpp_seq_sort(element)

#' @rdname z3_sequences
#' @export
z3_re_sort <- function(seq_sort) cpp_re_sort(seq_sort)

#' @rdname z3_sequences
#' @export
z3_char_sort <- function(ctx) cpp_char_sort(ctx)

#' @rdname z3_sequences
#' @export
z3_sort_basis <- function(sort) cpp_sort_basis(sort)

#' @rdname z3_sequences
#' @export
z3_seq_empty <- function(sort) cpp_seq_constant("empty", sort)

#' @rdname z3_sequences
#' @export
z3_seq_unit <- function(x) seq_op("unit", x)

#' @rdname z3_sequences
#' @export
z3_seq_nth <- function(s, i) seq_op("nth", s, i)

#' @rdname z3_sequences
#' @export
z3_last_indexof <- function(s, t) seq_op("last_indexof", s, t)

#' @rdname z3_sequences
#' @export
z3_replace_all <- function(s, t, dst) seq_op("replace_all", s, t, dst)

#' @rdname z3_sequences
#' @export
z3_replace_re <- function(s, re, dst) seq_op("replace_re", s, re, dst)

#' @rdname z3_sequences
#' @export
z3_replace_re_all <- function(s, re, dst) seq_op("replace_re_all", s, re, dst)

#' @rdname z3_sequences
#' @export
z3_seq_power <- function(s, n) seq_op("seq_power", s, n)

#' @rdname z3_sequences
#' @export
z3_seq_map <- function(f, s) seq_op("map", f, s)

#' @rdname z3_sequences
#' @export
z3_seq_mapi <- function(f, i, s) seq_op("mapi", f, i, s)

#' @rdname z3_sequences
#' @export
z3_seq_foldl <- function(f, acc, s) seq_op("foldl", f, acc, s)

#' @rdname z3_sequences
#' @export
z3_seq_foldli <- function(f, i, acc, s) seq_op("foldli", f, i, acc, s)

#' @rdname z3_sequences
#' @export
z3_re <- function(s) {
  if (is.character(s)) stop("z3_re() needs a Z3 expression; use z3_string_val() for literals, or pass the string directly where a regular expression is expected", call. = FALSE)
  seq_op("to_re", s)
}

#' @rdname z3_sequences
#' @export
z3_in_re <- function(s, re) seq_op("in_re", s, re)

#' @rdname z3_sequences
#' @export
z3_re_star <- function(re) seq_op("star", re_lift(re))

#' @rdname z3_sequences
#' @export
z3_re_plus <- function(re) seq_op("plus", re_lift(re))

#' @rdname z3_sequences
#' @export
z3_re_option <- function(re) seq_op("option", re_lift(re))

#' @rdname z3_sequences
#' @export
z3_re_complement <- function(re) seq_op("complement", re_lift(re))

#' @rdname z3_sequences
#' @export
z3_re_union <- function(...) re_nary("re_union", flatten_args(...))

#' @rdname z3_sequences
#' @export
z3_re_concat <- function(...) re_nary("re_concat", flatten_args(...))

#' @rdname z3_sequences
#' @export
z3_re_intersect <- function(...) re_nary("re_intersect", flatten_args(...))

#' @rdname z3_sequences
#' @export
z3_re_diff <- function(re1, re2) cpp_seq_op("re_diff", list(re_lift(re1), re2), integer())

#' @rdname z3_sequences
#' @export
z3_re_loop <- function(re, lo, hi) {
  cpp_seq_op("re_loop", list(re_lift(re)), c(as_count(lo, "lo"), as_count(hi, "hi")))
}

#' @rdname z3_sequences
#' @export
z3_re_power <- function(re, n) cpp_seq_op("re_power", list(re_lift(re)), as_count(n, "n"))

#' @rdname z3_sequences
#' @export
z3_re_range <- function(lo, hi, ctx = NULL) {
  if (is.character(lo) && is.character(hi)) {
    if (is.null(ctx)) {
      stop("z3_re_range() with two R strings needs ctx", call. = FALSE)
    }
    lo <- z3_string_val(ctx, lo)
  }
  seq_op("re_range", lo, hi)
}

#' @rdname z3_sequences
#' @export
z3_re_empty <- function(sort) cpp_seq_constant("re_empty", sort)

#' @rdname z3_sequences
#' @export
z3_re_full <- function(sort) cpp_seq_constant("re_full", sort)

#' @rdname z3_sequences
#' @export
z3_re_allchar <- function(sort) cpp_seq_constant("re_allchar", sort)

#' @rdname z3_sequences
#' @export
z3_char_val <- function(ctx, code) cpp_char_val(ctx, code)

#' @rdname z3_sequences
#' @export
z3_char_to_int <- function(x) seq_op("char_to_int", x)

#' @rdname z3_sequences
#' @export
z3_char_to_bv <- function(x) seq_op("char_to_bv", x)

#' @rdname z3_sequences
#' @export
z3_char_from_bv <- function(x) seq_op("char_from_bv", x)

#' @rdname z3_sequences
#' @export
z3_char_is_digit <- function(x) seq_op("char_is_digit", x)

#' @rdname z3_sequences
#' @export
z3_char_le <- function(x, t) seq_op("char_le", x, t)

#' @rdname z3_sequences
#' @export
z3_str_from_code <- function(x) seq_op("str_from_code", x)

#' @rdname z3_sequences
#' @export
z3_str_to_code <- function(x) seq_op("str_to_code", x)

# Regular-expression arguments: R strings are not context-aware, so the C++
# side converts sequences with str.to_re once a context is known.
re_lift <- function(re) {
  if (is.character(re)) {
    stop("give the context: use z3_string_val(ctx, \"...\") inside regular expression constructors",
         call. = FALSE)
  }
  re
}

re_nary <- function(op, args) {
  if (!any(vapply(args, is_z3_expr, TRUE))) {
    stop("at least one argument must be a Z3 expression to determine the context", call. = FALSE)
  }
  cpp_seq_op(op, args, integer())
}
