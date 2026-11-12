# Tactics and goals

A goal is a set of formulas. A tactic transforms a goal into zero or
more subgoals whose conjunction is equivalent (or equisatisfiable) to
the original; for example `"simplify"` rewrites formulas and
`"solve-eqs"` eliminates variables. Tactics compose with
`z3_tactic_then()` (apply one after the other) and `z3_tactic_or_else()`
(try the first, fall back to the second). `z3_tactics()` lists the
tactics available in the installed Z3. A tactic can also drive a solver,
see `z3_solver_from_tactic()`.

## Usage

``` r
z3_tactic(ctx, name)

z3_tactic_then(t1, t2)

z3_tactic_or_else(t1, t2)

z3_tactic_repeat(t, max = 4294967295)

z3_tactic_try_for(t, ms)

z3_tactics(ctx)

z3_solver_from_tactic(t)

z3_goal(ctx, models = TRUE, unsat_cores = FALSE, proofs = FALSE)

z3_goal_formulas(goal)

z3_goal_status(goal)

z3_apply_tactic(t, goal, ...)
```

## Arguments

- ctx:

  A
  [`z3_context()`](https://adamrsophie.github.io/z3r/reference/z3_context.md).

- name:

  Name of a built-in tactic.

- t, t1, t2:

  `z3_tactic` objects.

- max:

  Maximum number of repetitions.

- ms:

  Time limit in milliseconds.

- models, unsat_cores, proofs:

  Whether the goal should support model conversion, unsat cores and
  proofs.

- goal:

  A `z3_goal`.

- ...:

  For `z3_apply_tactic()`, tactic parameters.

## Value

`z3_tactic()` and the combinators return a `z3_tactic`; `z3_goal()`
returns a `z3_goal`; `z3_apply_tactic()` returns a list of subgoals of
class `z3_apply_result`, which prints and formats as Z3 writes the
result; `z3_goal_formulas()` returns a list of `z3_expr`;
`z3_goal_status()` returns `"sat"`, `"unsat"` or `"undecided"`.

## Examples

``` r
ctx <- z3_context()
x <- z3_int(ctx, "x")
y <- z3_int(ctx, "y")
g <- z3_goal(ctx)
z3_add(g, x == y + 1, y > 2, x < 10)
t <- z3_tactic_then(z3_tactic(ctx, "simplify"), z3_tactic(ctx, "solve-eqs"))
sub <- z3_apply_tactic(t, g)
z3_goal_formulas(sub[[1]])
#> [[1]]
#> <z3_expr Bool> (not (<= x 3))
#> 
#> [[2]]
#> <z3_expr Bool> (not (<= 10 x))
#> 
```
