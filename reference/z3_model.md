# Model of the last satisfiable check

Returns the model found by the most recent
[`z3_check()`](https://adamrsophie.github.io/z3r/reference/z3_check.md).
A model is only available when that check returned `"sat"` and the
solver has not been changed since (by
[`z3_add()`](https://adamrsophie.github.io/z3r/reference/z3_add.md),
[`z3_push()`](https://adamrsophie.github.io/z3r/reference/z3_push.md),
[`z3_pop()`](https://adamrsophie.github.io/z3r/reference/z3_push.md) or
[`z3_reset()`](https://adamrsophie.github.io/z3r/reference/z3_push.md)),
so a model never describes constraints it was not computed for.

## Usage

``` r
# S3 method for class 'z3_context'
z3_model(x)

z3_model(x)
```

## Arguments

- x:

  A `z3_solver` or `z3_optimize`.

## Value

A `z3_model`.

## Details

The returned model is a snapshot: it stays valid after the solver
changes.

## See also

[`z3_eval()`](https://adamrsophie.github.io/z3r/reference/z3_eval.md),
[`z3_model_values()`](https://adamrsophie.github.io/z3r/reference/as.list.z3_model.md).

## Examples

``` r
ctx <- z3_context()
x <- z3_int(ctx, "x")
s <- z3_solver(ctx)
z3_add(s, x * x == 49, x < 0)
z3_check(s)
#> [1] "sat"
m <- z3_model(s)
z3_eval(m, x)
#> <z3_expr Int> (- 7)
```
