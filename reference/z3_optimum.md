# Optimal value of an objective

Returns the optimum found by the last
[`z3_check()`](https://adamrsophie.github.io/z3r/reference/z3_check.md)
of the objective's optimizer that returned `"sat"`, or the best value
found so far by one that returned `"unknown"`.

## Usage

``` r
z3_optimum(objective)
```

## Arguments

- objective:

  A `z3_objective` from
  [`z3_minimize()`](https://adamrsophie.github.io/z3r/reference/z3_minimize.md),
  [`z3_maximize()`](https://adamrsophie.github.io/z3r/reference/z3_minimize.md)
  or
  [`z3_add_soft()`](https://adamrsophie.github.io/z3r/reference/z3_add_soft.md).

## Value

A list of class `z3_optimum` with elements `status`, `value` (a
`z3_expr` or `NULL`), `lower` and `upper` (bound text from Z3, or `NA`).

## Details

An optimum is not always a finite, attained number:

- `"optimal"`: the bound is finite and attained by the model.

- `"unbounded"`: the objective can grow (maximize) or shrink (minimize)
  without limit; `value` is `NULL`.

- `"not attained"`: the objective approaches `value` but never reaches
  it, as when maximizing `x` subject to `x < 3` over the reals. Z3
  represents this as `value - epsilon` or `value + epsilon`. Z3 4.8.13
  gets this wrong: it reports such an objective as attained at a smaller
  value (2 for `x < 3`), as its own command line tool does; Z3 4.10.2
  and later report the supremum.

- `"unknown"`: the check stopped early, for example at a timeout, and
  returned `"unknown"`. `value` is the objective's value in the best
  model Z3 had found, which is not proven optimal; it is `NULL` when Z3
  had no model that satisfies the hard constraints and the check's
  assumptions. `lower` and `upper` are the bounds known when it stopped.
  Before Z3 finds its first model these can be trivial bounds, such as
  the total weight of the soft constraints. When Z3 stopped before
  setting up the objective it reports zero for both bounds; z3r then
  returns `NA` for both, which also happens, rarely, when the bound is
  exactly zero.

Over nonlinear arithmetic the result can be non-optimal; Z3 does not
guarantee optimality there.

## Examples

``` r
ctx <- z3_context()
x <- z3_int(ctx, "x")
y <- z3_int(ctx, "y")
opt <- z3_optimize(ctx)
z3_add(opt, x >= 0, y >= 0, x + 2 * y <= 14, 3 * x - y >= 0, x - y <= 2)
obj <- z3_maximize(opt, 3 * x + 4 * y)
z3_check(opt)
#> [1] "sat"
z3_optimum(obj)
#> <z3_optimum> 34
z3_model(opt)
#> <z3_model>
#>   y = 4
#>   x = 6
```
