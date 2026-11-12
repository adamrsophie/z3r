# Simplify with parameters

[`z3_simplify()`](https://adamrsophie.github.io/z3r/reference/z3_simplify.md)
with named parameters calls Z3's configurable simplifier;
`z3_simplify_help()` lists its parameters (such as `som` for sum of
monomials, or `arith_lhs`).

## Usage

``` r
z3_simplify_help(ctx)
```

## Arguments

- ctx:

  A
  [`z3_context()`](https://adamrsophie.github.io/z3r/reference/z3_context.md).

## Value

A data frame of parameters.

## Examples

``` r
ctx <- z3_context()
x <- z3_int(ctx, "x")
z3_simplify((x + 1)^2, som = TRUE)
#> <z3_expr Int> (+ 1 (* 2 x) (* x x))
head(z3_simplify_help(ctx))
#>                name kind
#> 1   ite_extra_rules bool
#> 2              flat bool
#> 3       flat_and_or bool
#> 4 sort_disjunctions bool
#> 5          elim_and bool
#> 6          elim_ite bool
#>                                                                                                 description
#> 1 extra ite simplifications, these additional simplifications may reduce size locally but increase globally
#> 2                                             create nary applications for +,*,bvadd,bvmul,bvand,bvor,bvxor
#> 3                                                                       create nary applications for and,or
#> 4                                                                             sort subterms in disjunctions
#> 5                                                conjunctions are rewritten using negation and disjunctions
#> 6                                                                          eliminate ite in favor of and/or
```
