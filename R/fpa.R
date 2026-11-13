#' IEEE 754 floating point
#'
#' Z3's floating point theory models IEEE 754 binary formats exactly,
#' including rounding, signed zeros, infinities and NaN. A format has
#' `ebits` exponent bits and `sbits` significand bits (counting the hidden
#' bit): `z3_fp_sort(ctx, 11, 53)` is double precision, the format of R's
#' numbers. The standard formats also have names: `z3_fp_sort(ctx, "half")`,
#' `"single"`, `"double"` and `"quadruple"`, or by width
#' `z3_fp_sort(ctx, 16)`, `32`, `64` and `128`, the same as `z3_fp16_sort()`,
#' `z3_fp32_sort()`, `z3_fp64_sort()` and `z3_fp128_sort()`.
#'
#' **Values.** `z3_fp(ctx, name, sort)` makes a constant. R numbers combined
#' with a floating point expression are converted to its format only when
#' the format represents them exactly; `NaN`, `Inf` and `-0` are kept.
#' `z3_fp_val(ctx, x, sort, rounding)` rounds an R number into a format
#' explicitly. `z3_fp_nan()`, `z3_fp_inf()` and `z3_fp_zero()` give the
#' special values. [z3_value()] converts numerals back to R doubles
#' exactly.
#'
#' **Operators.** `+`, `-`, `*`, `/` and unary `-` use round-to-nearest,
#' ties-to-even (`"RNE"`), the rounding of R's own arithmetic; `%%` is the
#' IEEE remainder. `<`, `<=`, `>`, `>=` are IEEE comparisons (false when
#' either side is NaN). `==` and `!=` are **structural** equality, as for
#' every other sort: `NaN == NaN` is true and `0 == -0` is false. Use
#' `z3_fp_eq()` for IEEE equality, where the opposite holds.
#'
#' **Explicit rounding.** `z3_rounding(ctx, mode)` gives a rounding mode:
#' `"RNE"` (nearest, ties to even), `"RNA"` (nearest, ties away from zero),
#' `"RTP"` (toward +Inf), `"RTN"` (toward -Inf), `"RTZ"` (toward zero), or
#' by its SMT-LIB name, which [z3_value()] returns:
#' `"roundNearestTiesToEven"`, `"roundNearestTiesToAway"`,
#' `"roundTowardPositive"`, `"roundTowardNegative"`, `"roundTowardZero"`.
#' `z3_fp_add()`, `z3_fp_sub()`, `z3_fp_mul()`, `z3_fp_div()`,
#' `z3_fp_fma()`, `z3_fp_sqrt()` and `z3_fp_round_to_integral()` take it as
#' their first argument.
#'
#' **Other operations.** `z3_fp_rem()`, `z3_fp_min()`, `z3_fp_max()`,
#' `z3_fp_abs()` (also `abs()`), `z3_fp_neg()`; predicates `z3_fp_is_nan()`,
#' `z3_fp_is_infinite()`, `z3_fp_is_zero()`, `z3_fp_is_normal()`,
#' `z3_fp_is_subnormal()`, `z3_fp_is_negative()`, `z3_fp_is_positive()`;
#' comparisons `z3_fp_lt()`, `z3_fp_leq()`, `z3_fp_gt()`, `z3_fp_geq()`,
#' `z3_fp_eq()`.
#'
#' **Conversions.** `z3_fp_to_fp(rounding, x, sort)` converts another
#' format, a Real or an Int; `z3_fp_from_bv(bv, sort)` reinterprets IEEE
#' bits; `z3_fp_from_signed()`/`z3_fp_from_unsigned()` convert a bit-vector
#' integer; `z3_fp_from_real_exp(rounding, exponent, significand, sort)`
#' rounds `significand * 2^exponent` from an Int exponent and a Real
#' significand, which may be symbolic;
#' `z3_fp_from_bits(sign, exponent, significand)` assembles the
#' fields; `z3_fp_from_parts()` builds a numeral exactly from its sign,
#' unbiased exponent and fraction bits; `z3_fp_to_ubv()`/`z3_fp_to_sbv()` round to a bit-vector integer
#' of width `size`; `z3_fp_to_real()` gives the exact Real value;
#' `z3_fp_to_ieee_bv()` the bit pattern. `z3_fp_numeral_parts()` describes
#' a numeral (class, sign, exponent and significand).
#'
#' @param ctx A [z3_context()].
#' @param ebits,sbits Exponent and significand widths. With `sbits` omitted,
#'   `ebits` names a standard format: `"half"`, `"single"`, `"double"`,
#'   `"quadruple"`, or a width 16, 32, 64 or 128.
#' @param name Name of a constant.
#' @param sort A floating point sort.
#' @param x,y,z Floating point expressions or R numbers (any value for
#'   `z3_fp_to_fp()`).
#' @param rounding A rounding mode from `z3_rounding()`.
#' @param mode Rounding mode name, short (`"RNE"`) or SMT-LIB
#'   (`"roundNearestTiesToEven"`).
#' @param negative Sign of an infinity or zero.
#' @param bv,sign,exponent,significand Bit-vector expressions; for
#'   `z3_fp_from_real_exp()`, an Int exponent and a Real significand.
#' @param size Width of the resulting bit-vector.
#' @param biased For `z3_fp_numeral_parts()`: report the biased exponent.
#' @param numeric For `z3_fp_numeral_parts()`: also give the sign, exponent
#'   and fraction as numbers.
#' @return A `z3_expr` or `z3_sort`; `z3_fp_sort_info()` returns the two
#'   widths; `z3_fp_numeral_parts()` a list: `class` (`"normal"`,
#'   `"subnormal"`, `"zero"`, `"infinite"` or `"nan"`) and `negative`;
#'   `significand` and `exponent` as decimal strings for finite values; and
#'   for every value but NaN the fields as bit-vectors (`sign_bv`,
#'   `exponent_bv`, `significand_bv`). With `numeric = TRUE`, values other
#'   than NaN also have `sign` (0 or 1), `exponent_value` and `fraction`
#'   (the fraction bits without the hidden bit, as an integer), each a
#'   double while exact (at most 2^53 in magnitude) and a decimal string
#'   beyond.
#' @examples
#' ctx <- z3_context()
#' D <- z3_fp64_sort(ctx)
#' z3_fp_sort(ctx, "double") == D
#' parts <- z3_fp_numeral_parts(z3_fp_val(ctx, 1.5, D), numeric = TRUE)
#' parts[c("sign", "exponent_value", "fraction")]
#' z3_value(z3_rounding(ctx, "roundTowardZero"))
#' x <- z3_fp(ctx, "x", D)
#' s <- z3_solver(ctx)
#' # 0.1 + 0.2 != 0.3 in double precision, as in R
#' z3_add(s, x == z3_fp_val(ctx, 0.1, D) + z3_fp_val(ctx, 0.2, D))
#' z3_check(s)
#' z3_value(z3_eval(z3_model(s), x)) == 0.1 + 0.2
#' # A number that is its own successor: Inf
#' s <- z3_solver(ctx)
#' z3_add(s, x + 1 == x, !z3_fp_is_nan(x), x > 0)
#' z3_check(s)
#' z3_value(z3_eval(z3_model(s), x))
#' @name z3_fp
NULL

#' @rdname z3_fp
#' @export
z3_fp_sort <- function(ctx, ebits, sbits) {
  if (missing(sbits)) {
    name <- if (is.numeric(ebits)) as.character(ebits) else ebits
    formats <- c("half", "single", "double", "quadruple", "16", "32", "64", "128")
    if (!is.character(name) || length(name) != 1L || !name %in% formats) {
      stop("without sbits, ebits must name a format: \"half\", \"single\", \"double\", ",
           "\"quadruple\", or a width 16, 32, 64 or 128", call. = FALSE)
    }
    return(cpp_fp_sort_named(ctx, name))
  }
  cpp_fp_sort(ctx, as_count(ebits, "ebits"), as_count(sbits, "sbits"))
}

#' @rdname z3_fp
#' @export
z3_fp16_sort <- function(ctx) cpp_fp_sort_named(ctx, "16")

#' @rdname z3_fp
#' @export
z3_fp32_sort <- function(ctx) cpp_fp_sort_named(ctx, "32")

#' @rdname z3_fp
#' @export
z3_fp64_sort <- function(ctx) cpp_fp_sort_named(ctx, "64")

#' @rdname z3_fp
#' @export
z3_fp128_sort <- function(ctx) cpp_fp_sort_named(ctx, "128")

#' @rdname z3_fp
#' @export
z3_rounding_sort <- function(ctx) cpp_rm_sort(ctx)

#' @rdname z3_fp
#' @export
z3_fp_sort_info <- function(x) cpp_fp_sort_info(x)

#' @rdname z3_fp
#' @export
z3_fp <- function(ctx, name, sort) cpp_mk_const(ctx, name, sort)

#' @rdname z3_fp
#' @export
z3_fp_val <- function(ctx, x, sort, rounding = NULL) {
  if (!is.numeric(x) || length(x) != 1L) stop("x must be a single number", call. = FALSE)
  if (is.character(rounding)) rounding <- z3_rounding(ctx, rounding)
  cpp_fp_from_double(ctx, as.double(x), sort, rounding)
}

#' @rdname z3_fp
#' @export
z3_rounding <- function(ctx, mode = c("RNE", "RNA", "RTP", "RTN", "RTZ", "roundNearestTiesToEven",
                                       "roundNearestTiesToAway", "roundTowardPositive",
                                       "roundTowardNegative", "roundTowardZero")) {
  cpp_rounding_mode(ctx, match.arg(mode))
}

#' @rdname z3_fp
#' @export
z3_fp_nan <- function(sort) cpp_fp_special("nan", sort, FALSE)

#' @rdname z3_fp
#' @export
z3_fp_inf <- function(sort, negative = FALSE) cpp_fp_special("inf", sort, isTRUE(negative))

#' @rdname z3_fp
#' @export
z3_fp_zero <- function(sort, negative = FALSE) cpp_fp_special("zero", sort, isTRUE(negative))

#' @rdname z3_fp
#' @export
z3_fp_from_bits <- function(sign, exponent, significand) cpp_fp_from_bits(sign, exponent, significand)

fp_op <- function(op, ..., sort = NULL, size = 0L) cpp_fp_op(op, list(...), sort, size)

#' @rdname z3_fp
#' @export
z3_fp_add <- function(rounding, x, y) fp_op("add", rounding, x, y)

#' @rdname z3_fp
#' @export
z3_fp_sub <- function(rounding, x, y) fp_op("sub", rounding, x, y)

#' @rdname z3_fp
#' @export
z3_fp_mul <- function(rounding, x, y) fp_op("mul", rounding, x, y)

#' @rdname z3_fp
#' @export
z3_fp_div <- function(rounding, x, y) fp_op("div", rounding, x, y)

#' @rdname z3_fp
#' @export
z3_fp_fma <- function(rounding, x, y, z) fp_op("fma", rounding, x, y, z)

#' @rdname z3_fp
#' @export
z3_fp_sqrt <- function(rounding, x) fp_op("sqrt", rounding, x)

#' @rdname z3_fp
#' @export
z3_fp_round_to_integral <- function(rounding, x) fp_op("round_to_integral", rounding, x)

#' @rdname z3_fp
#' @export
z3_fp_rem <- function(x, y) fp_op("rem", x, y)

#' @rdname z3_fp
#' @export
z3_fp_min <- function(x, y) fp_op("min", x, y)

#' @rdname z3_fp
#' @export
z3_fp_max <- function(x, y) fp_op("max", x, y)

#' @rdname z3_fp
#' @export
z3_fp_abs <- function(x) fp_op("abs", x)

#' @rdname z3_fp
#' @export
z3_fp_neg <- function(x) fp_op("neg", x)

#' @rdname z3_fp
#' @export
z3_fp_eq <- function(x, y) fp_op("eq", x, y)

#' @rdname z3_fp
#' @export
z3_fp_lt <- function(x, y) fp_op("lt", x, y)

#' @rdname z3_fp
#' @export
z3_fp_leq <- function(x, y) fp_op("leq", x, y)

#' @rdname z3_fp
#' @export
z3_fp_gt <- function(x, y) fp_op("gt", x, y)

#' @rdname z3_fp
#' @export
z3_fp_geq <- function(x, y) fp_op("geq", x, y)

#' @rdname z3_fp
#' @export
z3_fp_is_nan <- function(x) fp_op("is_nan", x)

#' @rdname z3_fp
#' @export
z3_fp_is_infinite <- function(x) fp_op("is_infinite", x)

#' @rdname z3_fp
#' @export
z3_fp_is_zero <- function(x) fp_op("is_zero", x)

#' @rdname z3_fp
#' @export
z3_fp_is_normal <- function(x) fp_op("is_normal", x)

#' @rdname z3_fp
#' @export
z3_fp_is_subnormal <- function(x) fp_op("is_subnormal", x)

#' @rdname z3_fp
#' @export
z3_fp_is_negative <- function(x) fp_op("is_negative", x)

#' @rdname z3_fp
#' @export
z3_fp_is_positive <- function(x) fp_op("is_positive", x)

#' @rdname z3_fp
#' @export
z3_fp_to_real <- function(x) fp_op("to_real", x)

#' @rdname z3_fp
#' @export
z3_fp_to_ieee_bv <- function(x) fp_op("to_ieee_bv", x)

#' @rdname z3_fp
#' @export
z3_fp_to_ubv <- function(rounding, x, size) fp_op("to_ubv", rounding, x, size = as_count(size, "size", TRUE))

#' @rdname z3_fp
#' @export
z3_fp_to_sbv <- function(rounding, x, size) fp_op("to_sbv", rounding, x, size = as_count(size, "size", TRUE))

#' @rdname z3_fp
#' @export
z3_fp_to_fp <- function(rounding, x, sort) fp_op("to_fp", rounding, x, sort = sort)

#' @rdname z3_fp
#' @export
z3_fp_from_bv <- function(bv, sort) fp_op("from_ieee_bv", bv, sort = sort)

#' @rdname z3_fp
#' @export
z3_fp_from_signed <- function(rounding, bv, sort) fp_op("from_signed", rounding, bv, sort = sort)

#' @rdname z3_fp
#' @export
z3_fp_from_unsigned <- function(rounding, bv, sort) fp_op("from_unsigned", rounding, bv, sort = sort)

#' @rdname z3_fp
#' @export
z3_fp_from_real_exp <- function(rounding, exponent, significand, sort) {
  fp_op("from_int_real", rounding, exponent, significand, sort = sort)
}

#' @rdname z3_fp
#' @export
z3_fp_numeral_parts <- function(x, biased = FALSE, numeric = FALSE) {
  cpp_fp_numeral_parts(x, isTRUE(biased), isTRUE(numeric))
}

#' @rdname z3_fp
#' @param exponent_value Unbiased exponent, a whole number or string.
#' @param fraction The fraction bits of the significand as an integer (without
#'   the hidden bit), a whole number or string.
#' @export
z3_fp_from_parts <- function(sort, negative, exponent_value, fraction) {
  cpp_fp_from_parts(sort, isTRUE(negative), whole_string(exponent_value, "exponent_value"),
                    whole_string(fraction, "fraction"))
}

# A whole number as a decimal string: numbers must be exact doubles (at most
# 2^53 in magnitude); larger values must be given as strings.
whole_string <- function(x, what) {
  if (is.character(x) && length(x) == 1L && !is.na(x)) {
    x <- trimws(x)
    if (!grepl("^[+-]?[0-9]+$", x)) stop(sprintf("%s must be a whole number", what), call. = FALSE)
    return(sub("^[+]", "", x))
  }
  if (!is.numeric(x) || length(x) != 1L || !is.finite(x) || x != floor(x)) {
    stop(sprintf("%s must be a single whole number or a string of digits", what), call. = FALSE)
  }
  if (abs(x) > 2^53) {
    stop(sprintf("%s is beyond 2^53, where doubles are not exact; give it as a string", what),
         call. = FALSE)
  }
  format(x, scientific = FALSE, trim = TRUE)
}
