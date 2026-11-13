# Solver statistics

`z3_statistics()` returns the statistics as numbers;
`z3_statistics_text()` returns them as Z3 writes them, one `:name value`
pair per entry.

## Usage

``` r
z3_statistics(x)

z3_statistics_text(x)
```

## Arguments

- x:

  A `z3_solver`, `z3_optimize` or `z3_fixedpoint`.

## Value

`z3_statistics()` returns a named list of numbers (counts, memory in
megabytes, time in seconds) reported by Z3 for the last check.
`z3_statistics_text()` returns a string.

## Examples

``` r
ctx <- z3_context()
s <- z3_solver(ctx)
z3_add(s, z3_int(ctx, "x") > 0)
z3_check(s)
#> [1] "sat"
names(z3_statistics(s))
#> [1] "elim-unconstrained"  "solve-eqs-steps"     "solve-eqs-elim-vars"
#> [4] "num allocs"          "rlimit count"        "max memory"         
#> [7] "memory"             
cat(z3_statistics_text(s), "\n")
#> (:elim-unconstrained  2
#>  :max-memory          82.22
#>  :memory              51.66
#>  :num-allocs          385111569
#>  :rlimit-count        84
#>  :solve-eqs-elim-vars 1
#>  :solve-eqs-steps     1) 
```
