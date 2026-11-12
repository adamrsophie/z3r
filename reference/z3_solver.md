# Create a solver

A solver holds a stack of assertions and decides whether they are
satisfiable. Add constraints with
[`z3_add()`](https://adamrsophie.github.io/z3r/reference/z3_add.md),
decide with
[`z3_check()`](https://adamrsophie.github.io/z3r/reference/z3_check.md),
and inspect the result with
[`z3_model()`](https://adamrsophie.github.io/z3r/reference/z3_model.md)
or
[`z3_unsat_core()`](https://adamrsophie.github.io/z3r/reference/z3_unsat_core.md).

## Usage

``` r
z3_solver(ctx, logic = NULL, simple = FALSE, ...)
```

## Arguments

- ctx:

  A
  [`z3_context()`](https://adamrsophie.github.io/z3r/reference/z3_context.md).

- logic:

  Optional SMT-LIB logic name such as `"QF_LIA"` (quantifier free linear
  integer arithmetic). A logic lets Z3 pick a specialized strategy;
  constraints outside the logic may then fail or return `"unknown"`.

- simple:

  If `TRUE`, create Z3's simple solver: an incremental SMT solver
  without the tactic-based preprocessing of the default solver. Some
  features, such as
  [`z3_trail()`](https://adamrsophie.github.io/z3r/reference/z3_proof.md),
  need it.

- ...:

  Solver parameters, passed to
  [`z3_set_params()`](https://adamrsophie.github.io/z3r/reference/z3_set_params.md).

## Value

An object of class `z3_solver`.

## Examples

``` r
ctx <- z3_context()
x <- z3_int(ctx, "x")
y <- z3_int(ctx, "y")
s <- z3_solver(ctx)
z3_add(s, x + y == 10, x > 2 * y, y > 0)
z3_check(s)
#> [1] "sat"
z3_model(s)
#> <z3_model>
#>   y = 1
#>   x = 9
```
