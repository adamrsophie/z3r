# Assertions of a solver, optimizer or goal

Assertions of a solver, optimizer or goal

## Usage

``` r
z3_assertions(x)
```

## Arguments

- x:

  A `z3_solver`, `z3_optimize` or `z3_goal`.

## Value

A list of Boolean `z3_expr` objects.

## Examples

``` r
ctx <- z3_context()
x <- z3_int(ctx, "x")
s <- z3_solver(ctx)
z3_add(s, x > 0, x < 3)
z3_assertions(s)
#> [[1]]
#> <z3_expr Bool> (> x 0)
#> 
#> [[2]]
#> <z3_expr Bool> (< x 3)
#> 
```
