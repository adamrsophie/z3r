# Working with symcompR

[symcompR](https://github.com/adamrsophie/symcompR) is a computer
algebra system for R. The two packages answer different questions:

- symcompR simplifies, differentiates, integrates and solves equations
  in closed form;
- z3r decides whether combinations of constraints can be satisfied,
  finds witnesses, and proves properties for all inputs.

[`z3_from_symbolic()`](https://adamrsophie.github.io/z3r/reference/z3_from_symbolic.md)
and
[`z3_to_symbolic()`](https://adamrsophie.github.io/z3r/reference/z3_from_symbolic.md)
move expressions between them. Only exact translations are made:
integers, rationals, arithmetic, powers with integer exponents,
relations, logic, `Abs`, `floor`, `ceiling`, `Mod`, `Max`, `Min` and
`Piecewise`. Floats, `pi`, `oo` and transcendental functions are
refused, because Z3 has no exact counterpart for them.

``` r

library(z3r)
library(symcompR)
ctx <- z3_context()
```

## From symcompR to Z3

Symbols carry their sort through assumptions:

``` r

x <- Symbol("x", integer = TRUE)
y <- Symbol("y", integer = TRUE)
circle <- Eq(x^2 + y^2, 25)
s <- z3_solver(ctx)
z3_add(s, z3_from_symbolic(ctx, circle), z3_from_symbolic(ctx, Gt(x, y)), z3_from_symbolic(ctx, Gt(y, 0)))
z3_check(s)
#> [1] "sat"
z3_model(s)
#> <z3_model>
#>   y = 3
#>   x = 4
```

symcompR finds closed forms; Z3 then answers questions symcompR cannot.
Here symcompR expands a polynomial and Z3 proves it is never negative
over the reals:

``` r

r <- Symbol("r", real = TRUE)
p <- expand((r - 1)^2 * (r + 2)^2)
p
#> r^4 + 2*r^3 - 3*r^2 - 4*r + 4
s <- z3_solver(ctx)
z3_add(s, z3_from_symbolic(ctx, Lt(p, 0)))
z3_check(s)
#> [1] "unsat"
```

Symbols without assumptions are Booleans in a logical context, and need
a sort otherwise:

``` r

z3_from_symbolic(ctx, And(Symbol("a"), Not(Symbol("b"))))
#> <z3_expr Bool> (and (not b) a)
z3_from_symbolic(ctx, Symbol("z") + 1)
#> Error:
#> ! cannot tell the sort of symbol 'z': give it the assumption integer = TRUE or real = TRUE, or pass sorts = list(z = <sort>)
z3_from_symbolic(ctx, Symbol("z") + 1, sorts = list(z = z3_real_sort(ctx)))
#> <z3_expr Real> (+ 1.0 z)
```

`Mod` keeps symcompR’s (Python’s) sign convention, which differs from
Z3’s `mod`:

``` r

m <- z3_from_symbolic(ctx, x %% -3)
m
#> <z3_expr Int> (- x (* (- 3) (to_int (/ (to_real x) (- 3.0)))))
z3_simplify(z3_substitute(m, z3_int(ctx, "x"), 7))
#> <z3_expr Int> (- 2)
```

## From Z3 to symcompR

``` r

zx <- z3_int(ctx, "x")
z3_to_symbolic(zx * 2 + 1)
#> 2*x + 1
z3_to_symbolic(z3_and(zx > 1, zx <= 5))
#> (x <= 5) & (x > 1)
z3_to_symbolic(z3_ite(zx > 0, zx, -zx))
#> Piecewise((x, x > 0), (-x, True))
```

Z3’s Euclidean `div` and `mod` are written with `floor` and `Abs`, so
the translation keeps their meaning. A round trip is checked by the
solver:

``` r

t <- zx %% -3
back <- z3_from_symbolic(ctx, z3_to_symbolic(t))
s <- z3_solver(ctx)
z3_add(s, back != t)
z3_check(s)
#> [1] "unsat"
```
