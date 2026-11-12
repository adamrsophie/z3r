# Proofs, consequences and cubes

- `z3_proof()` returns Z3's proof of unsatisfiability after a check
  returned `"unsat"`. The context must have been created with
  `z3_context(proof = TRUE)`.

- `z3_consequences()` computes, for each variable, a literal that is
  implied by the assertions together with a subset of the assumptions,
  as `implies(and(assumptions), literal)` formulas. It is a check: it
  returns the status and updates the solver's last result.

- `z3_cube()` returns the next cube (a conjunction of literals as a
  list) for cube-and-conquer search over `vars`; an empty list means the
  search space is exhausted, and a list holding only `FALSE` means
  unsat.

- `z3_units()`, `z3_non_units()` and `z3_trail()` expose literals the
  solver has derived; `z3_trail()` also returns decision levels.

## Usage

``` r
z3_proof(x)

z3_consequences(x, assumptions, variables)

z3_cube(x, vars = list(), backtrack_level = 4294967295)

z3_units(x)

z3_non_units(x)

z3_trail(x)
```

## Arguments

- x:

  A `z3_solver`.

- assumptions, variables, vars:

  Lists of expressions.

- backtrack_level:

  Backtracking level for cube generation.

## Value

See Details.

## Examples

``` r
ctx <- z3_context(proof = TRUE)
p <- z3_bool(ctx, "p")
q <- z3_bool(ctx, "q")
s <- z3_solver(ctx)
z3_add(s, z3_implies(p, q), p, !q)
z3_check(s)
#> [1] "unsat"
z3_proof(s)
#> <z3_expr Proof> (mp (mp (asserted (=> p q))
#>         (rewrite (= (=> p q) (or q (not p))))
#>         (or q (not p)))
#>     (trans (monotonicity (iff-false (asserted (not q)) (= q false))
#>                          (monotonicity (iff-true (asserted p) (= p true))
#>                                        (= (not p) (not true)))
#>                          (= (or q (not p)) (or false (not true))))
#>            (trans (monotonicity (rewrite (= (not true) false))
#>                                 (= (or false (not true)) (or false false)))
#>                   (rewrite (= (or false false) false))
#>                   (= (or false (not true)) false))
#>            (= (or q (not p)) false))
#>     false)
s2 <- z3_solver(ctx)
z3_add(s2, z3_implies(p, q))
z3_consequences(s2, list(p), list(q))
#> $status
#> [1] "sat"
#> 
#> $consequences
#> $consequences[[1]]
#> <z3_expr Bool> (=> p q)
#> 
#> 
```
