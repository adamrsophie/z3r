# Add constraints

Asserts Boolean constraints into a solver, optimizer or goal. All
constraints are checked before any is added, so a call that fails adds
nothing.

## Usage

``` r
z3_add(x, ...)
```

## Arguments

- x:

  A `z3_solver`, `z3_optimize` or `z3_goal`.

- ...:

  Boolean `z3_expr` constraints, or lists of them.

## Value

`x`, invisibly.

## Examples

``` r
ctx <- z3_context()
x <- z3_int(ctx, "x")
s <- z3_solver(ctx)
z3_add(s, x > 0, x < 10)
z3_add(s, lapply(2:4, function(k) x != k))
z3_assertions(s)
#> [[1]]
#> <z3_expr Bool> (> x 0)
#> 
#> [[2]]
#> <z3_expr Bool> (< x 10)
#> 
#> [[3]]
#> <z3_expr Bool> (not (= x 2))
#> 
#> [[4]]
#> <z3_expr Bool> (not (= x 3))
#> 
#> [[5]]
#> <z3_expr Bool> (not (= x 4))
#> 
```
