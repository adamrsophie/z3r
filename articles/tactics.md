# Tactics, goals, probes and simplifiers

``` r

library(z3r)
ctx <- z3_context()
x <- z3_int(ctx, "x")
y <- z3_int(ctx, "y")
```

A *goal* is a set of formulas. A *tactic* turns a goal into subgoals
that together are equivalent to it: simplifying, eliminating variables,
splitting cases or solving. Tactics are how Z3 builds its solvers; z3r
exposes them to inspect preprocessing, build custom strategies, or use
Z3 as a formula transformer.

## Applying tactics

``` r

g <- z3_goal(ctx)
z3_add(g, x == y + 1, y > 2, x < 10)
g
#> <z3_goal>
#> (goal
#>   (= x (+ y 1))
#>   (> y 2)
#>   (< x 10))
t <- z3_tactic_then(z3_tactic(ctx, "simplify"), z3_tactic(ctx, "solve-eqs"))
sub <- z3_apply_tactic(t, g)
sub[[1]]
#> <z3_goal>
#> (goal
#>   (not (<= x 3))
#>   (not (<= 10 x)))
```

`solve-eqs` eliminated `x`.
[`z3_goal_convert_model()`](https://adamrsophie.github.io/z3r/reference/z3_goal_info.md)
maps a model of the subgoal back to the original variables:

``` r

s <- z3_solver(ctx)
z3_add(s, z3_goal_formulas(sub[[1]]))
z3_check(s)
#> [1] "sat"
m <- z3_goal_convert_model(sub[[1]], z3_model(s))
z3_eval(m, x)
#> <z3_expr Int> 4
```

The available tactics:

``` r

tl <- z3_tactics(ctx)
nrow(tl)
#> [1] 123
head(tl[tl$name %in% c("simplify", "solve-eqs", "ctx-simplify", "nla2bv", "bit-blast", "smt"), ])
#>             name
#> 10        nla2bv
#> 49           smt
#> 82     bit-blast
#> 97     solve-eqs
#> 108 ctx-simplify
#> 116     simplify
#>                                                                                                                                                       description
#> 10  convert a nonlinear arithmetic problem into a bit-vector problem, in most cases the resultant goal is an under approximation and is useul for finding models.
#> 49                                                                                                                                  apply a SAT based SMT solver.
#> 82                                                                                                                        reduce bit-vector expressions into SAT.
#> 97                                                                                                                                           solve for variables.
#> 108                                                                                                                        apply contextual simplification rules.
#> 116                                                                                                                                   apply simplification rules.
```

## Combinators

``` r

fast <- z3_tactic_try_for(z3_tactic(ctx, "smt"), 1000)
robust <- z3_tactic_or_else(fast, z3_tactic(ctx, "skip"))
repeated <- z3_tactic_repeat(z3_tactic(ctx, "simplify"), 3)
parallel <- z3_tactic_par_or(z3_tactic(ctx, "smt"), z3_tactic(ctx, "qfnra-nlsat"))
tuned <- z3_tactic_with(z3_tactic(ctx, "simplify"), som = TRUE)
g2 <- z3_goal(ctx)
z3_add(g2, (x + 1) * (x + 1) > 0)
z3_goal_formulas(z3_apply_tactic(tuned, g2)[[1]])
#> [[1]]
#> <z3_expr Bool> (not (<= (+ (* 2 x) (* x x)) (- 1)))
```

## Probes

A probe measures a goal. Probes compare and combine with R operators and
drive conditional tactics:

``` r

z3_probe_apply(z3_probe(ctx, "num-consts"), g)
#> [1] 2
z3_probe_apply(z3_probe(ctx, "is-qflia"), g)
#> [1] 1
small <- z3_probe(ctx, "size") < 10
strategy <- z3_tactic_cond(small, z3_tactic(ctx, "smt"), z3_tactic(ctx, "simplify"))
z3_goal_status(z3_apply_tactic(strategy, g)[[1]])
#> [1] "sat"
head(z3_probes(ctx))
#>           name
#> 1 is-unbounded
#> 2  is-quasi-pb
#> 3   is-qffplra
#> 4    is-qffpbv
#> 5      is-qffp
#> 6   is-qfufnra
#>                                                                                     description
#> 1         true if the goal contains integer/real constants that do not have lower/upper bounds.
#> 2                                                                 true if the goal is quasi-pb.
#> 3                                                              true if the goal is in QF_FPLRA.
#> 4                   true if the goal is in QF_FPBV (floats+bit-vectors) with enumeration types.
#> 5                                 true if the goal is in QF_FP (floats) with enumeration types.
#> 6 true if the goal is QF_UFNRA (quantifier-free nonlinear real arithmetic with other theories).
```

## Solvers from tactics

``` r

s <- z3_solver_from_tactic(z3_tactic_then(z3_tactic(ctx, "simplify"), z3_tactic(ctx, "smt")))
z3_add(s, x * 2 + y == 7, x > y)
z3_check(s)
#> [1] "sat"
z3_model(s)
#> <z3_model>
#>   y = 1
#>   x = 3
```

## Simplifiers

Tactics are not incremental. Simplifiers (Z3 4.12.2 and later) are
preprocessing steps that are, so they can be attached to a solver used
with push and pop:

``` r

simp <- z3_simplifier_then(z3_simplifier(ctx, "solve-eqs"), z3_simplifier(ctx, "elim-unconstrained"))
s <- z3_solver_add_simplifier(z3_solver(ctx), simp)
z3_add(s, x == y + 2, y > 3)
z3_check(s)
#> [1] "sat"
z3_push(s)
z3_add(s, x < 6)
z3_check(s)
#> [1] "unsat"
z3_pop(s)
head(z3_simplifiers(ctx))
#>                name
#> 1               der
#> 2 distribute-forall
#> 3       reduce-args
#> 4 cofactor-term-ite
#> 5  bv-divrem-bounds
#> 6    euf-completion
#>                                                                                                              description
#> 1                                                                                       destructive equality resolution.
#> 2                                                                                   distribute forall over conjunctions.
#> 3 reduce the number of arguments of function applications, when for all occurrences of a function f the i-th is a value.
#> 4                                                                           eliminate term if-then-else using cofactors.
#> 5                                      add range lemmas for bit-vector division/remainder terms with a symbolic divisor.
#> 6                                                                                    simplify modulo congruence closure.
```

## Goal facts and DIMACS

``` r

z3_goal_info(g)
#> $size
#> [1] 3
#> 
#> $num_exprs
#> [1] 9
#> 
#> $depth
#> [1] 0
#> 
#> $precision
#> [1] "precise"
#> 
#> $inconsistent
#> [1] FALSE
p <- z3_bools(ctx, c("a", "b", "c"))
gb <- z3_goal(ctx)
z3_add(gb, p$a | p$b, !p$a | p$c, !p$c)
cat(z3_to_dimacs(gb))
#> p cnf 3 3
#> 1 2 0
#> -1 3 0
#> -3 0
#> c 3 c
#> c 2 b
#> c 1 a
```
