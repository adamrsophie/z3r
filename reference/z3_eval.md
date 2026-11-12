# Evaluate an expression in a model

Substitutes the model's assignments into `x` and simplifies.

## Usage

``` r
z3_eval(model, x, completion = FALSE)
```

## Arguments

- model:

  A `z3_model`.

- x:

  A `z3_expr`, or an R scalar.

- completion:

  Whether to complete the model; see Details.

## Value

A `z3_expr`. Use
[`z3_value()`](https://adamrsophie.github.io/z3r/reference/z3_value.md)
to convert it to an R value.

## Details

With `completion = FALSE` (the default), constants that the model does
not mention stay symbolic, so the result shows which parts of `x` the
model leaves unconstrained. With `completion = TRUE`, Z3 assigns such
constants a default value (for example 0 or `FALSE`) and records it in
the model, so the result is always a value.

## Examples

``` r
ctx <- z3_context()
x <- z3_int(ctx, "x")
y <- z3_int(ctx, "y")
s <- z3_solver(ctx)
z3_add(s, x == 3)
z3_check(s)
#> [1] "sat"
m <- z3_model(s)
z3_eval(m, x + 1)
#> <z3_expr Int> 4
z3_eval(m, x + y)
#> <z3_expr Int> (+ 3 y)
z3_eval(m, x + y, completion = TRUE)
#> <z3_expr Int> 3
```
