# Goal properties

`z3_goal_info()` reports the number of formulas, expression count, depth
(number of tactic transformations), precision (`"precise"`, `"under"`,
`"over"`, `"under_over"`) and whether the goal is known to be
inconsistent. `z3_goal_reset()` removes all formulas.
`z3_goal_convert_model()` turns a model of a subgoal into a model of the
original goal, undoing the tactic's transformations.

## Usage

``` r
z3_goal_info(goal)

z3_goal_reset(goal)

z3_goal_convert_model(goal, model)
```

## Arguments

- goal:

  A `z3_goal`.

- model:

  A `z3_model` of a subgoal.

## Value

See Details.

## Examples

``` r
ctx <- z3_context()
x <- z3_int(ctx, "x")
y <- z3_int(ctx, "y")
g <- z3_goal(ctx)
z3_add(g, x == y + 1, y > 2)
sub <- z3_apply_tactic(z3_tactic(ctx, "solve-eqs"), g)[[1]]
s <- z3_solver(ctx)
z3_add(s, z3_goal_formulas(sub))
z3_check(s)
#> [1] "sat"
m <- z3_goal_convert_model(sub, z3_model(s))
z3_eval(m, x)
#> <z3_expr Int> 4
```
