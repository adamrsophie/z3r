# Algebraic numbers and real closed fields

``` r

library(z3r)
ctx <- z3_context()
```

Z3 decides nonlinear real arithmetic exactly, and represents irrational
solutions as *algebraic numbers*: a polynomial plus the index of one of
its real roots. z3r gives exact arithmetic on them, and access to Z3’s
real closed field package, which adds pi, e and infinitesimals.

## Algebraic numbers from models

``` r

r <- z3_real(ctx, "r")
s <- z3_solver(ctx)
z3_add(s, r^2 == 2, r > 0)
z3_check(s)
#> [1] "sat"
root2 <- z3_eval(z3_model(s), r)
root2
#> <z3_expr Real> (root-obj (+ (^ x 2) (- 2)) 2)
z3_decimal(root2, 40)
#> [1] "1.4142135623730950488016887242096980785696?"
z3_algebraic_poly(root2)
#> $coefficients
#> $coefficients[[1]]
#> <z3_expr Real> (- 2.0)
#> 
#> $coefficients[[2]]
#> <z3_expr Real> 0.0
#> 
#> $coefficients[[3]]
#> <z3_expr Real> 1.0
#> 
#> 
#> $index
#> [1] 2
```

Arithmetic stays exact:

``` r

z3_algebraic("*", root2, root2)
#> <z3_expr Real> 2.0
z3_algebraic("+", root2, 1)
#> <z3_expr Real> (root-obj (+ (^ x 2) (* (- 2) x) (- 1)) 2)
z3_algebraic("<", root2, z3_real_val(ctx, "1.4142136"))
#> [1] TRUE
z3_algebraic_power(root2, 6)
#> <z3_expr Real> 8.0
cbrt5 <- z3_algebraic_root(z3_real_val(ctx, 5), 3)
z3_algebraic_power(cbrt5, 3)
#> <z3_expr Real> 5.0
z3_algebraic_sign(z3_algebraic("-", cbrt5, root2))
#> [1] 1
```

## Roots of polynomials

[`z3_algebraic_roots()`](https://adamrsophie.github.io/z3r/reference/z3_algebraic.md)
finds all real roots of a polynomial in its last free variable. Free
variables are made with
[`z3_bound_var()`](https://adamrsophie.github.io/z3r/reference/z3_inspect.md):

``` r

v <- z3_bound_var(ctx, 0, z3_real_sort(ctx))
roots <- z3_algebraic_roots(v^3 - 3 * v + 1)
roots
#> [[1]]
#> <z3_expr Real> (root-obj (+ (^ x 3) (* (- 3) x) 1) 1)
#> 
#> [[2]]
#> <z3_expr Real> (root-obj (+ (^ x 3) (* (- 3) x) 1) 2)
#> 
#> [[3]]
#> <z3_expr Real> (root-obj (+ (^ x 3) (* (- 3) x) 1) 3)
vapply(roots, function(a) z3_value(a, exact = FALSE), 0)
#> [1] -1.8793852  0.3472964  1.5320889
```

## Real closed fields

The real closed field package computes with real algebraic numbers, the
transcendental constants pi and e, and infinitesimals, outside any
solver:

``` r

two <- z3_rcf(ctx, 2)
zero <- z3_rcf(ctx, 0)
one <- z3_rcf(ctx, 1)
rs <- z3_rcf_roots(list(-two, zero, one))    # roots of x^2 - 2
rs
#> [[1]]
#> <z3_rcf> root(x^2 + -2, (-oo, 0), {})
#> 
#> [[2]]
#> <z3_rcf> root(x^2 + -2, (0, +oo), {})
rs[[2]]^2 == two
#> [1] TRUE
z3_rcf_decimal(rs[[2]] * rs[[2]] + 1, 10)
#> [1] "3"
```

Transcendental numbers compare exactly against rationals:

``` r

piv <- z3_rcf_pi(ctx)
z3_rcf_decimal(piv, 30)
#> [1] "3.141592653589793238462643383279?"
piv > z3_rcf(ctx, "355/113") - z3_rcf(ctx, "1/1000000")
#> [1] TRUE
z3_rcf_e(ctx) < z3_rcf(ctx, "2719/1000")
#> [1] TRUE
```

An infinitesimal is positive but smaller than every positive real:

``` r

eps <- z3_rcf_infinitesimal(ctx)
c(positive = eps > 0, tiny = eps < z3_rcf(ctx, "1/1000000000000"))
#> positive     tiny 
#>     TRUE     TRUE
format(one + eps)
#> [1] "eps!1 + 1"
```

From Z3 4.12.3,
[`z3_rcf_info()`](https://adamrsophie.github.io/z3r/reference/z3_rcf.md)
describes a number’s structure: for an algebraic number its defining
polynomial, isolating interval and sign conditions.

``` r

info <- z3_rcf_info(rs[[2]])
info$kind
#> [1] "algebraic"
vapply(info$coefficients, format, "")
#> [1] "-2" "0"  "1"
```

## Subresultants

``` r

p <- z3_real(ctx, "p")
z3_subresultants(p^2 - 1, p^2 - 3 * p + 2, p)
#> [[1]]
#> <z3_expr Int> (- 3)
```
