# Scoped solving and model enumeration

`z3_with_scope(x, code)` evaluates `code` inside a
[`z3_push()`](https://adamrsophie.github.io/z3r/reference/z3_push.md) /
[`z3_pop()`](https://adamrsophie.github.io/z3r/reference/z3_push.md)
pair on a solver or optimizer: assertions added by `code` are removed
afterwards, also when `code` fails.

## Usage

``` r
z3_with_scope(x, code)

z3_all_models(solver, vars, limit = 100)
```

## Arguments

- x:

  A `z3_solver` or `z3_optimize`.

- code:

  Code to evaluate.

- solver:

  A `z3_solver`.

- vars:

  A constant or a list of constants.

- limit:

  Maximum number of assignments.

## Value

`z3_with_scope()` returns the value of `code`. `z3_all_models()` returns
a list of named lists, one per assignment, with attribute `"status"`.

## Details

`z3_all_models(solver, vars, limit)` lists the distinct assignments of
`vars` in models of the solver's assertions, at most `limit` of them. It
blocks each assignment found and checks again, inside a scope, so the
solver is unchanged afterwards. Values are converted as by
[`z3_model_values()`](https://adamrsophie.github.io/z3r/reference/as.list.z3_model.md).
The `"status"` attribute says why enumeration stopped: `"unsat"` (all
assignments were found), `"limit"` or `"unknown"`.

## Examples

``` r
ctx <- z3_context()
x <- z3_int(ctx, "x")
y <- z3_int(ctx, "y")
s <- z3_solver(ctx)
z3_add(s, x >= 0, y >= 0, x + y == 2)
z3_with_scope(s, {
  z3_add(s, x > 5)
  z3_check(s)
})
#> [1] "unsat"
z3_check(s)
#> [1] "sat"
sols <- z3_all_models(s, list(x, y))
length(sols)
#> [1] 3
attr(sols, "status")
#> [1] "unsat"
```
