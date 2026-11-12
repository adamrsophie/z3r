# Decimal digits of a number

Returns a decimal string for an exact number or an algebraic number,
truncated (not rounded) to `digits` digits after the decimal point. Z3
marks results that are not exact with a trailing `?`: `2/3` with two
digits is `"0.66?"`.

## Usage

``` r
z3_decimal(x, digits = 10)

z3_binary_string(x)
```

## Arguments

- x:

  A numeric `z3_expr` value.

- digits:

  Number of digits after the decimal point.

## Value

A character string.

## Details

`z3_binary_string()` returns the binary digits of an integer numeral.

## Examples

``` r
ctx <- z3_context()
z3_decimal(z3_real_val(ctx, "1/3"), 5)
#> [1] "0.33333?"
```
