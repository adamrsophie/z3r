# Solving: models, incremental solving, cores and proofs

``` r

library(z3r)
ctx <- z3_context()
x <- z3_int(ctx, "x")
y <- z3_int(ctx, "y")
```

## The three answers

``` r

s <- z3_solver(ctx)
z3_add(s, x > 0, x < 3)
z3_check(s)
#> [1] "sat"

s <- z3_solver(ctx)
z3_add(s, x > 0, x < 1)
z3_check(s)
#> [1] "unsat"
```

The third answer, `"unknown"`, means Z3 stopped without deciding. It is
never a disguised `"unsat"`:

``` r

z <- z3_int(ctx, "z")
s <- z3_solver(ctx, timeout = 300)
z3_add(s, x > 0, y > 0, z > 0, x^3 + y^3 == z^3)
z3_check(s)
#> [1] "unknown"
z3_reason_unknown(s)
#> [1] "timeout"
```

Use `timeout` (milliseconds) or the deterministic `rlimit` to bound a
check; Ctrl-C also interrupts one. `z3_param_help(s)` lists every solver
parameter and
[`z3_set_params()`](https://adamrsophie.github.io/z3r/reference/z3_set_params.md)
sets them with type checking:

``` r

z3_set_params(s, random_seed = 7, timeout = 1000)
z3_set_params(s, timeout = "soon")
#> Error:
#> ! parameter 'timeout' must be a non-negative whole number
```

## Models

``` r

s <- z3_solver(ctx)
z3_add(s, x + y == 10, x > 2 * y, y > 0)
z3_check(s)
#> [1] "sat"
m <- z3_model(s)
m
#> <z3_model>
#>   y = 1
#>   x = 9
z3_eval(m, x * y)
#> <z3_expr Int> 9
z3_model_values(m)
#> $y
#> [1] 1
#> 
#> $x
#> [1] 9
```

[`z3_eval()`](https://adamrsophie.github.io/z3r/reference/z3_eval.md)
leaves constants the model does not mention symbolic, unless asked to
complete the model:

``` r

w <- z3_int(ctx, "w")
z3_eval(m, x + w)
#> <z3_expr Int> (+ 9 w)
z3_eval(m, x + w, completion = TRUE)
#> <z3_expr Int> 9
```

A model belongs to one check. When the solver changes, z3r refuses to
hand out a model until the next check, so a model never describes
constraints it was not computed for:

``` r

z3_add(s, x > 100)
z3_model(s)
#> Error:
#> ! the solver has changed since the last z3_check(); call z3_check() again before requesting a model
```

The model already retrieved stays valid as a snapshot.

Models can also be built by hand, to evaluate formulas under chosen
values:

``` r

m2 <- z3_model(ctx)
z3_model_set(m2, x, 4)
z3_model_set(m2, y, 7)
z3_eval(m2, x * y + 1)
#> <z3_expr Int> 29
```

## Incremental solving

Push and pop scopes to explore variations without rebuilding the
problem:

``` r

s <- z3_solver(ctx)
z3_add(s, x >= 0, x <= 10)
z3_push(s)
z3_add(s, x > 20)
z3_check(s)
#> [1] "unsat"
z3_pop(s)
z3_check(s)
#> [1] "sat"
z3_num_scopes(s)
#> [1] 0
```

Assumptions hold for one check only:

``` r

z3_check(s, x == 3)
#> [1] "sat"
z3_check(s, x == 30)
#> [1] "unsat"
z3_assertions(s)
#> [[1]]
#> <z3_expr Bool> (>= x 0)
#> 
#> [[2]]
#> <z3_expr Bool> (<= x 10)
```

[`z3_with_scope()`](https://adamrsophie.github.io/z3r/reference/z3_with_scope.md)
pops for you, even when the code inside fails:

``` r

z3_with_scope(s, {
  z3_add(s, x > 20)
  z3_check(s)
})
#> [1] "unsat"
z3_num_scopes(s)
#> [1] 0
```

## Enumerating solutions

[`z3_all_models()`](https://adamrsophie.github.io/z3r/reference/z3_with_scope.md)
lists distinct assignments of some variables by blocking each one found.
The `"status"` attribute says whether all were found (`"unsat"`), the
limit was reached, or Z3 gave up:

``` r

s <- z3_solver(ctx)
z3_add(s, x >= 0, y >= 0, x + 2 * y == 6)
sols <- z3_all_models(s, list(x, y))
do.call(rbind, lapply(sols, as.data.frame))
#>   x y
#> 1 0 3
#> 2 2 2
#> 3 4 1
#> 4 6 0
attr(sols, "status")
#> [1] "unsat"
```

## Unsatisfiable cores

``` r

s <- z3_solver(ctx)
z3_assert_and_track(s, x > 10, "x_big")
z3_assert_and_track(s, x < 5, "x_small")
z3_assert_and_track(s, y == 2, "y_two")
z3_check(s)
#> [1] "unsat"
names(z3_unsat_core(s))
#> [1] "x_big"   "x_small"
```

The core names a subset of the labelled constraints that is already
contradictory; `y_two` is not in it. Cores are not guaranteed minimal.
The same works with assumptions:

``` r

a <- z3_bools(ctx, c("a1", "a2", "a3"))
s <- z3_solver(ctx)
z3_add(s, z3_implies(a$a1, x > 5), z3_implies(a$a2, x < 3), z3_implies(a$a3, y == 1))
z3_check(s, a$a1, a$a2, a$a3)
#> [1] "unsat"
names(z3_unsat_core(s))
#> [1] "a1" "a2"
```

## Proofs

A context created with `proof = TRUE` records proofs of
unsatisfiability:

``` r

pctx <- z3_context(proof = TRUE)
p <- z3_bool(pctx, "p")
q <- z3_bool(pctx, "q")
s <- z3_solver(pctx)
z3_add(s, z3_implies(p, q), p, !q)
z3_check(s)
#> [1] "unsat"
substr(format(z3_proof(s)), 1, 200)
#> [1] "(mp (mp (asserted (=> p q))\n        (rewrite (= (=> p q) (or q (not p))))\n        (or q (not p)))\n    (trans (monotonicity (iff-false (asserted (not q)) (= q false))\n                         (monotoni"
```

## Consequences, cubes and the trail

[`z3_consequences()`](https://adamrsophie.github.io/z3r/reference/z3_proof.md)
reports which literals over chosen variables follow from the assertions
and assumptions:

``` r

p <- z3_bools(ctx, c("p", "q", "r"))
s <- z3_solver(ctx)
z3_add(s, z3_implies(p$p, p$q), z3_implies(p$q, p$r))
z3_consequences(s, list(p$p), list(p$q, p$r))$consequences
#> [[1]]
#> <z3_expr Bool> (=> p q)
#> 
#> [[2]]
#> <z3_expr Bool> (=> p r)
```

[`z3_cube()`](https://adamrsophie.github.io/z3r/reference/z3_proof.md)
splits the search space for cube-and-conquer parallelism, and a simple
solver (`z3_solver(ctx, simple = TRUE)`) exposes its decision trail:

``` r

s <- z3_solver(ctx, simple = TRUE)
z3_add(s, p$p, z3_or(p$q, p$r))
z3_check(s)
#> [1] "sat"
z3_trail(s)
#> $literals
#> $literals[[1]]
#> <z3_expr Bool> p
#> 
#> $literals[[2]]
#> <z3_expr Bool> poly!36
#> 
#> $literals[[3]]
#> <z3_expr Bool> (not r)
#> 
#> $literals[[4]]
#> <z3_expr Bool> q
#> 
#> 
#> $levels
#> [1] 0 1 2 2
```

## Validity: proving instead of finding

A formula is valid (true for every value) exactly when its negation is
unsatisfiable:

``` r

prove <- function(ctx, formula) {
  s <- z3_solver(ctx)
  z3_add(s, !formula)
  switch(z3_check(s), unsat = "valid", sat = "not valid", unknown = "unknown")
}
prove(ctx, (x + y)^2 == x^2 + 2 * x * y + y^2)
#> [1] "valid"
prove(ctx, (x + y)^2 == x^2 + y^2)
#> [1] "not valid"
```

## Logic-specific and tactic-based solvers

``` r

s <- z3_solver(ctx, logic = "QF_LIA")
z3_add(s, 3 * x + 2 * y == 17, x >= 0, y >= 0)
z3_check(s)
#> [1] "sat"
head(unlist(z3_statistics(s)), 6)
#>        propagations binary propagations        final checks           added eqs 
#>                   2                   2                   1                   3 
#>           mk clause    mk clause binary 
#>                   1                   3
```

[`z3_statistics_text()`](https://adamrsophie.github.io/z3r/reference/z3_statistics.md)
gives the same statistics as Z3 prints them.

## Time limits and interruption

`timeout` limits one check to that many milliseconds; a check that runs
out of time returns `"unknown"`, and the solver keeps its own settings
for the next check:

``` r

s <- z3_solver(ctx)
z3_add(s, x * x * x + y * y * y == 33, x > 0)
r <- z3_check(s, timeout = 50)
r
#> [1] "unknown"
if (r == "unknown") z3_reason_unknown(s)
#> [1] "timeout"
```

Inside a callback
([`vignette("propagators")`](https://adamrsophie.github.io/z3r/articles/propagators.md)),
`z3_interrupt(s)` stops the running check, which then returns
`"unknown"` with the reason `"interrupted"`.

[`vignette("tactics")`](https://adamrsophie.github.io/z3r/articles/tactics.md)
shows solvers built from tactics, and
[`vignette("propagators")`](https://adamrsophie.github.io/z3r/articles/propagators.md)
shows how R code can join the search.
