# Translate objects between contexts

Copies an expression, sort, function, solver, optimizer, model or goal
into another context. This is the only way to combine objects from
different contexts. Translating an expression, sort or function into its
own context returns it unchanged; a solver, optimizer, model or goal is
always copied. Optimizers need Z3 4.15.5 or later.

## Usage

``` r
z3_translate(x, ctx)
```

## Arguments

- x:

  A `z3_expr`, `z3_sort`, `z3_function` (or declaration), `z3_solver`,
  `z3_optimize`, `z3_model` or `z3_goal`, or a list of expressions.

- ctx:

  The target
  [`z3_context()`](https://adamrsophie.github.io/z3r/reference/z3_context.md).

## Value

An object of the same class in `ctx`; for a list, a list of expressions
in `ctx`.

## Details

A translated solver or optimizer keeps its assertions and the parameters
set with
[`z3_set_params()`](https://adamrsophie.github.io/z3r/reference/z3_set_params.md),
which z3r applies to the copy again (Z3 does not carry over every limit,
such as `rlimit`), and its tracked constraints. The copy has no open
scopes: Z3 copies all assertions at the base level, so
[`z3_pop()`](https://adamrsophie.github.io/z3r/reference/z3_push.md) on
it cannot remove them. Objective handles from
[`z3_maximize()`](https://adamrsophie.github.io/z3r/reference/z3_minimize.md)
and
[`z3_minimize()`](https://adamrsophie.github.io/z3r/reference/z3_minimize.md)
belong to the original optimizer; read the copy's optima from its model
or with
[`z3_objectives()`](https://adamrsophie.github.io/z3r/reference/z3_objectives.md).

A plain list of expressions (all from one context) is translated in one
step, so subterms shared between its elements are copied once; names are
kept.

## Examples

``` r
a <- z3_context()
b <- z3_context()
x <- z3_int(a, "x")
y <- z3_translate(x + 1, b)
z3_same_context(y, b)
#> [1] TRUE
z3_translate(list(lo = x > 0, hi = x < 10), b)
#> $lo
#> <z3_expr Bool> (> x 0)
#> 
#> $hi
#> <z3_expr Bool> (< x 10)
#> 
```
