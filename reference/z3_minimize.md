# Objectives

Adds an objective to an optimizer. The returned handle is used with
[`z3_optimum()`](https://adamrsophie.github.io/z3r/reference/z3_optimum.md)
after a satisfiable
[`z3_check()`](https://adamrsophie.github.io/z3r/reference/z3_check.md).

## Usage

``` r
z3_minimize(opt, x)

z3_maximize(opt, x)
```

## Arguments

- opt:

  A `z3_optimize`.

- x:

  An Int, Real or bit-vector expression. Bit-vectors are optimized as
  unsigned numbers.

## Value

A `z3_objective` handle.

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
