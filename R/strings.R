#' String operations
#'
#' Z3 strings are sequences of Unicode characters. Character literals in
#' R are converted to Z3 string values exactly. Positions are 0-based, as
#' in SMT-LIB.
#'
#' * `z3_length()`: number of characters (an Int).
#' * `z3_concat()`: concatenation (see [z3_bitvectors]; it also joins
#'   strings).
#' * `z3_contains(s, t)`, `z3_prefixof(t, s)`, `z3_suffixof(t, s)`:
#'   containment tests. Note the SMT-LIB argument order: `z3_prefixof(t, s)`
#'   is "`t` is a prefix of `s`".
#' * `z3_substr(s, offset, length)`, `z3_str_at(s, i)`: substrings.
#' * `z3_indexof(s, t, offset)`: first position of `t` in `s` at or after
#'   `offset`, or -1.
#' * `z3_replace(s, src, dst)`: replaces the first occurrence of `src`.
#' * `z3_str_to_int()`, `z3_int_to_str()`: conversion between decimal
#'   strings and non-negative integers (`-1` for invalid strings).
#' * `z3_str_lt()`, `z3_str_le()`: lexicographic order by code point.
#'
#' @param s,t,src,dst String expressions or R strings.
#' @param offset,length,i Int expressions or R whole numbers.
#' @param x An Int (for `z3_int_to_str()`) or String expression.
#' @return A `z3_expr`.
#' @examples
#' ctx <- z3_context()
#' s <- z3_string(ctx, "s")
#' sol <- z3_solver(ctx)
#' z3_add(sol, z3_length(s) == 5, z3_prefixof("ab", s), z3_contains(s, "zz"))
#' z3_check(sol)
#' z3_value(z3_eval(z3_model(sol), s))
#' @name z3_strings
NULL

#' @rdname z3_strings
#' @export
z3_length <- function(s) cpp_op("length", list(s), integer())

#' @rdname z3_strings
#' @export
z3_contains <- function(s, t) cpp_op("contains", list(s, t), integer())

#' @rdname z3_strings
#' @export
z3_prefixof <- function(t, s) cpp_op("prefixof", list(t, s), integer())

#' @rdname z3_strings
#' @export
z3_suffixof <- function(t, s) cpp_op("suffixof", list(t, s), integer())

#' @rdname z3_strings
#' @export
z3_substr <- function(s, offset, length) cpp_op("substr", list(s, offset, length), integer())

#' @rdname z3_strings
#' @export
z3_str_at <- function(s, i) cpp_op("str_at", list(s, i), integer())

#' @rdname z3_strings
#' @export
z3_indexof <- function(s, t, offset = 0) cpp_op("indexof", list(s, t, offset), integer())

#' @rdname z3_strings
#' @export
z3_replace <- function(s, src, dst) cpp_op("replace", list(s, src, dst), integer())

#' @rdname z3_strings
#' @export
z3_str_to_int <- function(s) cpp_op("str_to_int", list(s), integer())

#' @rdname z3_strings
#' @export
z3_int_to_str <- function(x) cpp_op("int_to_str", list(x), integer())

#' @rdname z3_strings
#' @export
z3_str_lt <- function(s, t) cpp_op("str_lt", list(s, t), integer())

#' @rdname z3_strings
#' @export
z3_str_le <- function(s, t) cpp_op("str_le", list(s, t), integer())

#' String values as escaped text or bytes
#'
#' `z3_string_escaped()` returns a string value in SMT-LIB form: characters
#' outside printable ASCII are written as `\u{...}` escapes (hexadecimal
#' code points), and so is a backslash followed by `u`. Unlike
#' [z3_value()], it works for strings containing U+0000, and
#' `z3_string_val(ctx, z3_string_escaped(x), escaped = TRUE)` gives `x`
#' back.
#'
#' `z3_string_bytes()` returns the characters of a string value as a raw
#' vector, one byte per character. Every character must be below U+0100;
#' the result is the input of [z3_string_val()] for raw vectors.
#'
#' @param x A `z3_expr` that is a string value, such as the result of
#'   [z3_eval()].
#' @return A character string for `z3_string_escaped()`; a raw vector for
#'   `z3_string_bytes()`.
#' @examples
#' ctx <- z3_context()
#' z3_string_escaped(z3_string_val(ctx, "caf\u00e9"))
#' z3_string_bytes(z3_string_val(ctx, as.raw(c(0, 65, 255))))
#' @export
z3_string_escaped <- function(x) {
  if (!is_z3_expr(x)) stop("x must be a z3_expr", call. = FALSE)
  cpp_string_escaped(x)
}

#' @rdname z3_string_escaped
#' @export
z3_string_bytes <- function(x) {
  if (!is_z3_expr(x)) stop("x must be a z3_expr", call. = FALSE)
  cpp_string_bytes(x)
}
