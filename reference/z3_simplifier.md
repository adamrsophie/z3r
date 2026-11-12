# Simplifiers

Simplifiers (Z3 4.12.2 and later) are preprocessing steps that, unlike
tactics, work with incremental solving. `z3_simplifiers()` lists them;
`z3_simplifier_then()` chains two; `z3_simplifier_with()` fixes
parameters; `z3_solver_add_simplifier()` returns a new solver that
preprocesses its assertions with the simplifier.

## Usage

``` r
z3_simplifier(ctx, name)

z3_simplifiers(ctx)

z3_simplifier_then(s1, s2)

z3_simplifier_with(s, ...)

z3_solver_add_simplifier(solver, s)
```

## Arguments

- ctx:

  A
  [`z3_context()`](https://adamrsophie.github.io/z3r/reference/z3_context.md).

- name:

  Simplifier name, such as `"solve-eqs"` or `"elim-unconstrained"`.

- s1, s2, s:

  `z3_simplifier` objects.

- ...:

  Named parameters.

- solver:

  A `z3_solver`.

## Value

A `z3_simplifier`, a `z3_solver`, or a data frame.

## Examples

``` r
ctx <- z3_context()
if (z3_version_at_least("4.12.2")) {
  simp <- z3_simplifier_then(z3_simplifier(ctx, "solve-eqs"),
                             z3_simplifier(ctx, "elim-unconstrained"))
  s <- z3_solver_add_simplifier(z3_solver(ctx), simp)
  x <- z3_int(ctx, "x")
  z3_add(s, x == 2)
  z3_check(s)
}
#> [1] "sat"
```
