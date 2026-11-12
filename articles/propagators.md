# User propagators and solver callbacks

``` r

library(z3r)
ctx <- z3_context()
```

A user propagator is R code that takes part in Z3’s search. Z3 reports
when registered terms receive values; the propagator answers with
consequences or conflicts. This implements constraints that are hard to
write as formulas, for example ones computed by an R function. User
propagators need Z3 4.12.3 or later.

## How the search talks to R

- `push(cb)` and `pop(cb, n)`: Z3 opens and closes backtracking scopes.
  The propagator must undo its own bookkeeping on `pop`.
- `fixed(cb, term, value)`: a registered Boolean or bit-vector term got
  a value.
- `eq(cb, a, b)`, `diseq(cb, a, b)`: registered terms became equal or
  different.
- `final(cb)`: a complete assignment exists; the last chance to object.
- `created(cb, term)`: a term built with a propagator function appeared.
- `decide(cb, term, index, phase)`: Z3 is about to branch.

Inside a callback, `z3_propagate_conflict(cb, fixed)` says that the
current values of `fixed` are inconsistent, and
`z3_propagate_consequence(cb, fixed, consequence)` adds a consequence
they imply.

## At most two of six

``` r

p <- z3_bools(ctx, paste0("p", 1:6))
s <- z3_solver(ctx)

true_terms <- list()   # terms currently assigned TRUE
marks <- integer()     # length of true_terms at each push

z3_propagator(s,
  push = function(cb) marks <<- c(marks, length(true_terms)),
  pop = function(cb, n) {
    keep <- marks[length(marks) - n + 1]
    true_terms <<- true_terms[seq_len(keep)]
    marks <<- marks[seq_len(length(marks) - n)]
  },
  fixed = function(cb, term, value) {
    if (isTRUE(z3_value(value))) {
      true_terms[[length(true_terms) + 1]] <<- term
      if (length(true_terms) > 2) z3_propagate_conflict(cb, true_terms)
    }
  })
for (v in p) z3_propagate_register(s, v)
z3_add(s, z3_or(p[1:3]), z3_or(p[4:6]))
z3_check(s)
#> [1] "sat"
unlist(z3_model_values(z3_model(s)))
#>    p2    p1    p3    p4    p6    p5 
#> FALSE  TRUE FALSE  TRUE FALSE FALSE
```

The propagator alone enforces the cardinality limit; Z3 learns from each
conflict it reports. Forcing three terms true makes the problem
unsatisfiable:

``` r

z3_add(s, p$p1, p$p2, p$p4)
z3_check(s)
#> [1] "unsat"
```

## Errors and safety

An error raised in a callback stops the check and is reported as an R
error afterwards, so R and Z3 stay consistent:

``` r

s <- z3_solver(ctx)
z3_propagator(s, push = function(cb) NULL, pop = function(cb, n) NULL,
              fixed = function(cb, term, value) stop("not allowed"))
z3_propagate_register(s, p$p1)
z3_add(s, p$p1)
z3_check(s)
#> Error:
#> ! error in the R callback 'fixed': not allowed (the check was interrupted)
```

The `cb` handle is valid only while its callback runs. While a check
runs, callbacks cannot use their own solver
([`z3_add()`](https://adamrsophie.github.io/z3r/reference/z3_add.md),
[`z3_push()`](https://adamrsophie.github.io/z3r/reference/z3_push.md)
and the rest fail); they act through `cb`. Z3 sometimes copies a solver
internally, as in parallel mode; the copy does not call R, so z3r
refuses a `"sat"` result from such a check, and
[`z3_translate()`](https://adamrsophie.github.io/z3r/reference/z3_translate.md)
refuses solvers with callbacks.

## Watching the search

[`z3_on_clause()`](https://adamrsophie.github.io/z3r/reference/z3_propagator.md)
sees every clause the solver asserts, learns or deletes:

``` r

x <- z3_bools(ctx, paste0("x", 1:6))
s <- z3_solver(ctx)
learned <- 0
z3_on_clause(s, function(proof_hint, deps, literals) learned <<- learned + 1)
z3_add(s, z3_atleast(x, k = 4), z3_atmost(x, k = 3))
z3_check(s)
#> [1] "unsat"
learned
#> [1] 54
```

[`z3_on_model()`](https://adamrsophie.github.io/z3r/reference/z3_propagator.md)
reports every improved model during optimization:

``` r

n <- z3_int(ctx, "n")
opt <- z3_optimize(ctx)
z3_add(opt, n >= 0, n <= 100, n %% 9 == 4)
z3_maximize(opt, n)
#> <z3_objective: maximize n>
improvements <- c()
z3_on_model(opt, function(m) improvements <<- c(improvements, z3_value(z3_eval(m, n))))
z3_check(opt)
#> [1] "sat"
improvements
#> [1]  4 94
```

From Z3 5.3.2,
[`z3_set_proof_checker()`](https://adamrsophie.github.io/z3r/reference/z3_propagator.md)
installs an R function that checks proofs produced by the SMT-LIB2
interpreter.
