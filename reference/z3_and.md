# Logical connectives

Symbolic versions of R's logical operators. Arguments are `z3_expr`
Booleans, R `TRUE`/`FALSE`, or lists of them.

## Usage

``` r
z3_and(...)

z3_or(...)

z3_not(p)

z3_implies(p, q)

z3_xor(p, q)

z3_iff(p, q)

z3_ite(cond, a, b)

z3_distinct(...)
```

## Arguments

- ...:

  Boolean expressions, or for `z3_distinct()` expressions of a common
  sort. Lists are spliced.

- p, q:

  Boolean expressions.

- cond:

  A Boolean expression.

- a, b:

  Expressions of compatible sorts.

## Value

A `z3_expr`.

## Details

`z3_and()` and `z3_or()` need at least one argument, from which they
take the context; to combine a list that may be empty, include the
identity, as in `z3_and(z3_bool_val(ctx, TRUE), constraints)`.
`z3_ite(cond, a, b)` is "if `cond` then `a` else `b`" as an expression;
`a` and `b` must have compatible sorts. `z3_distinct()` states that all
its arguments are pairwise different.

## Examples

``` r
ctx <- z3_context()
p <- z3_bool(ctx, "p")
q <- z3_bool(ctx, "q")
z3_and(p, q)
#> <z3_expr Bool> (and p q)
z3_implies(p, q)
#> <z3_expr Bool> (=> p q)
x <- z3_int(ctx, "x")
z3_ite(x > 0, x, -x)
#> <z3_expr Int> (ite (> x 0) x (- x))
v <- z3_ints(ctx, c("a", "b", "c"))
z3_distinct(v)
#> <z3_expr Bool> (distinct a b c)
```
