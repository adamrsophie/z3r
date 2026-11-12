# Exact numeric, Boolean and string values

Build Z3 values without going through floating point.

## Usage

``` r
z3_int_val(ctx, value)

z3_real_val(ctx, value, den = NULL)

z3_bool_val(ctx, value)

z3_bv_val(ctx, value, width)

z3_string_val(ctx, value, escaped = FALSE)

z3_val(ctx, value, sort)
```

## Arguments

- ctx:

  A
  [`z3_context()`](https://adamrsophie.github.io/z3r/reference/z3_context.md).

- value:

  The value; see Details.

- den:

  For `z3_real_val()`, an optional denominator: a whole number, not
  zero.

- width:

  Bit-vector width in bits.

- escaped:

  For `z3_string_val()`, read `value` as an SMT-LIB string literal with
  `\u{...}` escapes.

- sort:

  Target sort for `z3_val()`.

## Value

A `z3_expr`.

## Details

`z3_int_val()` accepts a string of decimal digits with an optional sign,
a whole number with absolute value at most 2^53, an R integer, or a
[`gmp::bigz`](https://rdrr.io/pkg/gmp/man/biginteger.html).
`z3_real_val()` accepts an exact decimal (`"0.125"`, `"1.5e-3"`), a
fraction (`"1/3"`, `"-22/7"`), an R number, or a
[`gmp::bigq`](https://rdrr.io/pkg/gmp/man/bigrational.html). Strings are
parsed exactly, so `z3_real_val(ctx, "0.1")` is the rational 1/10. With
`den`, `z3_real_val(ctx, value, den)` is the fraction `value / den` of
two whole numbers (R integers, doubles of absolute value at most 2^53,
or
[`bit64::integer64`](https://bit64.r-lib.org/reference/bit64-package.html)
values), built without going through strings.

`z3_string_val()` converts an R string exactly, character by character.
With `escaped = TRUE` it reads the string as an SMT-LIB string literal
instead, so that `\u{41}` (written `"\\u{41}"` in R) is the character
`A`; escapes beyond U+2FFFF or without a closing brace are kept as text.
A raw vector gives one character per byte, with the code points 0 to 255
(so `as.raw(0)` is the character U+0000, which R strings cannot hold);
bytes are not decoded as UTF-8. See
[`z3_string_escaped()`](https://adamrsophie.github.io/z3r/reference/z3_string_escaped.md)
and
[`z3_string_bytes()`](https://adamrsophie.github.io/z3r/reference/z3_string_escaped.md)
for the reverse conversions.

## Converting R doubles

When an R number is combined with a Z3 expression, as in `x + 0.1`, it
is converted as follows:

- Whole numbers become exact integers. Whole numbers beyond 2^53 are
  rejected, because R may already have rounded them; pass them as
  strings or [`gmp::bigz`](https://rdrr.io/pkg/gmp/man/biginteger.html)
  instead.

- Other finite doubles become the exact rational value of their shortest
  decimal representation that converts back to the same double, so `0.1`
  becomes 1/10 rather than the binary approximation
  3602879701896397/36028797018963968. This matches what R prints.

- `NA`, `NaN` and `Inf` are rejected.

Set `options(z3r.doubles = "error")` to reject non-integer doubles and
require exact strings everywhere.

## Examples

``` r
ctx <- z3_context()
z3_int_val(ctx, "123456789012345678901234567890")
#> <z3_expr Int> 123456789012345678901234567890
z3_real_val(ctx, "1/3")
#> <z3_expr Real> (/ 1.0 3.0)
z3_real_val(ctx, 1, 3)
#> <z3_expr Real> (/ 1.0 3.0)
z3_real_val(ctx, "0.125")
#> <z3_expr Real> (/ 1.0 8.0)
z3_bv_val(ctx, -1, 8)
#> <z3_expr (_ BitVec 8)> #xff
z3_string_val(ctx, "hello")
#> <z3_expr String> "hello"
z3_string_val(ctx, "tab\\u{9}", escaped = TRUE)
#> <z3_expr String> "tab\u{9}"
z3_string_val(ctx, as.raw(c(0x68, 0x69)))
#> <z3_expr String> "hi"
z3_val(ctx, 2, z3_real_sort(ctx))
#> <z3_expr Real> 2.0
```
