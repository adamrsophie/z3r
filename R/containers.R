#' Dictionaries keyed by Z3 terms
#'
#' An AST map stores Z3 expressions under Z3 expression keys. Z3 shares
#' structurally equal terms, so `x + 1` built twice is the same key (but
#' `1 + x` is another one). Keys and values must be expressions of the
#' map's context; R numbers are not converted, since a number could stand
#' for an Int or a Real.
#'
#' * `z3_ast_map(ctx)` creates an empty map.
#' * `z3_ast_map_insert(map, key, value)` stores `value` under `key`,
#'   replacing any earlier value.
#' * `z3_ast_map_get(map, key)` returns the value stored under `key`, or
#'   `NULL` when there is none; `z3_ast_map_contains(map, key)` tests for
#'   the key.
#' * `z3_ast_map_erase(map, key)` removes a key (nothing happens when it is
#'   absent); `z3_ast_map_reset(map)` removes every key.
#' * `length()` counts the keys. `z3_ast_map_keys()` and
#'   `z3_ast_map_values()` list the keys and the values; for an unchanged
#'   map both come in the same order.
#' * `format()`, `print()` and `as.character()` show the map as Z3 prints
#'   it.
#'
#' A map is changed in place: every copy of a `z3_ast_map` object refers to
#' the same map.
#'
#' @param ctx A [z3_context()].
#' @param map,x A `z3_ast_map`.
#' @param key,value Expressions (`z3_expr`) of the map's context.
#' @param ... Unused.
#' @return `z3_ast_map()` returns a new `z3_ast_map`. `z3_ast_map_insert()`,
#'   `z3_ast_map_erase()` and `z3_ast_map_reset()` return `map` invisibly.
#'   `z3_ast_map_get()` returns a `z3_expr` or `NULL`,
#'   `z3_ast_map_contains()` `TRUE` or `FALSE`, `length()` an integer,
#'   `z3_ast_map_keys()` and `z3_ast_map_values()` lists of expressions,
#'   and `format()` a string.
#' @examples
#' ctx <- z3_context()
#' x <- z3_int(ctx, "x")
#' y <- z3_int(ctx, "y")
#' m <- z3_ast_map(ctx)
#' z3_ast_map_insert(m, x + 1, y)
#' z3_ast_map_insert(m, x, z3_int_val(ctx, 3))
#' z3_ast_map_get(m, x + 1)
#' z3_ast_map_contains(m, y)
#' length(m)
#' m
#' z3_ast_map_erase(m, x)
#' z3_ast_map_keys(m)
#' @export
z3_ast_map <- function(ctx) cpp_mk_ast_map(ctx)

#' @rdname z3_ast_map
#' @export
z3_ast_map_insert <- function(map, key, value) {
  cpp_ast_map_insert(map, key, value)
  invisible(map)
}

#' @rdname z3_ast_map
#' @export
z3_ast_map_get <- function(map, key) cpp_ast_map_find(map, key)

#' @rdname z3_ast_map
#' @export
z3_ast_map_contains <- function(map, key) cpp_ast_map_contains(map, key)

#' @rdname z3_ast_map
#' @export
z3_ast_map_erase <- function(map, key) {
  cpp_ast_map_erase(map, key)
  invisible(map)
}

#' @rdname z3_ast_map
#' @export
z3_ast_map_reset <- function(map) {
  cpp_ast_map_reset(map)
  invisible(map)
}

#' @rdname z3_ast_map
#' @export
z3_ast_map_keys <- function(map) cpp_ast_map_entries(map)$keys

#' @rdname z3_ast_map
#' @export
z3_ast_map_values <- function(map) cpp_ast_map_entries(map)$values

#' @rdname z3_ast_map
#' @export
length.z3_ast_map <- function(x) cpp_ast_map_size(x)

#' @rdname z3_ast_map
#' @export
format.z3_ast_map <- function(x, ...) {
  if (!cpp_is_valid(x)) return("<invalid z3_ast_map>")
  cpp_ast_map_to_string(x)
}

#' @export
print.z3_ast_map <- function(x, ...) {
  cat("<z3_ast_map>\n", format(x), "\n", sep = "")
  invisible(x)
}

#' @export
as.character.z3_ast_map <- function(x, ...) cpp_ast_map_to_string(x)

#' Z3's text for a list of terms
#'
#' Formats expressions the way Z3 prints a vector of terms: `(ast-vector`,
#' then each term on its own indented lines, then a closing parenthesis.
#'
#' @param terms An expression, or a non-empty list of expressions of one
#'   context.
#' @return A single string.
#' @examples
#' ctx <- z3_context()
#' x <- z3_int(ctx, "x")
#' cat(z3_format_terms(list(x + 1, x > 2)), "\n")
#' @export
z3_format_terms <- function(terms) cpp_ast_vector_to_string(as_list(terms))
