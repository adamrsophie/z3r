# Probes and conditional tactics

A probe measures a goal and returns a number (Booleans are 1 or 0), such
as `"num-consts"`, `"size"` or `"is-qflia"`. `z3_probes()` lists them.
Probes compare and combine with `<`, `>`, `<=`, `>=`, `==`, `&`, `|` and
`!`; numbers are turned into constant probes.

## Usage

``` r
z3_probe(ctx, name)

z3_probes(ctx)

z3_probe_apply(p, goal)

z3_tactic_when(p, t)

z3_tactic_cond(p, t1, t2)

z3_tactic_fail_if(p)

z3_tactic_skip(ctx)

z3_tactic_fail(ctx)

z3_tactic_fail_if_not_decided(ctx)

z3_tactic_par_or(...)

z3_tactic_par_then(t1, t2)

z3_tactic_with(t, ...)

z3_tactic_params(t)
```

## Arguments

- ctx:

  A
  [`z3_context()`](https://adamrsophie.github.io/z3r/reference/z3_context.md).

- name:

  Probe name.

- p:

  A `z3_probe`.

- goal:

  A `z3_goal`.

- t, t1, t2:

  `z3_tactic` objects.

- ...:

  Tactics for `z3_tactic_par_or()`; named parameters for
  `z3_tactic_with()`.

## Value

A `z3_probe`, a `z3_tactic`, a number (`z3_probe_apply()`), or a data
frame (`z3_probes()`, `z3_tactic_params()`).

## Details

- `z3_tactic_when(p, t)` applies `t` when the probe is true and skips
  otherwise; `z3_tactic_cond(p, t1, t2)` chooses between two tactics;
  `z3_tactic_fail_if(p)` fails when the probe is true.

- `z3_tactic_skip()`, `z3_tactic_fail()` and
  `z3_tactic_fail_if_not_decided()` are the trivial tactics.

- `z3_tactic_par_or()` runs tactics in parallel and keeps the first to
  succeed; `z3_tactic_par_then(t1, t2)` applies `t2` to every subgoal of
  `t1` in parallel.

- `z3_tactic_with()` fixes parameters of a tactic; `z3_tactic_params()`
  lists them.

Parallel tactics run inside Z3 on Z3's own threads; R is not involved.

## Examples

``` r
ctx <- z3_context()
x <- z3_int(ctx, "x")
g <- z3_goal(ctx)
z3_add(g, x > 0, x < 10)
z3_probe_apply(z3_probe(ctx, "num-consts"), g)
#> [1] 1
small <- z3_probe(ctx, "size") < 5
t <- z3_tactic_cond(small, z3_tactic(ctx, "smt"), z3_tactic(ctx, "skip"))
z3_apply_tactic(t, g)
#> <z3_apply_result: 1 subgoal>
#> (goals
#> (goal)
#> )
```
