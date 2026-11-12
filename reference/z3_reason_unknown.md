# Reason for an unknown result

Reason for an unknown result

## Usage

``` r
z3_reason_unknown(x)
```

## Arguments

- x:

  A `z3_solver` or `z3_optimize`.

## Value

A string such as `"timeout"` or `"incomplete quantifiers"`.

## Examples

``` r
ctx <- z3_context()
s <- z3_solver(ctx)
z3_reason_unknown(s)
#> [1] "unknown"
```
