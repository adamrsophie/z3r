# SMT solving from R with z3r

z3r connects R to [Z3](https://github.com/Z3Prover/z3), a satisfiability
modulo theories (SMT) solver. You describe a problem as logical
constraints over integers, exact reals, Booleans, bit-vectors, arrays,
strings and functions; Z3 either finds values that satisfy all of them,
proves that none exist, or reports that it could not decide.

z3r is an independent third-party package. It is not developed or
endorsed by Microsoft or by the Z3 team.

``` r

library(z3r)
z3_version()$runtime
#> [1] "5.4.1.0"
```

## What an SMT solver does

An SMT solver answers one question: *is there an assignment of values to
these variables that makes all of these formulas true?* The answer is

- `"sat"`: yes, and here is one (a *model*);
- `"unsat"`: no, provably not;
- `"unknown"`: Z3 gave up. This happens with resource limits, and with
  theories for which no complete algorithm exists (nonlinear integer
  arithmetic, quantifiers). `"unknown"` never means `"unsat"`.

This differs from tools R users know:

| Tool | Works with | Answers |
|----|----|----|
| [`optim()`](https://rdrr.io/r/stats/optim.html), `nloptr` | continuous functions, floating point | a local optimum, approximately |
| linear/integer programming (`lpSolve`, `ompr`) | linear constraints | a global optimum |
| computer algebra (SymPy, `symcompR`, `Ryacas`) | symbolic expressions | closed-form solutions, simplifications |
| SMT (Z3) | arbitrary logical combinations of constraints in many theories | a model, a proof of impossibility, or “unknown” |

An SMT solver is the right tool when the problem is *combinatorial* or
*logical*: disjunctions (“this job runs before that one, or after it”),
implications, integer and bit-level reasoning, and questions of the form
“does this hold for **all** inputs?”. Numbers are exact: integers have
unbounded size and reals are rationals or algebraic numbers, never
rounded floating point.

## A first problem

``` r

ctx <- z3_context()
x <- z3_int(ctx, "x")
y <- z3_int(ctx, "y")

s <- z3_solver(ctx)
z3_add(s, x + y == 10, x > 2 * y, y > 0)
z3_check(s)
#> [1] "sat"
m <- z3_model(s)
m
#> <z3_model>
#>   y = 1
#>   x = 9
z3_value(z3_eval(m, x))
#> [1] 9
```

The three objects are always the same: a **context** owns everything, a
**solver** collects constraints, and a **model** holds the values found
by the last satisfiable check.

Constraints are built with ordinary R operators. They do not compute
anything; they build formulas:

``` r

x + y == 10
#> <z3_expr Bool> (= (+ x y) 10)
x^2 + y^2 <= 25
#> <z3_expr Bool> (<= (+ (* x x) (* y y)) 25)
```

R’s `&&` and `||` cannot be overloaded, so use `&`, `|`, `!` or the
functions
[`z3_and()`](https://adamrsophie.github.io/z3r/reference/z3_and.md),
[`z3_or()`](https://adamrsophie.github.io/z3r/reference/z3_and.md),
[`z3_not()`](https://adamrsophie.github.io/z3r/reference/z3_and.md),
[`z3_implies()`](https://adamrsophie.github.io/z3r/reference/z3_and.md):

``` r

p <- z3_bool(ctx, "p")
z3_implies(p, x > 0) & (p | y < 0)
#> <z3_expr Bool> (and (=> p (> x 0)) (or p (< y 0)))
```

A formula is never silently converted to `TRUE` or `FALSE`, so mistakes
such as `if (x > 0)` are errors rather than wrong answers:

``` r

if (x > 0) "positive"
#> Error in `if (x > 0) ...`:
#> ! argument is not interpretable as logical
```

## Exact numbers

Z3 integers are unbounded and Z3 reals are exact, so z3r never passes
values through double precision unless asked to:

``` r

big <- z3_int_val(ctx, "123456789012345678901234567890")
z3_simplify(big * big)
#> <z3_expr Int> 15241578753238836750495351562536198787501905199875019052100
z3_real_val(ctx, "1/3") + z3_real_val(ctx, "1/6")
#> <z3_expr Real> (+ (/ 1.0 3.0) (/ 1.0 6.0))
z3_simplify(z3_real_val(ctx, "1/3") + z3_real_val(ctx, "1/6"))
#> <z3_expr Real> (/ 1.0 2.0)
```

R numbers mixed with formulas are converted by a documented rule (see
[`?z3_int_val`](https://adamrsophie.github.io/z3r/reference/z3_int_val.md)):
whole numbers become exact integers, and `0.1` becomes the rational
1/10, which is the number R prints, not the binary approximation R
stores. Whole numbers beyond 2^53 are refused, because R may already
have rounded them; pass such numbers as strings or
[`gmp::bigz`](https://rdrr.io/pkg/gmp/man/biginteger.html).

Converting results back to R is exact as well.
[`z3_value()`](https://adamrsophie.github.io/z3r/reference/z3_value.md)
returns a double only when the value is exactly representable, and
otherwise a `gmp` number (or a string when gmp is not installed):

``` r

s <- z3_solver(ctx)
r <- z3_real(ctx, "r")
z3_add(s, x == z3_int_val(ctx, "1180591620717411303424"), 3 * r == 1)
z3_check(s)
#> [1] "sat"
z3_model_values(z3_model(s))
#> $r
#> Big Rational ('bigq') :
#> [1] 1/3
#> 
#> $x
#> Big Integer ('bigz') :
#> [1] 1180591620717411303424
```

Division follows R: `/` is real division even for integers. `%/%` and
`%%` are Z3’s integer `div` and `mod`, which use the SMT-LIB (Euclidean)
convention. It agrees with R for positive divisors and differs for
negative ones:

``` r

z3_simplify(z3_int_val(ctx, 7) %/% -2)
#> <z3_expr Int> (- 3)
7 %/% -2
#> [1] -4
```

## Nonlinear problems and algebraic numbers

Z3 decides nonlinear real arithmetic exactly. Irrational solutions are
algebraic numbers:

``` r

s <- z3_solver(ctx)
z3_add(s, r * r == 2, r > 0)
z3_check(s)
#> [1] "sat"
root <- z3_eval(z3_model(s), r)
root
#> <z3_expr Real> (root-obj (+ (^ x 2) (- 2)) 2)
z3_decimal(root, 20)
#> [1] "1.41421356237309504880?"
z3_value(root, exact = FALSE)
#> [1] 1.414214
```

Nonlinear *integer* arithmetic is undecidable in general, so Z3 may
answer `"unknown"` or run for a long time. Limits keep checks bounded:

``` r

z <- z3_int(ctx, "z")
s <- z3_solver(ctx, timeout = 500)
z3_add(s, x > 0, y > 0, z > 0, x^3 + y^3 == z^3)
z3_check(s)
#> [1] "unknown"
z3_reason_unknown(s)
#> [1] "timeout"
```

Use `rlimit` instead of `timeout` for results that are reproducible
across machines. Ctrl-C also stops a running check.

## Proving things for all inputs

To prove that a formula is always true, ask whether its negation can be
satisfied. `"unsat"` is a proof.

``` r

a <- z3_int(ctx, "a")
b <- z3_int(ctx, "b")
s <- z3_solver(ctx)
z3_add(s, !(z3_abs(a + b) <= z3_abs(a) + z3_abs(b)))
z3_check(s)
#> [1] "unsat"
```

The triangle inequality holds for every pair of (unbounded) integers. No
amount of random testing gives that guarantee.

## A realistic example: scheduling with disjunctions

Three jobs share one machine. Each has a duration and a release time;
job 2 must finish before job 3 starts. We want the earliest time at
which all jobs are done. The “no overlap” requirement is a disjunction
for every pair, so the feasible region is not convex, and gradient-based
optimizers such as [`optim()`](https://rdrr.io/r/stats/optim.html) have
nothing to follow; integer programming needs big-M reformulations. For
an SMT optimizer it is the natural formulation.

``` r

dur <- c(4, 3, 2)
release <- c(0, 1, 5)
start <- z3_ints(ctx, paste0("start", 1:3))

opt <- z3_optimize(ctx)
for (i in 1:3) z3_add(opt, start[[i]] >= release[i])
for (i in 1:2) for (j in (i + 1):3) {
  z3_add(opt, start[[i]] + dur[i] <= start[[j]] | start[[j]] + dur[j] <= start[[i]])
}
z3_add(opt, start[[2]] + dur[2] <= start[[3]])
makespan <- z3_int(ctx, "makespan")
for (i in 1:3) z3_add(opt, makespan >= start[[i]] + dur[i])
obj <- z3_minimize(opt, makespan)

z3_check(opt)
#> [1] "sat"
z3_optimum(obj)
#> <z3_optimum> 9
unlist(z3_model_values(z3_model(opt))[paste0("start", 1:3)])
#> start1 start2 start3 
#>      0      4      7
```

The answer is optimal: Z3 proves no schedule finishes earlier.

## Incremental solving

A solver keeps its assertions between checks.
[`z3_push()`](https://adamrsophie.github.io/z3r/reference/z3_push.md)
and [`z3_pop()`](https://adamrsophie.github.io/z3r/reference/z3_push.md)
open and close scopes, which is how “what if” questions are asked
without rebuilding the problem:

``` r

s <- z3_solver(ctx)
z3_add(s, x > 0, x < 10)
z3_push(s)
z3_add(s, x > 20)
z3_check(s)
#> [1] "unsat"
z3_pop(s)
z3_check(s)
#> [1] "sat"
```

Assumptions hold for a single check only:

``` r

z3_check(s, x == 5)
#> [1] "sat"
z3_check(s, x == 50)
#> [1] "unsat"
```

A model is only handed out for the check that produced it. After the
solver changes,
[`z3_model()`](https://adamrsophie.github.io/z3r/reference/z3_model.md)
refuses until the next
[`z3_check()`](https://adamrsophie.github.io/z3r/reference/z3_check.md),
so a model never describes constraints it was not computed for.

## Explaining contradictions: unsatisfiable cores

Label constraints with
[`z3_assert_and_track()`](https://adamrsophie.github.io/z3r/reference/z3_assert_and_track.md).
When the problem is unsatisfiable,
[`z3_unsat_core()`](https://adamrsophie.github.io/z3r/reference/z3_unsat_core.md)
names a subset of the labels that is already contradictory:

``` r

s <- z3_solver(ctx)
z3_assert_and_track(s, x > 10, "at_least_11")
z3_assert_and_track(s, x < 5, "at_most_4")
z3_assert_and_track(s, x != 7, "not_seven")
z3_check(s)
#> [1] "unsat"
names(z3_unsat_core(s))
#> [1] "at_least_11" "at_most_4"
```

Cores are not guaranteed to be minimal.

## Optimization with soft constraints

Soft constraints are preferences with weights. The optimizer minimizes
the total weight of the preferences it has to give up:

``` r

want <- z3_bools(ctx, c("coffee", "tea", "juice"))
opt <- z3_optimize(ctx)
z3_add(opt, z3_or(!want$coffee, !want$tea))     # not both hot drinks
z3_add(opt, z3_or(!want$tea, !want$juice))
z3_add_soft(opt, want$coffee, 2)
z3_add_soft(opt, want$tea, 3)
z3_add_soft(opt, want$juice, 2)
z3_check(opt)
#> [1] "sat"
unlist(z3_model_values(z3_model(opt)))
#>  juice coffee    tea 
#>   TRUE   TRUE  FALSE
```

Optimal values are not always attained or finite;
[`z3_optimum()`](https://adamrsophie.github.io/z3r/reference/z3_optimum.md)
reports `"unbounded"` and `"not attained"` (an infimum or supremum
approached with an infinitesimal `epsilon`) instead of inventing a
number:

``` r

opt <- z3_optimize(ctx)
z3_add(opt, r < 3)
z3_optimum({
  h <- z3_maximize(opt, r)
  z3_check(opt)
  h
})
#> <z3_optimum> 3 (supremum, not attained)
```

## Where to go next

Each part of z3r has its own article:

| Article | Covers |
|----|----|
| [`vignette("numbers")`](https://adamrsophie.github.io/z3r/articles/numbers.md) | Sorts, exact numerals, conversion rules, operator semantics |
| [`vignette("solving")`](https://adamrsophie.github.io/z3r/articles/solving.md) | Models, incremental solving, cores, proofs, consequences, validity |
| [`vignette("optimization")`](https://adamrsophie.github.io/z3r/articles/optimization.md) | Objectives, unbounded and unattained optima, MaxSAT, scheduling |
| [`vignette("logic")`](https://adamrsophie.github.io/z3r/articles/logic.md) | Propositional logic, puzzles (Sudoku, queens), pseudo-Boolean constraints |
| [`vignette("bitvectors")`](https://adamrsophie.github.io/z3r/articles/bitvectors.md) | Machine integers, overflow, proving bit tricks correct |
| [`vignette("floating-point")`](https://adamrsophie.github.io/z3r/articles/floating-point.md) | IEEE 754 arithmetic, rounding, finding numerical surprises |
| [`vignette("arrays-sets")`](https://adamrsophie.github.io/z3r/articles/arrays-sets.md) | Arrays, lambdas, sets |
| [`vignette("strings-regex")`](https://adamrsophie.github.io/z3r/articles/strings-regex.md) | Strings, sequences, regular expressions, characters |
| [`vignette("datatypes")`](https://adamrsophie.github.io/z3r/articles/datatypes.md) | Enumerations, records, lists, trees, polymorphic datatypes |
| [`vignette("functions-quantifiers")`](https://adamrsophie.github.io/z3r/articles/functions-quantifiers.md) | Uninterpreted and recursive functions, quantifiers, relations |
| [`vignette("tactics")`](https://adamrsophie.github.io/z3r/articles/tactics.md) | Goals, tactics, probes, simplifiers |
| [`vignette("horn")`](https://adamrsophie.github.io/z3r/articles/horn.md) | Constrained Horn clauses, program verification, Datalog |
| [`vignette("algebraic")`](https://adamrsophie.github.io/z3r/articles/algebraic.md) | Algebraic numbers, real closed fields, pi and infinitesimals |
| [`vignette("smtlib-inspection")`](https://adamrsophie.github.io/z3r/articles/smtlib-inspection.md) | SMT-LIB2, walking expressions, substitution, translation |
| [`vignette("propagators")`](https://adamrsophie.github.io/z3r/articles/propagators.md) | User propagators and solver callbacks |
| [`vignette("z3-5-theories")`](https://adamrsophie.github.io/z3r/articles/z3-5-theories.md) | Complex numbers, finite fields and finite sets of Z3 5 |
| [`vignette("symcompr")`](https://adamrsophie.github.io/z3r/articles/symcompr.md) | Exchanging expressions with the symcompR computer algebra system |

The [Z3 guide](https://microsoft.github.io/z3guide/) explains the
underlying theories in depth; every SMT-LIB2 example there runs from R
through
[`z3_eval_smtlib()`](https://adamrsophie.github.io/z3r/reference/z3_to_smtlib.md).
