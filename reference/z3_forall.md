# Quantifiers

`z3_forall(vars, body)` states that `body` holds for all values of the
bound variables; `z3_exists()` states that it holds for some. The bound
variables are ordinary constants (made with
[`z3_int()`](https://adamrsophie.github.io/z3r/reference/z3_const.md)
and friends); inside the quantifier they are bound, and occurrences of
the same constants outside the quantifier are unaffected.

## Usage

``` r
z3_forall(
  vars,
  body,
  patterns = list(),
  no_patterns = list(),
  weight = 0,
  qid = "",
  skid = ""
)

z3_exists(
  vars,
  body,
  patterns = list(),
  no_patterns = list(),
  weight = 0,
  qid = "",
  skid = ""
)

z3_quantifier_db(
  sorts,
  names,
  body,
  type = c("forall", "exists", "lambda"),
  patterns = list(),
  no_patterns = list(),
  weight = 0,
  qid = "",
  skid = ""
)

z3_pattern(...)
```

## Arguments

- vars:

  A constant or a list of constants to bind.

- body:

  A Boolean expression (any expression for a lambda).

- patterns:

  Optional list of patterns; each pattern is a list of terms or a
  `z3_pattern`. A single `z3_pattern` is also accepted.

- no_patterns:

  Optional list of terms that must not be used as patterns.

- weight:

  Instantiation weight; higher weights make the quantifier less eagerly
  instantiated.

- qid, skid:

  Optional quantifier identifier (shown in statistics and traces) and
  prefix for Skolem constants.

- sorts:

  A sort or a list of sorts of the bound variables.

- names:

  Names of the bound variables, one per sort.

- type:

  `"forall"`, `"exists"` or `"lambda"`.

- ...:

  Terms of the pattern: expressions or lists of them.

## Value

A Boolean `z3_expr`; an array `z3_expr` for a lambda. A `z3_pattern` for
`z3_pattern()`.

## Details

Quantified formulas are outside the decidable fragments Z3 handles
completely, so checks may return `"unknown"`. Patterns (also called
triggers) guide Z3's instantiation heuristics: each pattern is a list of
terms that mention all bound variables. `z3_pattern()` makes one pattern
from its terms; it prints in Z3's syntax. A pattern cannot be combined
with `no_patterns`.

`z3_quantifier_db()` binds de Bruijn variables, made with
[`z3_bound_var()`](https://adamrsophie.github.io/z3r/reference/z3_inspect.md),
instead of constants, as Z3 does internally. The variables are described
by `sorts` and `names`: index 0 refers to the last of them, index 1 to
the one before it, and so on. Higher indices refer to enclosing binders,
so quantifiers can be nested. The body (and any pattern) uses the
variables directly. With `type = "lambda"` the result is an array, as
from
[`z3_lambda()`](https://adamrsophie.github.io/z3r/reference/z3_array.md).

## Examples

``` r
ctx <- z3_context()
I <- z3_int_sort(ctx)
f <- z3_function(ctx, "f", I, I)
x <- z3_int(ctx, "x")
s <- z3_solver(ctx)
z3_add(s, z3_forall(x, f(x) >= x), f(3) < 3)
z3_check(s)
#> [1] "unsat"

p <- z3_pattern(f(x))
p
#> <z3_pattern> ((f x))
z3_forall(x, f(x) > 0, patterns = p)
#> <z3_expr Bool> (forall ((x Int)) (! (> (f x) 0) :pattern ((f x)) :weight 0))

# The same quantifier over a de Bruijn variable
v0 <- z3_bound_var(ctx, 0, I)
z3_quantifier_db(I, "x", f(v0) > 0, patterns = z3_pattern(f(v0)))
#> <z3_expr Bool> (forall ((x Int)) (! (> (f x) 0) :pattern ((f x)) :weight 0))
sq <- z3_quantifier_db(I, "i", v0 * v0, type = "lambda")
z3_simplify(z3_select(sq, 7))
#> <z3_expr Int> 49
```
