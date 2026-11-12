#' Bit-vector operations
#'
#' Bit-vectors are fixed-width machine integers. Arithmetic wraps around
#' modulo 2^width, and the same bits can be read as unsigned or as two's
#' complement signed numbers, so operations whose result depends on that
#' reading come in unsigned (`u`) and signed (`s`) versions. Both operands
#' of a binary operation must have the same width; R numbers are converted
#' to that width and must fit in it as an unsigned or signed value.
#'
#' `+`, `-`, `*`, `==`, `!=`, `&`, `|` and `!` work directly on
#' bit-vectors; see [z3_operators].
#'
#' * Comparisons: `z3_bvult()`, `z3_bvule()`, `z3_bvugt()`, `z3_bvuge()`
#'   (unsigned) and `z3_bvslt()`, `z3_bvsle()`, `z3_bvsgt()`, `z3_bvsge()`
#'   (signed).
#' * Division: `z3_bvudiv()`, `z3_bvurem()` (unsigned); `z3_bvsdiv()`,
#'   `z3_bvsrem()` (signed, remainder has the sign of the dividend) and
#'   `z3_bvsmod()` (signed, remainder has the sign of the divisor).
#'   Division by zero follows SMT-LIB: `bvudiv(x, 0)` is all ones and
#'   `bvurem(x, 0)` is `x`.
#' * Bitwise: `z3_bvand()`, `z3_bvor()`, `z3_bvxor()`, `z3_bvnot()`,
#'   `z3_bvnand()`, `z3_bvnor()`, `z3_bvxnor()`.
#' * Shifts: `z3_bvshl()` (left), `z3_bvlshr()` (logical right, fills with
#'   zeros), `z3_bvashr()` (arithmetic right, copies the sign bit).
#'   Rotations: `z3_rotate_left()`, `z3_rotate_right()` by a constant.
#' * Width: `z3_concat()` joins bit-vectors (first argument in the high
#'   bits), `z3_extract(x, high, low)` takes bits `high` down to `low`,
#'   `z3_zero_ext()` and `z3_sign_ext()` add `n` bits, `z3_repeat()`
#'   repeats `x`.
#' * Conversions: `z3_bv2int()` reads the bits as an unbounded Int,
#'   unsigned unless `signed = TRUE`; `z3_int2bv()` takes an Int modulo
#'   2^width.
#' * Overflow predicates: `z3_bvadd_no_overflow()` and friends are `TRUE`
#'   when the operation does not overflow in the given signedness.
#'
#' @param x,y Bit-vector expressions (or R whole numbers for one operand).
#' @param high,low Bit positions, `width > high >= low >= 0`.
#' @param n Number of bits or repetitions.
#' @param signed Signed (two's complement) interpretation.
#' @param width Bit-vector width.
#' @param ... Bit-vector or String expressions for `z3_concat()`.
#' @return A `z3_expr`.
#' @examples
#' ctx <- z3_context()
#' x <- z3_bv(ctx, "x", 8)
#' s <- z3_solver(ctx)
#' # 200 is negative as a signed 8-bit number
#' z3_add(s, x == 200, z3_bvslt(x, 0), z3_bvugt(x, 100))
#' z3_check(s)
#' # Overflow: x + 1 wraps around to 0
#' z3_simplify(z3_bv_val(ctx, 255, 8) + 1)
#' z3_simplify(z3_extract(z3_bv_val(ctx, 0xAB, 8), 7, 4))
#' z3_simplify(z3_bv2int(z3_bv_val(ctx, 255, 8), signed = TRUE))
#' @name z3_bitvectors
NULL

bv_bin <- function(op) function(x, y) cpp_op(op, list(x, y), integer())

#' @rdname z3_bitvectors
#' @export
z3_bvult <- bv_bin("bvult")
#' @rdname z3_bitvectors
#' @export
z3_bvule <- bv_bin("bvule")
#' @rdname z3_bitvectors
#' @export
z3_bvugt <- bv_bin("bvugt")
#' @rdname z3_bitvectors
#' @export
z3_bvuge <- bv_bin("bvuge")
#' @rdname z3_bitvectors
#' @export
z3_bvslt <- bv_bin("bvslt")
#' @rdname z3_bitvectors
#' @export
z3_bvsle <- bv_bin("bvsle")
#' @rdname z3_bitvectors
#' @export
z3_bvsgt <- bv_bin("bvsgt")
#' @rdname z3_bitvectors
#' @export
z3_bvsge <- bv_bin("bvsge")
#' @rdname z3_bitvectors
#' @export
z3_bvudiv <- bv_bin("bvudiv")
#' @rdname z3_bitvectors
#' @export
z3_bvsdiv <- bv_bin("bvsdiv")
#' @rdname z3_bitvectors
#' @export
z3_bvurem <- bv_bin("bvurem")
#' @rdname z3_bitvectors
#' @export
z3_bvsrem <- bv_bin("bvsrem")
#' @rdname z3_bitvectors
#' @export
z3_bvsmod <- bv_bin("bvsmod")
#' @rdname z3_bitvectors
#' @export
z3_bvand <- bv_bin("bvand")
#' @rdname z3_bitvectors
#' @export
z3_bvor <- bv_bin("bvor")
#' @rdname z3_bitvectors
#' @export
z3_bvxor <- bv_bin("bvxor")
#' @rdname z3_bitvectors
#' @export
z3_bvnand <- bv_bin("bvnand")
#' @rdname z3_bitvectors
#' @export
z3_bvnor <- bv_bin("bvnor")
#' @rdname z3_bitvectors
#' @export
z3_bvxnor <- bv_bin("bvxnor")
#' @rdname z3_bitvectors
#' @export
z3_bvshl <- bv_bin("bvshl")
#' @rdname z3_bitvectors
#' @export
z3_bvlshr <- bv_bin("bvlshr")
#' @rdname z3_bitvectors
#' @export
z3_bvashr <- bv_bin("bvashr")

#' @rdname z3_bitvectors
#' @export
z3_bvnot <- function(x) cpp_op("bvnot", list(x), integer())

#' @rdname z3_bitvectors
#' @export
z3_bvneg <- function(x) cpp_op("bvneg", list(x), integer())

#' @rdname z3_bitvectors
#' @export
z3_concat <- function(...) cpp_op("concat", flatten_args(...), integer())

#' @rdname z3_bitvectors
#' @export
z3_extract <- function(x, high, low) {
  cpp_op("extract", list(x), c(as_count(high, "high"), as_count(low, "low")))
}

#' @rdname z3_bitvectors
#' @export
z3_zero_ext <- function(x, n) cpp_op("zero_ext", list(x), as_count(n, "n"))

#' @rdname z3_bitvectors
#' @export
z3_sign_ext <- function(x, n) cpp_op("sign_ext", list(x), as_count(n, "n"))

#' @rdname z3_bitvectors
#' @export
z3_rotate_left <- function(x, n) cpp_op("rotate_left", list(x), as_count(n, "n"))

#' @rdname z3_bitvectors
#' @export
z3_rotate_right <- function(x, n) cpp_op("rotate_right", list(x), as_count(n, "n"))

#' @rdname z3_bitvectors
#' @export
z3_repeat <- function(x, n) cpp_op("repeat", list(x), as_count(n, "n", positive = TRUE))

#' @rdname z3_bitvectors
#' @export
z3_bv2int <- function(x, signed = FALSE) {
  cpp_op("bv2int", list(x), as.integer(isTRUE(signed)))
}

#' @rdname z3_bitvectors
#' @export
z3_int2bv <- function(x, width) {
  cpp_op("int2bv", list(x), as_count(width, "width", positive = TRUE))
}

#' @rdname z3_bitvectors
#' @export
z3_bvadd_no_overflow <- function(x, y, signed) {
  cpp_op("bvadd_no_overflow", list(x, y), as.integer(isTRUE(signed)))
}

#' @rdname z3_bitvectors
#' @export
z3_bvmul_no_overflow <- function(x, y, signed) {
  cpp_op("bvmul_no_overflow", list(x, y), as.integer(isTRUE(signed)))
}

#' @rdname z3_bitvectors
#' @export
z3_bvsub_no_underflow <- function(x, y, signed) {
  cpp_op("bvsub_no_underflow", list(x, y), as.integer(isTRUE(signed)))
}

#' @rdname z3_bitvectors
#' @export
z3_bvadd_no_underflow <- bv_bin("bvadd_no_underflow")

#' @rdname z3_bitvectors
#' @export
z3_bvsub_no_overflow <- bv_bin("bvsub_no_overflow")

#' @rdname z3_bitvectors
#' @export
z3_bvmul_no_underflow <- bv_bin("bvmul_no_underflow")

#' @rdname z3_bitvectors
#' @export
z3_bvsdiv_no_overflow <- bv_bin("bvsdiv_no_overflow")

#' @rdname z3_bitvectors
#' @export
z3_bv_width <- function(x) cpp_bv_width(x)

#' @rdname z3_bitvectors
#' @export
z3_bvneg_no_overflow <- function(x) cpp_misc_op("bvneg_no_overflow", list(x), integer())

#' @rdname z3_bitvectors
#' @param i Bit index for `z3_bit2bool()` (0 is the least significant bit).
#' @export
z3_bit2bool <- function(x, i) cpp_misc_op("bit2bool", list(x), as_count(i, "i"))

#' @rdname z3_bitvectors
#' @export
z3_bvredand <- function(x) cpp_op("bvredand", list(x), integer())

#' @rdname z3_bitvectors
#' @export
z3_bvredor <- function(x) cpp_op("bvredor", list(x), integer())

#' @rdname z3_bitvectors
#' @export
z3_rotate_left_by <- bv_bin("rotate_left_by")

#' @rdname z3_bitvectors
#' @export
z3_rotate_right_by <- bv_bin("rotate_right_by")

#' @rdname z3_bitvectors
#' @export
z3_ubv_to_str <- function(x) cpp_seq_op("ubv_to_str", list(x), integer())

#' @rdname z3_bitvectors
#' @export
z3_sbv_to_str <- function(x) cpp_seq_op("sbv_to_str", list(x), integer())

#' @rdname z3_bitvectors
#' @param ctx A [z3_context()].
#' @param bits A logical vector, least significant bit first.
#' @export
z3_bv_from_bits <- function(ctx, bits) {
  if (!is.logical(bits)) stop("bits must be a logical vector", call. = FALSE)
  cpp_bv_from_bits(ctx, bits)
}
