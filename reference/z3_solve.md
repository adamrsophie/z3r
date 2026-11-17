# Solve or prove in one call

Shortcuts that create a solver, add the constraints and check them.
`z3_solve()` returns a model of the constraints, or `NULL` when they are
unsatisfiable. `z3_prove()` checks that a Boolean claim holds for every
value of its constants: it returns `TRUE` when it does, and otherwise
`FALSE` with a counterexample model in the attribute `"counterexample"`.
Both raise an error when Z3 returns `"unknown"`.

## Usage

``` r
z3_solve(..., timeout = NULL)

z3_prove(claim, timeout = NULL)
```

## Arguments

- ...:

  Boolean `z3_expr` constraints, or lists of them.

- timeout:

  Optional time limit in milliseconds.

- claim:

  A Boolean `z3_expr`.

## Value

`z3_solve()`: a `z3_model` or `NULL`. `z3_prove()`: `TRUE`, or `FALSE`
with a counterexample.

## Examples

``` r
ctx <- z3_context()
x <- z3_int(ctx, "x")
z3_solve(x * x == 49, x < 0)
#> <z3_model>
#>   x = (- 7)
z3_prove(x * x >= 0)
#> [1] TRUE
z3_prove(x * x > 0)
#> [1] FALSE
#> attr(,"counterexample")
#> <z3_model>
#>   x = 0
```
