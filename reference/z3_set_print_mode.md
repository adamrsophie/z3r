# Printing mode

Sets how expressions of a context are printed: `"smtlib2"` (the default,
SMT-LIB2 compliant), `"low_level"` (Z3's internal form with sharing), or
`"smtlib_full"`.

## Usage

``` r
z3_set_print_mode(ctx, mode = c("smtlib2", "low_level", "smtlib_full"))
```

## Arguments

- ctx:

  A
  [`z3_context()`](https://adamrsophie.github.io/z3r/reference/z3_context.md).

- mode:

  One of the modes above.

## Value

`ctx`, invisibly.

## Examples

``` r
ctx <- z3_context()
z3_set_print_mode(ctx, "low_level")
z3_int(ctx, "x") + 1
#> <z3_expr Int> (+ x 1)
#> 
```
