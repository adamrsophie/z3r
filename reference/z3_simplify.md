# Simplify an expression

Applies Z3's algebraic simplifier. The result is equivalent to the input
but not necessarily simpler in every case.

## Usage

``` r
z3_simplify(x, ...)
```

## Arguments

- x:

  A `z3_expr`.

- ...:

  Simplifier parameters.

## Value

A `z3_expr`.

## Details

Named parameters select options of Z3's simplifier, listed by
[`z3_simplify_help()`](https://adamrsophie.github.io/z3r/reference/z3_simplify_help.md).

## Examples

``` r
ctx <- z3_context()
x <- z3_int(ctx, "x")
z3_simplify(x + 2 * x - 3 + 3)
#> <z3_expr Int> (* 3 x)
z3_simplify(z3_int_val(ctx, 7) %/% -2)
#> <z3_expr Int> (- 3)
z3_simplify((x + 1) * (x + 1), som = TRUE)
#> <z3_expr Int> (+ 1 (* 2 x) (* x x))
```
