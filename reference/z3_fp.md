# IEEE 754 floating point

Z3's floating point theory models IEEE 754 binary formats exactly,
including rounding, signed zeros, infinities and NaN. A format has
`ebits` exponent bits and `sbits` significand bits (counting the hidden
bit): `z3_fp_sort(ctx, 11, 53)` is double precision, the format of R's
numbers. The standard formats also have names:
`z3_fp_sort(ctx, "half")`, `"single"`, `"double"` and `"quadruple"`, or
by width `z3_fp_sort(ctx, 16)`, `32`, `64` and `128`, the same as
`z3_fp16_sort()`, `z3_fp32_sort()`, `z3_fp64_sort()` and
`z3_fp128_sort()`.

## Usage

``` r
z3_fp_sort(ctx, ebits, sbits)

z3_fp16_sort(ctx)

z3_fp32_sort(ctx)

z3_fp64_sort(ctx)

z3_fp128_sort(ctx)

z3_rounding_sort(ctx)

z3_fp_sort_info(x)

z3_fp(ctx, name, sort)

z3_fp_val(ctx, x, sort, rounding = NULL)

z3_rounding(
  ctx,
  mode = c("RNE", "RNA", "RTP", "RTN", "RTZ", "roundNearestTiesToEven",
    "roundNearestTiesToAway", "roundTowardPositive", "roundTowardNegative",
    "roundTowardZero")
)

z3_fp_nan(sort)

z3_fp_inf(sort, negative = FALSE)

z3_fp_zero(sort, negative = FALSE)

z3_fp_from_bits(sign, exponent, significand)

z3_fp_add(rounding, x, y)

z3_fp_sub(rounding, x, y)

z3_fp_mul(rounding, x, y)

z3_fp_div(rounding, x, y)

z3_fp_fma(rounding, x, y, z)

z3_fp_sqrt(rounding, x)

z3_fp_round_to_integral(rounding, x)

z3_fp_rem(x, y)

z3_fp_min(x, y)

z3_fp_max(x, y)

z3_fp_abs(x)

z3_fp_neg(x)

z3_fp_eq(x, y)

z3_fp_lt(x, y)

z3_fp_leq(x, y)

z3_fp_gt(x, y)

z3_fp_geq(x, y)

z3_fp_is_nan(x)

z3_fp_is_infinite(x)

z3_fp_is_zero(x)

z3_fp_is_normal(x)

z3_fp_is_subnormal(x)

z3_fp_is_negative(x)

z3_fp_is_positive(x)

z3_fp_to_real(x)

z3_fp_to_ieee_bv(x)

z3_fp_to_ubv(rounding, x, size)

z3_fp_to_sbv(rounding, x, size)

z3_fp_to_fp(rounding, x, sort)

z3_fp_from_bv(bv, sort)

z3_fp_from_signed(rounding, bv, sort)

z3_fp_from_unsigned(rounding, bv, sort)

z3_fp_from_real_exp(rounding, exponent, significand, sort)

z3_fp_numeral_parts(x, biased = FALSE, numeric = FALSE)

z3_fp_from_parts(sort, negative, exponent_value, fraction)
```

## Arguments

- ctx:

  A
  [`z3_context()`](https://adamrsophie.github.io/z3r/reference/z3_context.md).

- ebits, sbits:

  Exponent and significand widths. With `sbits` omitted, `ebits` names a
  standard format: `"half"`, `"single"`, `"double"`, `"quadruple"`, or a
  width 16, 32, 64 or 128.

- x, y, z:

  Floating point expressions or R numbers (any value for
  `z3_fp_to_fp()`).

- name:

  Name of a constant.

- sort:

  A floating point sort.

- rounding:

  A rounding mode from `z3_rounding()`.

- mode:

  Rounding mode name, short (`"RNE"`) or SMT-LIB
  (`"roundNearestTiesToEven"`).

- negative:

  Sign of an infinity or zero.

- size:

  Width of the resulting bit-vector.

- bv, sign, exponent, significand:

  Bit-vector expressions; for `z3_fp_from_real_exp()`, an Int exponent
  and a Real significand.

- biased:

  For `z3_fp_numeral_parts()`: report the biased exponent.

- numeric:

  For `z3_fp_numeral_parts()`: also give the sign, exponent and fraction
  as numbers.

- exponent_value:

  Unbiased exponent, a whole number or string.

- fraction:

  The fraction bits of the significand as an integer (without the hidden
  bit), a whole number or string.

## Value

A `z3_expr` or `z3_sort`; `z3_fp_sort_info()` returns the two widths;
`z3_fp_numeral_parts()` a list: `class` (`"normal"`, `"subnormal"`,
`"zero"`, `"infinite"` or `"nan"`) and `negative`; `significand` and
`exponent` as decimal strings for finite values; and for every value but
NaN the fields as bit-vectors (`sign_bv`, `exponent_bv`,
`significand_bv`). With `numeric = TRUE`, values other than NaN also
have `sign` (0 or 1), `exponent_value` and `fraction` (the fraction bits
without the hidden bit, as an integer), each a double while exact (at
most 2^53 in magnitude) and a decimal string beyond.

## Details

**Values.** `z3_fp(ctx, name, sort)` makes a constant. R numbers
combined with a floating point expression are converted to its format
only when the format represents them exactly; `NaN`, `Inf` and `-0` are
kept. `z3_fp_val(ctx, x, sort, rounding)` rounds an R number into a
format explicitly. `z3_fp_nan()`, `z3_fp_inf()` and `z3_fp_zero()` give
the special values.
[`z3_value()`](https://adamrsophie.github.io/z3r/reference/z3_value.md)
converts numerals back to R doubles exactly.

**Operators.** `+`, `-`, `*`, `/` and unary `-` use round-to-nearest,
ties-to-even (`"RNE"`), the rounding of R's own arithmetic; `%%` is the
IEEE remainder. `<`, `<=`, `>`, `>=` are IEEE comparisons (false when
either side is NaN). `==` and `!=` are **structural** equality, as for
every other sort: `NaN == NaN` is true and `0 == -0` is false. Use
`z3_fp_eq()` for IEEE equality, where the opposite holds.

**Explicit rounding.** `z3_rounding(ctx, mode)` gives a rounding mode:
`"RNE"` (nearest, ties to even), `"RNA"` (nearest, ties away from zero),
`"RTP"` (toward +Inf), `"RTN"` (toward -Inf), `"RTZ"` (toward zero), or
by its SMT-LIB name, which
[`z3_value()`](https://adamrsophie.github.io/z3r/reference/z3_value.md)
returns: `"roundNearestTiesToEven"`, `"roundNearestTiesToAway"`,
`"roundTowardPositive"`, `"roundTowardNegative"`, `"roundTowardZero"`.
`z3_fp_add()`, `z3_fp_sub()`, `z3_fp_mul()`, `z3_fp_div()`,
`z3_fp_fma()`, `z3_fp_sqrt()` and `z3_fp_round_to_integral()` take it as
their first argument.

**Other operations.** `z3_fp_rem()`, `z3_fp_min()`, `z3_fp_max()`,
`z3_fp_abs()` (also [`abs()`](https://rdrr.io/r/base/MathFun.html)),
`z3_fp_neg()`; predicates `z3_fp_is_nan()`, `z3_fp_is_infinite()`,
`z3_fp_is_zero()`, `z3_fp_is_normal()`, `z3_fp_is_subnormal()`,
`z3_fp_is_negative()`, `z3_fp_is_positive()`; comparisons `z3_fp_lt()`,
`z3_fp_leq()`, `z3_fp_gt()`, `z3_fp_geq()`, `z3_fp_eq()`.

**Conversions.** `z3_fp_to_fp(rounding, x, sort)` converts another
format, a Real or an Int; `z3_fp_from_bv(bv, sort)` reinterprets IEEE
bits; `z3_fp_from_signed()`/`z3_fp_from_unsigned()` convert a bit-vector
integer; `z3_fp_from_real_exp(rounding, exponent, significand, sort)`
rounds `significand * 2^exponent` from an Int exponent and a Real
significand, which may be symbolic;
`z3_fp_from_bits(sign, exponent, significand)` assembles the fields;
`z3_fp_from_parts()` builds a numeral exactly from its sign, unbiased
exponent and fraction bits; `z3_fp_to_ubv()`/`z3_fp_to_sbv()` round to a
bit-vector integer of width `size`; `z3_fp_to_real()` gives the exact
Real value; `z3_fp_to_ieee_bv()` the bit pattern.
`z3_fp_numeral_parts()` describes a numeral (class, sign, exponent and
significand).

## Examples

``` r
ctx <- z3_context()
D <- z3_fp64_sort(ctx)
z3_fp_sort(ctx, "double") == D
#> [1] TRUE
parts <- z3_fp_numeral_parts(z3_fp_val(ctx, 1.5, D), numeric = TRUE)
parts[c("sign", "exponent_value", "fraction")]
#> $sign
#> [1] 0
#> 
#> $exponent_value
#> [1] 0
#> 
#> $fraction
#> [1] 2.2518e+15
#> 
z3_value(z3_rounding(ctx, "roundTowardZero"))
#> [1] "roundTowardZero"
x <- z3_fp(ctx, "x", D)
s <- z3_solver(ctx)
# 0.1 + 0.2 != 0.3 in double precision, as in R
z3_add(s, x == z3_fp_val(ctx, 0.1, D) + z3_fp_val(ctx, 0.2, D))
z3_check(s)
#> [1] "sat"
z3_value(z3_eval(z3_model(s), x)) == 0.1 + 0.2
#> [1] TRUE
# A number that is its own successor: Inf
s <- z3_solver(ctx)
z3_add(s, x + 1 == x, !z3_fp_is_nan(x), x > 0)
z3_check(s)
#> [1] "sat"
z3_value(z3_eval(z3_model(s), x))
#> [1] Inf
```
