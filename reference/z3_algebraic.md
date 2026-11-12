# Algebraic number arithmetic

Exact arithmetic on Z3 numeral expressions, including the irrational
algebraic numbers that appear in models of nonlinear real problems (see
[`z3_value()`](https://adamrsophie.github.io/z3r/reference/z3_value.md)).
The results are again exact numerals.

## Usage

``` r
z3_algebraic(op, a, b)

z3_algebraic_root(a, k)

z3_algebraic_power(a, k)

z3_algebraic_sign(a)

z3_algebraic_poly(a)

z3_algebraic_roots(p, values = list())

z3_algebraic_eval(p, values = list())

z3_subresultants(p, q, x)

z3_algebraic_bounds(a, precision = 10)
```

## Arguments

- op:

  Operation name.

- a, b:

  Numerals or algebraic numbers (R numbers are accepted for `b`).

- k:

  A positive whole number.

- p, q:

  Polynomial expressions.

- values:

  A list of values.

- x:

  The variable.

- precision:

  Number of decimal digits of the interval width.

## Value

A `z3_expr`, a logical, an integer, or a list.

## Details

- `z3_algebraic(op, a, b)` with `op` one of `"+"`, `"-"`, `"*"`, `"/"`,
  or a comparison (`"<"`, `">"`, `"<="`, `">="`, `"=="`, `"!="`, which
  return `TRUE`/`FALSE`).

- `z3_algebraic_root(a, k)` is the k-th root; `z3_algebraic_power(a, k)`
  the k-th power; `z3_algebraic_sign(a)` -1, 0 or 1.

- `z3_algebraic_bounds(a, precision)` gives rational lower and upper
  bounds within 10^-precision.

- `z3_algebraic_poly(a)` gives the coefficients of the defining
  polynomial and the index of the root it denotes.

- `z3_algebraic_roots(p, values)` finds the roots of polynomial `p` in
  its last free variable after substituting `values` for the others, and
  `z3_algebraic_eval(p, values)` gives the sign of `p` at `values`. The
  variables of `p` are bound variables made with
  [`z3_bound_var()`](https://adamrsophie.github.io/z3r/reference/z3_inspect.md).

- `z3_subresultants(p, q, x)` returns the subresultants of polynomials
  `p` and `q` with respect to `x`.

## Examples

``` r
ctx <- z3_context()
two <- z3_real_val(ctx, 2)
r <- z3_algebraic_root(two, 2)
r
#> <z3_expr Real> (root-obj (+ (^ x 2) (- 2)) 2)
z3_algebraic("*", r, r)
#> <z3_expr Real> 2.0
z3_algebraic("<", r, z3_real_val(ctx, "1.5"))
#> [1] TRUE
z3_algebraic_poly(r)
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
#> 
```
