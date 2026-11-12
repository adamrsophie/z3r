# Solver and optimizer parameters

`z3_set_params()` sets Z3 parameters such as `timeout` (milliseconds),
`rlimit` (a deterministic resource limit), `random_seed`, or
`unsat_core`. Each value's type is checked against Z3's description of
the parameter. `z3_param_help()` lists the accepted parameters.

## Usage

``` r
z3_set_params(x, ...)

z3_param_help(x)
```

## Arguments

- x:

  A `z3_solver` or `z3_optimize`.

- ...:

  Named parameter values.

## Value

`z3_set_params()` returns `x` invisibly. `z3_param_help()` returns a
data frame with columns `name`, `kind` and `description`.

## Details

A deterministic `rlimit` is preferable to `timeout` when results must be
reproducible across machines.

## Examples

``` r
ctx <- z3_context()
s <- z3_solver(ctx)
z3_set_params(s, timeout = 1000, random_seed = 42)
head(z3_param_help(s))
#>                     name   kind
#> 1            smtlib2_log symbol
#> 2     cancel_backup_file symbol
#> 3                timeout   uint
#> 4         lemmas2console   bool
#> 5 instantiations2console   bool
#> 6           axioms2files   bool
#>                                                   description
#> 1                             file to save solver interaction
#> 2     file to save partial search state if search is canceled
#> 3   timeout on the solver object; overwrites a global timeout
#> 4                                  print lemmas during search
#> 5              print quantifier instantiations to the console
#> 6 print negated theory axioms to separate files during search
```
