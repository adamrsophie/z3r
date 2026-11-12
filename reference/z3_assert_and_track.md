# Track assertions for unsatisfiable cores

Asserts `constraint` under a Boolean label. When the solver is
unsatisfiable,
[`z3_unsat_core()`](https://adamrsophie.github.io/z3r/reference/z3_unsat_core.md)
returns the labels of a subset of the tracked assertions that is itself
unsatisfiable.

## Usage

``` r
z3_assert_and_track(x, constraint, name)
```

## Arguments

- x:

  A `z3_solver` or `z3_optimize`.

- constraint:

  A Boolean expression.

- name:

  The label: a string naming a new Boolean constant. Use names that are
  not used for other constants.

## Value

The label, a Boolean `z3_expr`, invisibly.

## Examples

``` r
ctx <- z3_context()
x <- z3_int(ctx, "x")
s <- z3_solver(ctx)
z3_assert_and_track(s, x > 10, "lower_bound")
z3_assert_and_track(s, x < 5, "upper_bound")
z3_assert_and_track(s, x != 7, "not_seven")
z3_check(s)
#> [1] "unsat"
names(z3_unsat_core(s))
#> [1] "lower_bound" "upper_bound"
```
