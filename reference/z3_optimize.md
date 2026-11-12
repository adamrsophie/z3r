# Create an optimizer

An optimizer is a solver that also minimizes or maximizes objectives and
satisfies as many weighted soft constraints as possible. It is a
separate object from
[`z3_solver()`](https://adamrsophie.github.io/z3r/reference/z3_solver.md)
because Z3's optimization engine has different capabilities: it does not
support tactics, and over nonlinear arithmetic or quantifiers it may
return `"unknown"` or a non-optimal answer.

## Usage

``` r
z3_optimize(ctx, ...)
```

## Arguments

- ctx:

  A
  [`z3_context()`](https://adamrsophie.github.io/z3r/reference/z3_context.md).

- ...:

  Optimizer parameters, passed to
  [`z3_set_params()`](https://adamrsophie.github.io/z3r/reference/z3_set_params.md).

## Value

An object of class `z3_optimize`.

## Details

Use [`z3_add()`](https://adamrsophie.github.io/z3r/reference/z3_add.md)
for hard constraints,
[`z3_add_soft()`](https://adamrsophie.github.io/z3r/reference/z3_add_soft.md)
for soft constraints,
[`z3_minimize()`](https://adamrsophie.github.io/z3r/reference/z3_minimize.md)
and
[`z3_maximize()`](https://adamrsophie.github.io/z3r/reference/z3_minimize.md)
for objectives, then
[`z3_check()`](https://adamrsophie.github.io/z3r/reference/z3_check.md),
[`z3_model()`](https://adamrsophie.github.io/z3r/reference/z3_model.md)
and
[`z3_optimum()`](https://adamrsophie.github.io/z3r/reference/z3_optimum.md).

With several objectives, the default priority is lexicographic: the
first objective is optimized first. Use
`z3_set_params(opt, priority = "box")` to optimize each objective
independently, or `priority = "pareto"` to enumerate Pareto-optimal
points with repeated calls to
[`z3_check()`](https://adamrsophie.github.io/z3r/reference/z3_check.md).

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
