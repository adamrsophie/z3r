# Initial values (phase hints)

Suggests a value for a variable that the solver tries first. It does not
constrain the result. Needs Z3 4.13.2 or later. Z3 supports it on the
plain SMT solver (`z3_solver(ctx, simple = TRUE)`) and on optimizers,
not on the default tactic-based solver.

## Usage

``` r
z3_set_initial_value(x, var, value)
```

## Arguments

- x:

  A `z3_solver` or `z3_optimize`.

- var:

  A constant.

- value:

  Its suggested value.

## Value

`x`, invisibly.

## Examples

``` r
ctx <- z3_context()
x <- z3_int(ctx, "x")
s <- z3_solver(ctx, simple = TRUE)
z3_add(s, x > 0)
if (z3_version_at_least("4.13.2")) z3_set_initial_value(s, x, 42)
z3_check(s)
#> [1] "sat"
```
