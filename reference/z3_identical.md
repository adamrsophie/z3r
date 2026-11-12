# Structural equality of expressions

Tests whether two expressions are the same term (after Z3's internal
hash-consing). This is not logical equivalence: `x + 1` and `1 + x` are
different terms. Use `==` to build a symbolic equation instead.

## Usage

``` r
z3_identical(x, y)
```

## Arguments

- x, y:

  `z3_expr`, `z3_sort` or `z3_func_decl` objects.

## Value

`TRUE` or `FALSE`.

## Details

Sorts and function declarations are compared the same way.

## Examples

``` r
ctx <- z3_context()
z3_identical(z3_int(ctx, "x"), z3_int(ctx, "x"))
#> [1] TRUE
z3_identical(z3_int_sort(ctx), z3_sort(z3_int(ctx, "x")))
#> [1] TRUE
```
