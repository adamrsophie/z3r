# Cardinality and pseudo-Boolean constraints

`z3_atmost(..., k)` and `z3_atleast(..., k)` state that at most (at
least) `k` of the Boolean arguments are true. `z3_pble()`, `z3_pbge()`
and `z3_pbeq()` compare a weighted sum of Booleans,
`sum(coeffs[i] * lits[i])`, with `k`. Coefficients and `k` are integers.
Z3 solves these with dedicated procedures, which is much faster than
encoding them with
[`z3_ite()`](https://adamrsophie.github.io/z3r/reference/z3_and.md).

## Usage

``` r
z3_atmost(..., k)

z3_atleast(..., k)

z3_pble(lits, coeffs, k)

z3_pbge(lits, coeffs, k)

z3_pbeq(lits, coeffs, k)
```

## Arguments

- ...:

  Boolean expressions or lists of them.

- k:

  The bound, an integer.

- lits:

  A list of Boolean expressions.

- coeffs:

  Integer coefficients, one per literal.

## Value

A Boolean `z3_expr`.

## Examples

``` r
ctx <- z3_context()
p <- z3_bools(ctx, letters[1:4])
s <- z3_solver(ctx)
z3_add(s, z3_atleast(p, k = 3), z3_pble(p, c(1, 2, 3, 4), 6))
z3_check(s)
#> [1] "sat"
unlist(z3_model_values(z3_model(s)))
#>     b     c     d     a 
#>  TRUE  TRUE FALSE  TRUE 
```
