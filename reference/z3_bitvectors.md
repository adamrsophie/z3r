# Bit-vector operations

Bit-vectors are fixed-width machine integers. Arithmetic wraps around
modulo 2^width, and the same bits can be read as unsigned or as two's
complement signed numbers, so operations whose result depends on that
reading come in unsigned (`u`) and signed (`s`) versions. Both operands
of a binary operation must have the same width; R numbers are converted
to that width and must fit in it as an unsigned or signed value.

## Usage

``` r
z3_bvult(x, y)

z3_bvule(x, y)

z3_bvugt(x, y)

z3_bvuge(x, y)

z3_bvslt(x, y)

z3_bvsle(x, y)

z3_bvsgt(x, y)

z3_bvsge(x, y)

z3_bvudiv(x, y)

z3_bvsdiv(x, y)

z3_bvurem(x, y)

z3_bvsrem(x, y)

z3_bvsmod(x, y)

z3_bvand(x, y)

z3_bvor(x, y)

z3_bvxor(x, y)

z3_bvnand(x, y)

z3_bvnor(x, y)

z3_bvxnor(x, y)

z3_bvshl(x, y)

z3_bvlshr(x, y)

z3_bvashr(x, y)

z3_bvnot(x)

z3_bvneg(x)

z3_concat(...)

z3_extract(x, high, low)

z3_zero_ext(x, n)

z3_sign_ext(x, n)

z3_rotate_left(x, n)

z3_rotate_right(x, n)

z3_repeat(x, n)

z3_bv2int(x, signed = FALSE)

z3_int2bv(x, width)

z3_bvadd_no_overflow(x, y, signed)

z3_bvmul_no_overflow(x, y, signed)

z3_bvsub_no_underflow(x, y, signed)

z3_bvadd_no_underflow(x, y)

z3_bvsub_no_overflow(x, y)

z3_bvmul_no_underflow(x, y)

z3_bvsdiv_no_overflow(x, y)

z3_bv_width(x)

z3_bvneg_no_overflow(x)

z3_bit2bool(x, i)

z3_bvredand(x)

z3_bvredor(x)

z3_rotate_left_by(x, y)

z3_rotate_right_by(x, y)

z3_ubv_to_str(x)

z3_sbv_to_str(x)

z3_bv_from_bits(ctx, bits)
```

## Arguments

- x, y:

  Bit-vector expressions (or R whole numbers for one operand).

- ...:

  Bit-vector or String expressions for `z3_concat()`.

- high, low:

  Bit positions, `width > high >= low >= 0`.

- n:

  Number of bits or repetitions.

- signed:

  Signed (two's complement) interpretation.

- width:

  Bit-vector width.

- i:

  Bit index for `z3_bit2bool()` (0 is the least significant bit).

- ctx:

  A
  [`z3_context()`](https://adamrsophie.github.io/z3r/reference/z3_context.md).

- bits:

  A logical vector, least significant bit first.

## Value

A `z3_expr`.

## Details

`+`, `-`, `*`, `==`, `!=`, `&`, `|` and `!` work directly on
bit-vectors; see
[z3_operators](https://adamrsophie.github.io/z3r/reference/z3_operators.md).

- Comparisons: `z3_bvult()`, `z3_bvule()`, `z3_bvugt()`, `z3_bvuge()`
  (unsigned) and `z3_bvslt()`, `z3_bvsle()`, `z3_bvsgt()`, `z3_bvsge()`
  (signed).

- Division: `z3_bvudiv()`, `z3_bvurem()` (unsigned); `z3_bvsdiv()`,
  `z3_bvsrem()` (signed, remainder has the sign of the dividend) and
  `z3_bvsmod()` (signed, remainder has the sign of the divisor).
  Division by zero follows SMT-LIB: `bvudiv(x, 0)` is all ones and
  `bvurem(x, 0)` is `x`.

- Bitwise: `z3_bvand()`, `z3_bvor()`, `z3_bvxor()`, `z3_bvnot()`,
  `z3_bvnand()`, `z3_bvnor()`, `z3_bvxnor()`.

- Shifts: `z3_bvshl()` (left), `z3_bvlshr()` (logical right, fills with
  zeros), `z3_bvashr()` (arithmetic right, copies the sign bit).
  Rotations: `z3_rotate_left()`, `z3_rotate_right()` by a constant.

- Width: `z3_concat()` joins bit-vectors (first argument in the high
  bits), `z3_extract(x, high, low)` takes bits `high` down to `low`,
  `z3_zero_ext()` and `z3_sign_ext()` add `n` bits, `z3_repeat()`
  repeats `x`.

- Conversions: `z3_bv2int()` reads the bits as an unbounded Int,
  unsigned unless `signed = TRUE`; `z3_int2bv()` takes an Int modulo
  2^width.

- Overflow predicates: `z3_bvadd_no_overflow()` and friends are `TRUE`
  when the operation does not overflow in the given signedness.

## Examples

``` r
ctx <- z3_context()
x <- z3_bv(ctx, "x", 8)
s <- z3_solver(ctx)
# 200 is negative as a signed 8-bit number
z3_add(s, x == 200, z3_bvslt(x, 0), z3_bvugt(x, 100))
z3_check(s)
#> [1] "sat"
# Overflow: x + 1 wraps around to 0
z3_simplify(z3_bv_val(ctx, 255, 8) + 1)
#> <z3_expr (_ BitVec 8)> #x00
z3_simplify(z3_extract(z3_bv_val(ctx, 0xAB, 8), 7, 4))
#> <z3_expr (_ BitVec 4)> #xa
z3_simplify(z3_bv2int(z3_bv_val(ctx, 255, 8), signed = TRUE))
#> <z3_expr Int> (- 1)
```
