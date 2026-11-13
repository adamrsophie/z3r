# Objectives of an optimizer

Lists the objectives Z3 holds, including those added by
[`z3_add_smtlib()`](https://adamrsophie.github.io/z3r/reference/z3_to_smtlib.md),
in the order
[`z3_check()`](https://adamrsophie.github.io/z3r/reference/z3_check.md)
optimizes them. Z3 lists each term in minimized form: a maximized term
appears negated, and soft constraints appear as the weighted sum of
their violations.

## Usage

``` r
z3_objectives(opt)
```

## Arguments

- opt:

  A
  [`z3_optimize()`](https://adamrsophie.github.io/z3r/reference/z3_optimize.md).

## Value

A list of `z3_expr`.

## Examples

``` r
ctx <- z3_context()
x <- z3_int(ctx, "x")
y <- z3_int(ctx, "y")
opt <- z3_optimize(ctx)
z3_add(opt, x >= 0, y >= 0, x + 2 * y <= 14, 3 * x - y >= 0, x - y <= 2)
obj <- z3_maximize(opt, 3 * x + 4 * y)
z3_check(opt)
#> [1] "sat"
z3_optimum(obj)
#> <z3_optimum> 34
z3_model(opt)
#> <z3_model>
#>   y = 4
#>   x = 6
```
