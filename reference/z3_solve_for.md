# Solve for variables

Asks the solver to express variables as terms over the others, valid
under the returned guards. Z3 may solve for only some of the variables;
the result lists those it solved for (`vars`) with their `terms`, in the
same order. Needs Z3 4.14.0 or later. Only the plain SMT solver
(`z3_solver(ctx, simple = TRUE)`) solves for variables; the default
solver returns empty lists.

## Usage

``` r
z3_solve_for(x, vars)
```

## Arguments

- x:

  A `z3_solver`.

- vars:

  A list of constants.

## Value

A list with elements `vars`, `terms` and `guards`, lists of `z3_expr`;
`vars[[i]]` equals `terms[[i]]` under the guards.

## Examples

``` r
ctx <- z3_context()
x <- z3_int(ctx, "x")
y <- z3_int(ctx, "y")
s <- z3_solver(ctx, simple = TRUE)
z3_add(s, x + 2 * y == 10)
z3_check(s)
#> [1] "sat"
if (z3_version_at_least("4.14.0")) z3_solve_for(s, list(x))
#> $vars
#> $vars[[1]]
#> <z3_expr Int> x
#> 
#> 
#> $terms
#> $terms[[1]]
#> <z3_expr Int> (+ (* (- 2) y) 10)
#> 
#> 
#> $guards
#> $guards[[1]]
#> <z3_expr Bool> (and (<= (+ x (* 2 y)) 10) (>= (+ x (* 2 y)) 10))
#> 
#> 
```
