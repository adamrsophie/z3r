# Inspect the structure of an expression

Z3 expressions are trees. `z3_kind()` classifies a node:

## Usage

``` r
z3_kind(x)

z3_decl(x)

z3_args(x)

z3_num_args(x)

z3_decl_info(decl)

z3_is_const(x)

z3_is_numeral(x)

z3_is_app(x)

z3_is_quantifier(x)

z3_quantifier_info(x)

z3_var_index(x)

z3_bound_var(ctx, index, sort)

z3_apply(decl, ...)
```

## Arguments

- x:

  A `z3_expr`.

- decl:

  A `z3_func_decl` or `z3_function`.

- ctx:

  A
  [`z3_context()`](https://adamrsophie.github.io/z3r/reference/z3_context.md)
  (unused when `sort` is given).

- index:

  De Bruijn index of a bound variable.

- sort:

  Sort of the bound variable.

- ...:

  Arguments for `z3_apply()`: expressions, R values, or lists of them.

## Value

See Details.

## Details

- `"numeral"`: a number (Int, Real, bit-vector or floating point value);

- `"app"`: an application of a function declaration to arguments,
  including constants (no arguments) and built-in operators;

- `"var"`: a bound variable inside a quantifier body, identified by its
  de Bruijn index;

- `"quantifier"`: `forall`, `exists` or `lambda`.

`z3_decl()` returns the function declaration at the head of an
application (numerals included), `z3_args()` its arguments, and
`z3_decl_info()` describes a declaration: its name, kind (such as
`"add"`, `"le"`, `"and"`, `"bvadd"` or `"uninterpreted"`), domain, range
and indexed parameters (for example the bounds of an `extract`). Kinds
that only some Z3 versions define are reported as `"other"`.

`z3_quantifier_info()` returns the type, bound variable names and sorts,
body (with bound variables as `"var"` nodes), weight and patterns of a
quantifier. `z3_var_index()` gives a bound variable's de Bruijn index;
`z3_bound_var()` creates one, for building bodies for
[`z3_substitute_vars()`](https://adamrsophie.github.io/z3r/reference/z3_substitute.md)
and
[`z3_substitute_funs()`](https://adamrsophie.github.io/z3r/reference/z3_substitute.md).

## Examples

``` r
ctx <- z3_context()
x <- z3_int(ctx, "x")
e <- x + 2 * x^2
z3_kind(e)
#> [1] "app"
z3_decl_info(z3_decl(e))$kind
#> [1] "add"
z3_args(e)
#> [[1]]
#> <z3_expr Int> x
#> 
#> [[2]]
#> <z3_expr Int> (* 2 x x)
#> 
z3_kind(z3_int_val(ctx, 3))
#> [1] "numeral"
q <- z3_forall(x, x * 0 == 0)
z3_quantifier_info(q)$names
#> [1] "x"
```
