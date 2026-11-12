# Check satisfiability

Decides whether the assertions of a solver or optimizer, together with
the optional assumptions, are satisfiable.

## Usage

``` r
z3_check(x, ..., timeout = NULL)
```

## Arguments

- x:

  A `z3_solver` or `z3_optimize`.

- ...:

  Assumptions: Boolean expressions or lists of them.

- timeout:

  `NULL`, or a time limit in milliseconds (a positive whole number) for
  this check only; see the Timeouts section.

## Value

`"sat"`, `"unsat"` or `"unknown"`.

## Details

The result is one of three strings:

- `"sat"`: a model exists; retrieve it with
  [`z3_model()`](https://adamrsophie.github.io/z3r/reference/z3_model.md).

- `"unsat"`: no model exists;
  [`z3_unsat_core()`](https://adamrsophie.github.io/z3r/reference/z3_unsat_core.md)
  explains why when assumptions or tracked assertions were used.

- `"unknown"`: Z3 gave up, for example because of a timeout, a resource
  limit, or a theory (such as nonlinear integer arithmetic or
  quantifiers) for which Z3 has no complete decision procedure.
  `"unknown"` is not `"unsat"`; see
  [`z3_reason_unknown()`](https://adamrsophie.github.io/z3r/reference/z3_reason_unknown.md).

Assumptions are Boolean expressions that hold for this check only. They
are not added to the solver.

## Timeouts

`timeout` limits this check to that many milliseconds, replacing any
timeout of the solver or optimizer for this call only; a check that runs
out of time returns `"unknown"`. Afterwards z3r sets the object's
parameters back, also when the check fails or is interrupted: the
timeout set with
[`z3_set_params()`](https://adamrsophie.github.io/z3r/reference/z3_set_params.md)
returns, and without one the object gets the context's timeout as last
set through z3r (`z3_context(timeout = )` or
[`z3_update_param()`](https://adamrsophie.github.io/z3r/reference/z3_memory_usage.md)),
or none. Z3 cannot remove a parameter, so the object then keeps that
value as its own: a later change of the context's timeout, or a global
`timeout` set with
[`z3_global_param_set()`](https://adamrsophie.github.io/z3r/reference/z3_global_param_set.md),
no longer reaches it. Printing the object shows the parameters z3r has
set on it.

## Interrupting a check

While a check runs, Z3 installs its own handler for the interrupt signal
(Z3's `ctrl_c` parameter, on by default). Pressing Ctrl-C (or the stop
button of a front end that sends an interrupt signal) cancels the check
and leaves the solver usable. For a `z3_solver`, z3r then raises an
ordinary R interrupt, so `tryCatch(..., interrupt = )` works as for any
R computation. A `z3_optimize` reports the same reason (`"canceled"`)
for an interrupt, a timeout and a resource limit, so an interrupted
optimizer check returns `"unknown"` instead. Front ends that do not send
a signal (such as RGui on Windows) cannot interrupt Z3; set a `timeout`
or `rlimit` with
[`z3_set_params()`](https://adamrsophie.github.io/z3r/reference/z3_set_params.md)
there. R code running inside the check (a callback) can stop it with
[`z3_interrupt()`](https://adamrsophie.github.io/z3r/reference/z3_interrupt.md).

## Examples

``` r
ctx <- z3_context()
x <- z3_int(ctx, "x")
p <- z3_bool(ctx, "p")
s <- z3_solver(ctx)
z3_add(s, z3_implies(p, x > 10), x < 5)
z3_check(s)
#> [1] "sat"
z3_check(s, p)
#> [1] "unsat"
z3_unsat_core(s)
#> $p
#> <z3_expr Bool> p
#> 
z3_check(s, timeout = 5000)
#> [1] "sat"
```
