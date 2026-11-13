# Soft constraints

Adds a constraint that the optimizer should satisfy if possible. Z3
minimizes the total weight of violated soft constraints in each group
(`id`); groups act as separate objectives.

## Usage

``` r
z3_add_soft(opt, constraint, weight = 1, id = "soft")
```

## Arguments

- opt:

  A `z3_optimize`.

- constraint:

  A Boolean expression.

- weight:

  Positive weight: a number or an exact decimal or fraction string.

- id:

  Name of the group of soft constraints.

## Value

Invisibly, a `z3_objective` handle for the group: after a satisfiable
check,
[`z3_optimum()`](https://adamrsophie.github.io/z3r/reference/z3_optimum.md)
gives the total weight of its violated constraints.

## Examples

``` r
ctx <- z3_context()
p <- z3_bools(ctx, c("a", "b", "c"))
opt <- z3_optimize(ctx)
z3_add(opt, z3_or(!p$a, !p$b), z3_or(!p$b, !p$c))
z3_add_soft(opt, p$a, 2)
z3_add_soft(opt, p$b, 3)
z3_add_soft(opt, p$c, 2)
z3_check(opt)
#> [1] "sat"
z3_model_values(z3_model(opt))
#> $b
#> [1] FALSE
#> 
#> $c
#> [1] TRUE
#> 
#> $a
#> [1] TRUE
#> 
```
