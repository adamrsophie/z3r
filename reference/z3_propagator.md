# User propagators

A user propagator lets R code take part in Z3's search: Z3 tells it when
registered terms get fixed values or become equal, and it can respond by
propagating consequences or reporting conflicts. This implements custom
theories and constraints that are awkward to state as formulas. Needs Z3
4.12.3 or later.

## Usage

``` r
z3_propagator(
  solver,
  push,
  pop,
  fixed = NULL,
  eq = NULL,
  diseq = NULL,
  final = NULL,
  created = NULL,
  decide = NULL,
  binding = NULL
)

z3_propagate_register(solver, e)

z3_propagator_function(ctx, name, domain, range)

z3_propagate_consequence(
  cb,
  fixed = list(),
  consequence,
  lhs = list(),
  rhs = list()
)

z3_propagate_conflict(cb, fixed = list(), lhs = list(), rhs = list())

z3_propagate_register_cb(cb, e)

z3_next_split(cb, term, index = 0, phase = NA)

z3_on_clause(solver, fn)

z3_on_model(optimizer, fn)

z3_set_proof_checker(ctx, fn)
```

## Arguments

- solver:

  A `z3_solver`.

- push, pop, eq, diseq, final, created, decide, binding:

  Callback functions (see Details).

- fixed:

  For `z3_propagator()`, the callback for fixed values (see Details);
  for `z3_propagate_consequence()` and `z3_propagate_conflict()`, a list
  of fixed terms justifying the consequence.

- ctx:

  A
  [`z3_context()`](https://adamrsophie.github.io/z3r/reference/z3_context.md).

- name, domain, range:

  Name and signature of a propagator function.

- cb:

  The callback handle passed to a callback.

- consequence:

  A Boolean expression.

- lhs, rhs:

  Lists of terms whose equalities justify a consequence.

- term, e:

  An expression (Bool or bit-vector for registration).

- index:

  Bit index for bit-vector splits.

- phase:

  `TRUE`, `FALSE` or `NA` (let Z3 choose).

- fn:

  A callback function.

- optimizer:

  A `z3_optimize`.

## Value

`z3_propagator()`, `z3_on_clause()`, `z3_on_model()` and
`z3_propagate_register()` return their first argument invisibly;
`z3_propagator_function()` returns a `z3_function`; the in-callback
functions return `TRUE` when Z3 accepted the request.

## Details

`z3_propagator(solver, ...)` installs the callbacks, given as named
functions. `push(cb)` and `pop(cb, n)` are required: Z3 opens and closes
backtracking scopes, and the propagator must undo its own state on
`pop`. The optional callbacks are `fixed(cb, term, value)` (a registered
term got a value), `eq(cb, a, b)` and `diseq(cb, a, b)` (registered
terms became equal or different), `final(cb)` (a full assignment exists;
the last chance to object), `created(cb, term)` (a term using a function
from `z3_propagator_function()` appeared),
`decide(cb, term, index, phase)` (Z3 is about to split on a term) and
`binding(cb, quantifier, instance)` (Z3 4.15.3; return `FALSE` to block
a quantifier instance).

Terms are registered before the check with `z3_propagate_register()`, or
inside a callback with `z3_propagate_register_cb()`. Inside a callback,
`cb` is a handle valid only until the callback returns:

- `z3_propagate_consequence(cb, fixed, consequence, lhs, rhs)` states
  that `consequence` follows from the values of the `fixed` terms and
  the equalities `lhs[[i]] == rhs[[i]]`. With `consequence = FALSE` it
  is a conflict, and Z3 backtracks.

- `z3_propagate_conflict(cb, fixed)` is the same with `FALSE`.

- `z3_next_split(cb, term, index, phase)` suggests the next decision.

An error in a callback interrupts the check and is raised as an R error
(an interrupt with Ctrl-C is raised as an interrupt). While a check
runs, its solver cannot be used from its own callbacks:
[`z3_add()`](https://adamrsophie.github.io/z3r/reference/z3_add.md),
[`z3_push()`](https://adamrsophie.github.io/z3r/reference/z3_push.md),
[`z3_check()`](https://adamrsophie.github.io/z3r/reference/z3_check.md)
and every other function on it fail; act through the callback handle
instead.

Z3 cannot copy R callbacks.
[`z3_translate()`](https://adamrsophie.github.io/z3r/reference/z3_translate.md)
and
[`z3_solver_add_simplifier()`](https://adamrsophie.github.io/z3r/reference/z3_simplifier.md)
refuse solvers that have them, and when Z3 copies a solver internally
during a check (as in parallel mode), z3r refuses a `"sat"` result,
which the copy may have found without the propagator's constraints.
[`z3_reset()`](https://adamrsophie.github.io/z3r/reference/z3_push.md)
removes a solver's callbacks; install them again afterwards.

`z3_on_clause(solver, fn)` registers `fn(proof_hint, deps, literals)`,
called for every clause the solver asserts, infers or deletes (Z3
4.12.3). `z3_on_model(optimizer, fn)` registers `fn(model)`, called with
each improved model during optimization. `z3_set_proof_checker(ctx, fn)`
(Z3 5.3.2) registers `fn(proof, assertions)`, called by the SMT-LIB2
interpreter
([`z3_eval_smtlib()`](https://adamrsophie.github.io/z3r/reference/z3_to_smtlib.md))
after unsatisfiable checks when proofs are enabled; it returns a report
string, or `NULL` for an empty report, and an R error in it is raised
when
[`z3_eval_smtlib()`](https://adamrsophie.github.io/z3r/reference/z3_to_smtlib.md)
returns. The checker stays registered while the context object exists;
pass `fn = NULL` to remove it. It cannot replace itself or call
[`z3_eval_smtlib()`](https://adamrsophie.github.io/z3r/reference/z3_to_smtlib.md)
on its own context.

Callback functions are owned by the solver, optimizer or context they
are registered with, so closures that refer back to that object do not
keep it alive.

## Examples

``` r
ctx <- z3_context()
if (z3_version_at_least("4.12.3")) {
  # A propagator that forbids more than two of four Booleans being true.
  p <- z3_bools(ctx, paste0("p", 1:4))
  s <- z3_solver(ctx)
  trail <- list()
  true_terms <- list()
  z3_propagator(s,
    push = function(cb) trail[[length(trail) + 1]] <<- length(true_terms),
    pop = function(cb, n) {
      keep <- trail[[length(trail) - n + 1]]
      true_terms <<- true_terms[seq_len(keep)]
      trail <<- trail[seq_len(length(trail) - n)]
    },
    fixed = function(cb, term, value) {
      if (isTRUE(z3_value(value))) {
        true_terms[[length(true_terms) + 1]] <<- term
        if (length(true_terms) > 2) z3_propagate_conflict(cb, true_terms)
      }
    })
  for (x in p) z3_propagate_register(s, x)
  z3_add(s, z3_or(p[1:2]), z3_or(p[3:4]))
  z3_check(s)
  sum(unlist(z3_model_values(z3_model(s))))
}
#> [1] 2
```
