# Divisibility

`z3_divides(a, b)` states that the integer `a` divides `b`. `a` must be
a positive integer numeral (Z3's divisibility predicate has a fixed
divisor). Before Z3 4.12.0 the result is `b mod a == 0`, because those
releases do not build the predicate correctly.

## Usage

``` r
z3_divides(a, b)
```

## Arguments

- a, b:

  Int expressions or R whole numbers.

## Value

A Boolean `z3_expr`.

## Examples

``` r
ctx <- z3_context()
x <- z3_int(ctx, "x")
s <- z3_solver(ctx)
z3_add(s, z3_divides(7, x), x > 50, x < 60)
z3_check(s)
#> [1] "sat"
z3_eval(z3_model(s), x)
#> <z3_expr Int> 56
```
