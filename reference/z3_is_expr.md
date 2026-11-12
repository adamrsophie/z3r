# Test for z3r objects

Test for z3r objects

## Usage

``` r
z3_is_expr(x)
```

## Arguments

- x:

  Any R object.

## Value

`TRUE` if `x` is a Z3 expression.

## Examples

``` r
ctx <- z3_context()
is_z3_expr_value <- z3_is_expr(z3_int(ctx, "x"))
```
