# SMT-LIB2 parsers with shared declarations

A parser (Z3 4.9.0 and later) keeps the sorts and functions declared in
earlier fragments, and accepts sorts and functions declared in R, so a
problem can be read in pieces. `z3_parser_add()` makes a sort or
function known to it; `z3_parse()` returns the assertions of a fragment.

## Usage

``` r
z3_parser(ctx)

z3_parser_add(parser, x)

z3_parse(parser, text)
```

## Arguments

- ctx:

  A
  [`z3_context()`](https://adamrsophie.github.io/z3r/reference/z3_context.md).

- parser:

  A `z3_parser`.

- x:

  A `z3_sort`, `z3_function` or `z3_func_decl`.

- text:

  SMT-LIB2 text.

## Value

`z3_parser()` returns a parser; `z3_parse()` a list of expressions;
`z3_parser_add()` the parser, invisibly.

## Examples

``` r
ctx <- z3_context()
if (z3_version_at_least("4.9.0")) {
  p <- z3_parser(ctx)
  f <- z3_function(ctx, "f", z3_int_sort(ctx), z3_int_sort(ctx))
  z3_parser_add(p, f)
  z3_parse(p, "(declare-const a Int) (assert (> (f a) 3))")
  z3_parse(p, "(assert (< a 0))")
}
#> [[1]]
#> <z3_expr Bool> (< a 0)
#> 
```
