# Real closed fields

Exact computation with real algebraic numbers, the transcendental
constants pi and e, and infinitesimals, independent of any solver.
Values are `z3_rcf` objects with arithmetic (`+ - * /`, `^` with a whole
exponent, unary `-`) and comparison operators.

## Usage

``` r
z3_rcf(ctx, value)

z3_rcf_pi(ctx)

z3_rcf_e(ctx)

z3_rcf_infinitesimal(ctx)

z3_rcf_roots(coeffs)

z3_rcf_decimal(x, digits = 10)

z3_rcf_fraction(x)

z3_rcf_info(x)

# S3 method for class 'z3_rcf'
format(x, compact = FALSE, ...)
```

## Arguments

- ctx:

  A
  [`z3_context()`](https://adamrsophie.github.io/z3r/reference/z3_context.md).

- value:

  An exact rational as a string, or an R whole number.

- coeffs:

  A list of `z3_rcf` coefficients, constant term first.

- x:

  A `z3_rcf`.

- digits:

  Digits after the decimal point.

- compact:

  Use the compact notation for extensions.

- ...:

  Unused.

## Value

A `z3_rcf`, a list of them, a string or a list.

## Details

- `z3_rcf(ctx, "1/3")` makes a rational and `z3_rcf(ctx, 5)` an integer
  (whole numbers within R's integer range use Z3's small integer
  constructor); `z3_rcf_pi()`, `z3_rcf_e()` and `z3_rcf_infinitesimal()`
  the special elements.

- `z3_rcf_roots(coeffs)` finds all real roots of the polynomial
  `coeffs[[1]] + coeffs[[2]] x + ...`, exactly.

- [`format()`](https://rdrr.io/r/base/format.html) prints the number;
  `z3_rcf_decimal(x, digits)` gives decimal digits; `z3_rcf_fraction()`
  the numerator and denominator; `z3_rcf_info()` (Z3 4.12.3) classifies
  the number and, for algebraic numbers, gives the defining polynomial,
  isolating interval and sign conditions.

## Examples

``` r
ctx <- z3_context()
two <- z3_rcf(ctx, 2)
r <- z3_rcf_roots(list(-two, z3_rcf(ctx, 0), z3_rcf(ctx, 1)))  # x^2 - 2
r
#> [[1]]
#> <z3_rcf> root(x^2 + -2, (-oo, 0), {})
#> 
#> [[2]]
#> <z3_rcf> root(x^2 + -2, (0, +oo), {})
#> 
z3_rcf_decimal(r[[2]], 20)
#> [1] "1.41421356237309504880?"
r[[2]]^2 == two
#> [1] TRUE
z3_rcf_pi(ctx) > z3_rcf(ctx, "314159/100000")
#> [1] TRUE
eps <- z3_rcf_infinitesimal(ctx)
eps > 0 & eps < z3_rcf(ctx, "1/1000000")
#> [1] TRUE
```
