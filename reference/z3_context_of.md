# Context of a Z3 object

Context of a Z3 object

## Usage

``` r
z3_context_of(x)

z3_same_context(x, y)
```

## Arguments

- x:

  Any z3r object: an expression, sort, solver, model, optimizer, goal or
  tactic.

- y:

  Another z3r object.

## Value

The `z3_context` that owns `x`.

For `z3_same_context()`, `TRUE` if both objects belong to the same Z3
context.

## Examples

``` r
ctx <- z3_context()
x <- z3_int(ctx, "x")
z3_same_context(z3_context_of(x), ctx)
#> [1] TRUE
```
