# Interrupt a running check

Stops the check that is running on `solver`. It is meant for R code that
runs inside that check: the callbacks of a user propagator
([`z3_propagator()`](https://adamrsophie.github.io/z3r/reference/z3_propagator.md))
or of
[`z3_on_clause()`](https://adamrsophie.github.io/z3r/reference/z3_propagator.md),
which cannot otherwise use their own solver, and the model callback of
an optimizer
([`z3_on_model()`](https://adamrsophie.github.io/z3r/reference/z3_propagator.md)).
The check then returns `"unknown"`; the solver or optimizer stays
usable. For a solver
[`z3_reason_unknown()`](https://adamrsophie.github.io/z3r/reference/z3_reason_unknown.md)
reports `"interrupted"`. An optimizer keeps the best model found so far,
whose objective values
[`z3_optimum()`](https://adamrsophie.github.io/z3r/reference/z3_optimum.md)
reports. Z3 notices the request at its next opportunity, so a callback
may still run a few more times. When no check is running,
`z3_interrupt()` does nothing.

## Usage

``` r
z3_interrupt(solver)
```

## Arguments

- solver:

  A `z3_solver` or `z3_optimize`.

## Value

`solver`, invisibly.

## Details

Z3 can interrupt a single solver but not a single optimizer; for an
optimizer `z3_interrupt()` interrupts its context while the optimizer's
check runs.

## Examples

``` r
ctx <- z3_context()
if (z3_version_at_least("4.12.3")) {
  p <- z3_bools(ctx, paste0("p", 1:3))
  s <- z3_solver(ctx)
  z3_add(s, z3_or(p), z3_or(!p$p1, !p$p2))
  # Stop the search at the first clause Z3 reports.
  z3_on_clause(s, function(proof_hint, deps, literals) z3_interrupt(s))
  z3_check(s)
}
#> [1] "unknown"

# Stop an optimizer at its first model.
x <- z3_int(ctx, "x")
opt <- z3_optimize(ctx)
z3_add(opt, x >= 0, x <= 100)
obj <- z3_maximize(opt, x)
z3_on_model(opt, function(model) z3_interrupt(opt))
z3_check(opt)
#> [1] "unknown"
z3_optimum(obj)
#> <z3_optimum> 0 (best so far, not proven optimal)
```
