# Convert a Z3 value to an R value

Converts a value expression, typically the result of
[`z3_eval()`](https://adamrsophie.github.io/z3r/reference/z3_eval.md),
to the closest R type that represents it exactly.

## Usage

``` r
z3_value(x, exact = TRUE, signed = FALSE)
```

## Arguments

- x:

  A `z3_expr` that is a value.

- exact:

  If `FALSE`, numbers that are not exactly representable as doubles
  (including algebraic numbers) are rounded to a nearby double.

- signed:

  For bit-vectors, interpret the bits as a two's complement signed
  number.

## Value

An R value; see the table.

## Details

|  |  |
|----|----|
| Z3 value | R result |
| Bool | `TRUE` or `FALSE` |
| Int, Real, bit-vector | a double when exactly representable; otherwise a [`gmp::bigz`](https://rdrr.io/pkg/gmp/man/biginteger.html) or [`gmp::bigq`](https://rdrr.io/pkg/gmp/man/bigrational.html) if gmp is installed, or else a character string holding the exact value (`"1/3"`) |
| algebraic number | error when `exact = TRUE`; a nearby double when `exact = FALSE` |
| String, character | a UTF-8 character string (an error for a string containing U+0000, which R cannot hold) |
| finite field element | its representative in `[0, p)`, as for Int |
| Complex | an R complex number when both parts are exactly representable doubles (rounded when `exact = FALSE`) |
| floating point | a double (`NaN`, `Inf`, `-0` included); formats wider than double precision only when the value fits exactly, or rounded when `exact = FALSE` |
| rounding mode | its name, such as `"roundNearestTiesToEven"` |

A double is "exactly representable" when its numerator has absolute
value at most 2^53 and its denominator is a power of two up to 2^1023
(subnormal values, with larger denominators, give a `bigq`); for example
`1/4` converts to `0.25`, but `1/3` does not convert to `0.3333...`
unless `exact = FALSE`.

Symbolic expressions are not values: evaluate them in a model first.

## Examples

``` r
ctx <- z3_context()
z3_value(z3_int_val(ctx, 42))
#> [1] 42
z3_value(z3_real_val(ctx, "1/4"))
#> [1] 0.25
z3_value(z3_real_val(ctx, "1/3"), exact = FALSE)
#> [1] 0.3333333
z3_value(z3_bv_val(ctx, 255, 8), signed = TRUE)
#> [1] -1
```
