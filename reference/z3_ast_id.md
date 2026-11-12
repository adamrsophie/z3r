# Expression identity and properties

`z3_ast_id()` is a number unique to the term within its context (equal
terms share it); `z3_ast_hash()` is Z3's hash. `z3_is_well_sorted()`
checks sorts recursively. `z3_is_ground()` (no bound variables) and
`z3_depth()` (height of the tree) need Z3 4.14.0 or later.

## Usage

``` r
z3_ast_id(x)

z3_ast_hash(x)

z3_is_well_sorted(x)

z3_is_ground(x)

z3_depth(x)
```

## Arguments

- x:

  A `z3_expr` (for `z3_ast_id()` also a sort or function).

## Value

A number or `TRUE`/`FALSE`.

## Examples

``` r
ctx <- z3_context()
x <- z3_int(ctx, "x")
z3_ast_id(x + 1) == z3_ast_id(x + 1)
#> [1] TRUE
z3_is_well_sorted(x + 1)
#> [1] TRUE
```
