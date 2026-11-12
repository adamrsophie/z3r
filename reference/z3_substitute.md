# Substitution

- `z3_substitute(x, from, to)` replaces every occurrence of the
  expressions in `from` by the corresponding expressions in `to`, all at
  once.

- `z3_substitute_vars(x, to)` replaces bound variable `i` (a de Bruijn
  index, see
  [`z3_bound_var()`](https://adamrsophie.github.io/z3r/reference/z3_inspect.md))
  by `to[[i + 1]]`.

- `z3_substitute_funs(x, from, to)` replaces applications of the
  functions in `from` by the bodies in `to`, in which bound variable `i`
  stands for argument `i + 1`. Needs Z3 4.9.0 or later.

## Usage

``` r
z3_substitute(x, from, to)

z3_substitute_vars(x, to)

z3_substitute_funs(x, from, to)
```

## Arguments

- x:

  A `z3_expr`.

- from:

  Expressions (or functions for `z3_substitute_funs()`) to replace: a
  single object or a list.

- to:

  Replacements: a single object or a list of the same length.

## Value

A `z3_expr`.

## Examples

``` r
ctx <- z3_context()
x <- z3_int(ctx, "x")
y <- z3_int(ctx, "y")
z3_substitute(x + y, x, 3)
#> <z3_expr Int> (+ 3 y)
z3_simplify(z3_substitute(x * y, list(x, y), list(2, 5)))
#> <z3_expr Int> 10
f <- z3_function(ctx, "f", z3_int_sort(ctx), z3_int_sort(ctx))
v0 <- z3_bound_var(ctx, 0, z3_int_sort(ctx))
if (z3_version_at_least("4.9.0")) z3_substitute_funs(f(x) + f(1), f, v0 * v0)
#> <z3_expr Int> (+ (* x x) (* 1 1))
```
