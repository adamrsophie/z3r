# Contents of a model

- [`as.list()`](https://rdrr.io/r/base/list.html) returns the model's
  constant assignments as a named list of `z3_expr` values. Nothing is
  converted, so no precision is lost.

- `z3_model_values()` converts each value with
  [`z3_value()`](https://adamrsophie.github.io/z3r/reference/z3_value.md).
  Values that have no exact R representation (algebraic numbers, arrays,
  values of uninterpreted sorts) are kept as `z3_expr` objects.

- `z3_model_functions()` returns the interpretations of uninterpreted
  functions: for each function, a list of `entries` (argument values and
  result) and an `else` value for all other arguments (`NULL` in a
  partial model, which has none).

## Usage

``` r
# S3 method for class 'z3_model'
as.list(x, ...)

z3_model_values(model)

z3_model_functions(model)
```

## Arguments

- x, model:

  A `z3_model`.

- ...:

  Unused.

## Value

A named list.

## Details

The model only contains the constants that matter for the assertions;
see
[`z3_eval()`](https://adamrsophie.github.io/z3r/reference/z3_eval.md)
with `completion = TRUE` for the others.

## Examples

``` r
ctx <- z3_context()
x <- z3_int(ctx, "x")
r <- z3_real(ctx, "r")
s <- z3_solver(ctx)
z3_add(s, x == z3_int_val(ctx, "1180591620717411303424"), 3 * r == 1)
z3_check(s)
#> [1] "sat"
m <- z3_model(s)
as.list(m)
#> $x
#> <z3_expr Int> 1180591620717411303424
#> 
#> $r
#> <z3_expr Real> (/ 1.0 3.0)
#> 
z3_model_values(m)
#> $x
#> Big Integer ('bigz') :
#> [1] 1180591620717411303424
#> 
#> $r
#> Big Rational ('bigq') :
#> [1] 1/3
#> 
```
