# Sort of an expression

Sort of an expression

## Usage

``` r
z3_sort(x)

z3_sort_kind(x)

z3_sort_name(x)
```

## Arguments

- x:

  A `z3_expr`.

## Value

For `z3_sort()`, the `z3_sort` of `x`. For `z3_sort_kind()`, one of
`"bool"`, `"int"`, `"real"`, `"bitvector"`, `"array"`, `"string"`,
`"uninterpreted"` or another lower-case Z3 sort kind; `x` may be an
expression or a sort. For `z3_sort_name()`, the name of the sort as Z3
stores it, such as `"Int"` or the name given to an uninterpreted or
datatype sort, without the parameters of sorts such as bit-vectors (use
[`format()`](https://rdrr.io/r/base/format.html) for the full sort); `x`
may be an expression or a sort.

## Examples

``` r
ctx <- z3_context()
x <- z3_int(ctx, "x")
z3_sort(x)
#> <z3_sort> Int
z3_sort_kind(x / 2)
#> [1] "real"
z3_sort_name(z3_uninterpreted_sort(ctx, "Person"))
#> [1] "Person"
```
