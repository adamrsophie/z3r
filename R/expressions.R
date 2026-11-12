#' Symbolic constants
#'
#' Create named symbolic constants (the "variables" of an SMT problem). Two
#' constants with the same name and sort in the same context are the same
#' constant.
#'
#' `z3_ints()`, `z3_reals()` and `z3_bools()` create several constants at
#' once and return them as a named list.
#'
#' @param ctx A [z3_context()].
#' @param name Name of the constant, a single string. For the plural forms,
#'   a character vector of names.
#' @param sort A `z3_sort`, see [z3_sorts].
#' @param width Bit-vector width in bits.
#' @param prefix Prefix of the generated name for `z3_fresh_const()`.
#' @return A `z3_expr`, or a named list of them for the plural forms.
#' @seealso [z3_int_val()] for numeric constants, [z3_array()],
#'   [z3_function()].
#' @examples
#' ctx <- z3_context()
#' x <- z3_int(ctx, "x")
#' y <- z3_real(ctx, "y")
#' p <- z3_bool(ctx, "p")
#' b <- z3_bv(ctx, "b", 8)
#' v <- z3_ints(ctx, c("a", "b", "c"))
#' v$a + v$b + v$c
#' @export
z3_const <- function(ctx, name, sort) cpp_mk_const(ctx, name, sort)

#' @rdname z3_const
#' @export
z3_int <- function(ctx, name) cpp_mk_const(ctx, name, cpp_int_sort(ctx))

#' @rdname z3_const
#' @export
z3_real <- function(ctx, name) cpp_mk_const(ctx, name, cpp_real_sort(ctx))

#' @rdname z3_const
#' @export
z3_bool <- function(ctx, name) cpp_mk_const(ctx, name, cpp_bool_sort(ctx))

#' @rdname z3_const
#' @export
z3_bv <- function(ctx, name, width) {
  cpp_mk_const(ctx, name, z3_bv_sort(ctx, width))
}

#' @rdname z3_const
#' @export
z3_string <- function(ctx, name) cpp_mk_const(ctx, name, cpp_string_sort(ctx))

#' @rdname z3_const
#' @export
z3_ints <- function(ctx, name) many_consts(ctx, name, cpp_int_sort(ctx))

#' @rdname z3_const
#' @export
z3_reals <- function(ctx, name) many_consts(ctx, name, cpp_real_sort(ctx))

#' @rdname z3_const
#' @export
z3_bools <- function(ctx, name) many_consts(ctx, name, cpp_bool_sort(ctx))

#' @rdname z3_const
#' @export
z3_fresh_const <- function(ctx, prefix, sort) cpp_mk_fresh_const(ctx, prefix, sort)

many_consts <- function(ctx, name, sort) {
  if (!is.character(name) || anyNA(name) || anyDuplicated(name)) {
    stop("name must be a character vector of distinct, non-missing names", call. = FALSE)
  }
  stats::setNames(lapply(name, function(n) cpp_mk_const(ctx, n, sort)), name)
}

#' Exact numeric, Boolean and string values
#'
#' Build Z3 values without going through floating point.
#'
#' `z3_int_val()` accepts a string of decimal digits with an optional sign,
#' a whole number with absolute value at most 2^53, an R integer, or a
#' `gmp::bigz`. `z3_real_val()` accepts an exact decimal (`"0.125"`,
#' `"1.5e-3"`), a fraction (`"1/3"`, `"-22/7"`), an R number, or a
#' `gmp::bigq`. Strings are parsed exactly, so `z3_real_val(ctx, "0.1")` is
#' the rational 1/10.
#'
#' @section Converting R doubles:
#' When an R number is combined with a Z3 expression, as in `x + 0.1`, it
#' is converted as follows:
#'
#' * Whole numbers become exact integers. Whole numbers beyond 2^53 are
#'   rejected, because R may already have rounded them; pass them as
#'   strings or `gmp::bigz` instead.
#' * Other finite doubles become the exact rational value of their
#'   shortest decimal representation that converts back to the same double,
#'   so `0.1` becomes 1/10 rather than the binary approximation
#'   3602879701896397/36028797018963968. This matches what R prints.
#' * `NA`, `NaN` and `Inf` are rejected.
#'
#' Set `options(z3r.doubles = "error")` to reject non-integer doubles and
#' require exact strings everywhere.
#'
#' @param ctx A [z3_context()].
#' @param value The value; see Details.
#' @param width Bit-vector width in bits.
#' @param sort Target sort for `z3_val()`.
#' @return A `z3_expr`.
#' @examples
#' ctx <- z3_context()
#' z3_int_val(ctx, "123456789012345678901234567890")
#' z3_real_val(ctx, "1/3")
#' z3_real_val(ctx, "0.125")
#' z3_bv_val(ctx, -1, 8)
#' z3_string_val(ctx, "hello")
#' z3_val(ctx, 2, z3_real_sort(ctx))
#' @export
z3_int_val <- function(ctx, value) {
  if (is.character(value)) {
    value <- check_numeral(value, "^[+-]?[0-9]+$", "an integer such as \"-42\"")
    return(cpp_mk_numeral(ctx, value, cpp_int_sort(ctx)))
  }
  if (is.numeric(value) && length(value) == 1L && !is.na(value) && value != floor(value)) {
    stop("value is not a whole number; use z3_real_val() for rationals", call. = FALSE)
  }
  cpp_lift(ctx, value, cpp_int_sort(ctx))
}

#' @rdname z3_int_val
#' @export
z3_real_val <- function(ctx, value) {
  if (is.character(value)) {
    value <- check_numeral(
      value, "^[+-]?(([0-9]+[.]?[0-9]*|[.][0-9]+)([eE][+-]?[0-9]+)?|[0-9]+/[0-9]+)$",
      "a decimal such as \"0.125\" or a fraction such as \"1/3\""
    )
    if (grepl("/0+$", value)) stop("the denominator must not be zero", call. = FALSE)
    return(cpp_mk_numeral(ctx, value, cpp_real_sort(ctx)))
  }
  cpp_lift(ctx, value, cpp_real_sort(ctx))
}

#' @rdname z3_int_val
#' @export
z3_bool_val <- function(ctx, value) {
  if (!is.logical(value) || length(value) != 1L || is.na(value)) {
    stop("value must be TRUE or FALSE", call. = FALSE)
  }
  cpp_lift(ctx, value, cpp_bool_sort(ctx))
}

#' @rdname z3_int_val
#' @export
z3_bv_val <- function(ctx, value, width) {
  width <- as_count(width, "width", positive = TRUE)
  if (is.character(value)) {
    value <- check_numeral(value, "^-?[0-9]+$", "an integer such as \"255\"")
    return(cpp_mk_bv_numeral(ctx, value, width))
  }
  cpp_lift(ctx, value, cpp_bv_sort(ctx, width))
}

#' @rdname z3_int_val
#' @export
z3_string_val <- function(ctx, value) {
  if (!is.character(value) || length(value) != 1L || is.na(value)) {
    stop("value must be a single non-missing string", call. = FALSE)
  }
  cpp_lift(ctx, value, cpp_string_sort(ctx))
}

#' @rdname z3_int_val
#' @export
z3_val <- function(ctx, value, sort) cpp_lift(ctx, value, sort)

check_numeral <- function(value, pattern, what) {
  if (length(value) != 1L || is.na(value)) {
    stop("value must be a single non-missing string", call. = FALSE)
  }
  value <- trimws(value)
  if (!grepl(pattern, value)) {
    stop(sprintf("\"%s\" is not %s", value, what), call. = FALSE)
  }
  value
}

#' @export
format.z3_expr <- function(x, ...) {
  if (!cpp_is_valid(x)) return("<invalid z3_expr>")
  cpp_expr_to_string(x)
}

#' @export
print.z3_expr <- function(x, ...) {
  if (!cpp_is_valid(x)) {
    cat("<z3_expr: invalid (restored from a saved session)>\n")
  } else {
    cat("<z3_expr ", cpp_sort_to_string(cpp_sort_of(x)), "> ", format(x), "\n", sep = "")
  }
  invisible(x)
}

#' @export
as.character.z3_expr <- function(x, ...) format(x)

#' Convert a Z3 value to an R value
#'
#' Converts a value expression, typically the result of [z3_eval()], to the
#' closest R type that represents it exactly.
#'
#' | Z3 value | R result |
#' |---|---|
#' | Bool | `TRUE` or `FALSE` |
#' | Int, Real, bit-vector | a double when exactly representable; otherwise a `gmp::bigz` or `gmp::bigq` if gmp is installed, or else a character string holding the exact value (`"1/3"`) |
#' | algebraic number | error when `exact = TRUE`; a nearby double when `exact = FALSE` |
#' | String | a UTF-8 character string |
#' | floating point | a double (`NaN`, `Inf`, `-0` included); formats wider than double precision only when the value fits exactly, or rounded when `exact = FALSE` |
#' | rounding mode | its name, such as `"roundNearestTiesToEven"` |
#'
#' A double is "exactly representable" when its numerator has absolute
#' value at most 2^53 and its denominator is a power of two; for example
#' `1/4` converts to `0.25`, but `1/3` does not convert to `0.3333...`
#' unless `exact = FALSE`.
#'
#' Symbolic expressions are not values: evaluate them in a model first.
#'
#' @param x A `z3_expr` that is a value.
#' @param exact If `FALSE`, numbers that are not exactly representable as
#'   doubles (including algebraic numbers) are rounded to a nearby double.
#' @param signed For bit-vectors, interpret the bits as a two's complement
#'   signed number.
#' @return An R value; see the table.
#' @examples
#' ctx <- z3_context()
#' z3_value(z3_int_val(ctx, 42))
#' z3_value(z3_real_val(ctx, "1/4"))
#' z3_value(z3_real_val(ctx, "1/3"), exact = FALSE)
#' z3_value(z3_bv_val(ctx, 255, 8), signed = TRUE)
#' @export
z3_value <- function(x, exact = TRUE, signed = FALSE) {
  if (!is_z3_expr(x)) stop("x must be a z3_expr", call. = FALSE)
  if (isTRUE(signed) && cpp_sort_kind(x) == "bitvector") {
    x <- cpp_simplify(cpp_op("bv2int", list(x), 1L))
  }
  info <- cpp_value_info(x, 20L)
  switch(info$kind,
    bool = info$value,
    string = info$value,
    int = , real = , bv = number_value(info, exact),
    fp = if (info$exact_double || !exact) info$value else stop(sprintf(
      "the floating point value %s is not exactly representable as a double; use exact = FALSE to round it",
      info$text
    ), call. = FALSE),
    rm = info$value,
    algebraic = if (exact) {
      stop(sprintf(
        "%s is an irrational algebraic number with no exact R representation; use exact = FALSE for a double or z3_decimal() for digits",
        info$text
      ), call. = FALSE)
    } else {
      info$approx
    },
    symbolic = stop(
      "x is a symbolic expression, not a value; evaluate it in a model with z3_eval() first",
      call. = FALSE
    ),
    stop(sprintf("values of sort %s cannot be converted to R",
                 cpp_sort_to_string(cpp_sort_of(x))), call. = FALSE)
  )
}

number_value <- function(info, exact) {
  if (info$exact_double) return(info$value)
  if (!exact) return(info$approx)
  if (requireNamespace("gmp", quietly = TRUE)) {
    if (info$kind == "real") return(gmp::as.bigq(info$exact))
    return(gmp::as.bigz(info$exact))
  }
  info$exact
}

#' Decimal digits of a number
#'
#' Returns a decimal string for an exact number or an algebraic number,
#' rounded to `digits` digits after the decimal point. Z3 marks inexact
#' results with a trailing `?`.
#'
#' @param x A numeric `z3_expr` value.
#' @param digits Number of digits after the decimal point.
#' @return A character string.
#' @examples
#' ctx <- z3_context()
#' z3_decimal(z3_real_val(ctx, "1/3"), 5)
#' @export
z3_decimal <- function(x, digits = 10) {
  info <- cpp_value_info(x, as_count(digits, "digits"))
  if (info$kind == "algebraic") return(info$decimal)
  if (!info$kind %in% c("int", "real", "bv")) stop("x must be a numeric value", call. = FALSE)
  cpp_decimal_string(x, as_count(digits, "digits"))
}

#' @export
as.double.z3_expr <- function(x, ...) {
  v <- z3_value(x, exact = FALSE)
  if (!is.numeric(v)) stop("x is not a numeric value", call. = FALSE)
  v
}

#' @export
as.integer.z3_expr <- function(x, ...) {
  v <- z3_value(x)
  if (!is.numeric(v) || v != floor(v) || abs(v) > .Machine$integer.max) {
    stop("x is not an integer value within R's integer range", call. = FALSE)
  }
  as.integer(v)
}

#' @export
as.logical.z3_expr <- function(x, ...) {
  if (cpp_sort_kind(x) != "bool") stop("x is not a Boolean value", call. = FALSE)
  z3_value(x)
}

#' Simplify an expression
#'
#' Applies Z3's algebraic simplifier. The result is equivalent to the input
#' but not necessarily simpler in every case.
#'
#' Named parameters select options of Z3's simplifier, listed by
#' [z3_simplify_help()].
#'
#' @param x A `z3_expr`.
#' @param ... Simplifier parameters.
#' @return A `z3_expr`.
#' @examples
#' ctx <- z3_context()
#' x <- z3_int(ctx, "x")
#' z3_simplify(x + 2 * x - 3 + 3)
#' z3_simplify(z3_int_val(ctx, 7) %/% -2)
#' z3_simplify((x + 1) * (x + 1), som = TRUE)
#' @export
z3_simplify <- function(x, ...) {
  if (...length() == 0L) return(cpp_simplify(x))
  cpp_simplify_ex(x, params_list(...))
}

#' Structural equality of expressions
#'
#' Tests whether two expressions are the same term (after Z3's internal
#' hash-consing). This is not logical equivalence: `x + 1` and `1 + x` are
#' different terms. Use `==` to build a symbolic equation instead.
#'
#' Sorts and function declarations are compared the same way.
#'
#' @param x,y `z3_expr`, `z3_sort` or `z3_func_decl` objects.
#' @return `TRUE` or `FALSE`.
#' @examples
#' ctx <- z3_context()
#' z3_identical(z3_int(ctx, "x"), z3_int(ctx, "x"))
#' z3_identical(z3_int_sort(ctx), z3_sort(z3_int(ctx, "x")))
#' @export
z3_identical <- function(x, y) {
  if (inherits(x, "z3_function")) x <- z3_decl(x)
  if (inherits(y, "z3_function")) y <- z3_decl(y)
  if (inherits(x, c("z3_sort", "z3_func_decl"))) {
    if (!identical(class(x), class(y))) return(FALSE)
    return(cpp_decl_identity(x, y))
  }
  cpp_expr_equal(x, y)
}

#' Test for z3r objects
#'
#' @param x Any R object.
#' @return `TRUE` if `x` is a Z3 expression.
#' @examples
#' ctx <- z3_context()
#' is_z3_expr_value <- z3_is_expr(z3_int(ctx, "x"))
#' @export
z3_is_expr <- function(x) is_z3_expr(x)

#' @rdname z3_decimal
#' @details `z3_binary_string()` returns the binary digits of an integer
#'   numeral.
#' @export
z3_binary_string <- function(x) cpp_binary_string(x)
