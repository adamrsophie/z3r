# Congruence classes

After a satisfiable check, terms that the solver found equal belong to
one congruence class. `z3_congruence_root()` returns a class
representative and `z3_congruence_next()` the next member of the class,
cycling back to the start. `z3_congruence_explain()` returns the
explanation of why two terms are equal. Root and next need Z3 4.12.0;
explain needs 4.14.0. Only some solver configurations track classes.

## Usage

``` r
z3_congruence_root(x, a)

z3_congruence_next(x, a)

z3_congruence_explain(x, a, b)
```

## Arguments

- x:

  A `z3_solver`.

- a, b:

  Expressions.

## Value

A `z3_expr`.

## Examples

``` r
ctx <- z3_context()
a <- z3_int(ctx, "a")
b <- z3_int(ctx, "b")
s <- z3_solver(ctx)
z3_add(s, a == b)
z3_check(s)
#> [1] "sat"
if (z3_version_at_least("4.12.0")) z3_congruence_root(s, a)
#> <z3_expr Int> a
```
