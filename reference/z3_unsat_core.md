# Unsatisfiable core

After
[`z3_check()`](https://adamrsophie.github.io/z3r/reference/z3_check.md)
returns `"unsat"`, returns a subset of the assumptions and
tracked-assertion labels that is still unsatisfiable together with the
untracked assertions. Z3 does not guarantee that the core is minimal.
Without assumptions or tracked assertions the core is empty.

## Usage

``` r
z3_unsat_core(x)
```

## Arguments

- x:

  A `z3_solver` or `z3_optimize`.

## Value

A list of Boolean `z3_expr` objects, named by their text.

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
