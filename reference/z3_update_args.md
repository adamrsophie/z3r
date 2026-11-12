# Rebuild an application with new arguments

`z3_update_args(x, args)` returns the application `x` with its arguments
replaced by `args` (same number and sorts).

## Usage

``` r
z3_update_args(x, args)
```

## Arguments

- x:

  An application.

- args:

  A list of new arguments.

## Value

A `z3_expr`.

## Examples

``` r
ctx <- z3_context()
x <- z3_int(ctx, "x")
z3_update_args(x + 1, list(x * 2, 5))
#> <z3_expr Int> (+ (* x 2) 5)
```
