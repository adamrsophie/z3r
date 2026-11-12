# Incremental solving: scopes

`z3_push()` opens a backtracking scope; `z3_pop()` removes every
assertion added since the matching push. `z3_reset()` removes all
assertions and scopes, and also user propagators and clause callbacks
([`z3_propagator()`](https://adamrsophie.github.io/z3r/reference/z3_propagator.md)).
`z3_num_scopes()` reports the number of open scopes. Popping an
optimizer removes the objectives added in the scope; their handles can
no longer be used.

## Usage

``` r
z3_push(x)

z3_pop(x, n = 1)

z3_reset(x)

z3_num_scopes(x)
```

## Arguments

- x:

  A `z3_solver` (or a `z3_optimize` for push and pop).

- n:

  Number of scopes to pop.

## Value

`x`, invisibly; `z3_num_scopes()` returns an integer.

## Examples

``` r
ctx <- z3_context()
x <- z3_int(ctx, "x")
s <- z3_solver(ctx)
z3_add(s, x > 0)
z3_push(s)
z3_add(s, x < 0)
z3_check(s)
#> [1] "unsat"
z3_pop(s)
z3_check(s)
#> [1] "sat"
```
