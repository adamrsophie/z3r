# Implied equalities

Groups terms into classes that are equal in every model of the solver's
assertions. Terms with the same class number are provably equal.

## Usage

``` r
z3_implied_equalities(solver, terms)
```

## Arguments

- solver:

  A `z3_solver`.

- terms:

  A list of expressions.

## Value

A list with the check `status` and an integer `class` per term.

## Examples

``` r
ctx <- z3_context()
x <- z3_int(ctx, "x")
y <- z3_int(ctx, "y")
z <- z3_int(ctx, "z")
s <- z3_solver(ctx)
z3_add(s, x == y + 1, y + 1 == z)
z3_implied_equalities(s, list(x, y, z))
#> $status
#> [1] "sat"
#> 
#> $class
#> [1] 0 1 0
#> 
```
