# Z3's text for a list of terms

Formats expressions the way Z3 prints a vector of terms: `(ast-vector`,
then each term on its own indented lines, then a closing parenthesis.

## Usage

``` r
z3_format_terms(terms)
```

## Arguments

- terms:

  An expression, or a non-empty list of expressions of one context.

## Value

A single string.

## Examples

``` r
ctx <- z3_context()
x <- z3_int(ctx, "x")
cat(z3_format_terms(list(x + 1, x > 2)), "\n")
#> (ast-vector
#>   (+ x 1)
#>   (> x 2)) 
```
