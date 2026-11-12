# Theories added in Z3 5: complex numbers, finite fields and finite sets

``` r

library(z3r)
z3_version()$headers
#> [1] "5.4.1.0"
ctx <- z3_context()
```

The Z3 5 series adds new theories. z3r supports them when it is built
against a Z3 that has them; on an older Z3 the functions report the
version they need. The chunks below run only when the theory is
available.

## Complex numbers (Z3 5.3.2)

``` r

z <- z3_const(ctx, "z", z3_complex_sort(ctx))
w <- z3_const(ctx, "w", z3_complex_sort(ctx))
s <- z3_solver(ctx)
z3_add(s, z3_real_part(z) == 3, z3_imag_part(z) == -4,
       z3_real_part(w) == z3_imag_part(z), z3_imag_part(w) == z3_real_part(z))
z3_check(s)
#> [1] "sat"
m <- z3_model(s)
z3_eval(m, z)
#> <z3_expr Complex> (complex.make 3.0 (- 4.0))
z3_eval(m, w)
#> <z3_expr Complex> (complex.make (- 4.0) 3.0)
```

Addition, subtraction, multiplication, negation and division work on
complex expressions, and R complex numbers are exact literals. Ordering
is not defined. The square roots of `-1`:

``` r

s <- z3_solver(ctx)
z3_add(s, z * z == -1)
z3_check(s)
#> [1] "sat"
z3_eval(z3_model(s), z)
#> <z3_expr Complex> (complex.make 0.0 (- 1.0))
z3_simplify(z3_complex(z3_real_val(ctx, 1), 2) * (3-1i) / 2)
#> <z3_expr Complex> (complex.make (/ 5.0 2.0) (/ 5.0 2.0))
```

## Finite fields (Z3 5.3.0)

The field of integers modulo a prime. Square roots modulo 7:

``` r

F7 <- z3_ff_sort(ctx, 7)
a <- z3_const(ctx, "a", F7)
roots <- function(v) {
  s <- z3_solver(ctx)
  z3_add(s, z3_ff_mul(a, a) == z3_val(ctx, v, F7))
  out <- c()
  while (z3_check(s) == "sat") {
    r <- z3_eval(z3_model(s), a)
    out <- c(out, format(r))
    z3_add(s, a != r)
  }
  out
}
roots(2)
#> [1] "(as ff3 (_ FiniteField 7))" "(as ff4 (_ FiniteField 7))"
roots(3)
#> NULL
```

2 is a square modulo 7 and 3 is not. Large primes are given as strings:

``` r

z3_ff_size(z3_ff_sort(ctx, "21888242871839275222246405745257275088548364400416034343698204186575808495617"))
#> [1] "21888242871839275222246405745257275088548364400416034343698204186575808495617"
```

## Finite sets (Z3 5.0.0)

Unlike the array-based sets of
[`z3_set_sort()`](https://adamrsophie.github.io/z3r/reference/z3_set_sort.md),
finite sets have cardinality:

``` r

I <- z3_int_sort(ctx)
S <- z3_finite_set_sort(I)
A <- z3_const(ctx, "A", S)
s <- z3_solver(ctx)
z3_add(s, z3_fs_member(1, A), z3_fs_member(3, A),
       z3_fs_subset(A, z3_fs_union(z3_fs_singleton(z3_int_val(ctx, 1)), z3_fs_singleton(z3_int_val(ctx, 3)))))
z3_check(s)
#> [1] "sat"
z3_eval(z3_model(s), A)
#> <z3_expr (FiniteSet Int)> (set.union (set.singleton 3) (set.singleton 1))
```

The theory is new and incomplete in Z3 5.4.1: some problems mixing
cardinality and ranges return `"unknown"`:

``` r

s <- z3_solver(ctx)
z3_add(s, z3_fs_size(A) == 3, z3_fs_subset(A, z3_fs_range(z3_int_val(ctx, 1), 4)))
z3_check(s)
#> [1] "unknown"
z3_reason_unknown(s)
#> [1] "smt tactic failed to show goal to be sat/unsat (incomplete (theory finite_set))"
```
