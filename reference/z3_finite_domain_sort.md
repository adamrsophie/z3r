# Finite domain sorts

A sort with `size` elements, numbered from 0, used by the Datalog engine
for relations over finite domains. R whole numbers combined with such
expressions become elements, as in `r(1, 2)` or `z3_val(ctx, 3, S)`.
`z3_finite_domain_size()` reads the size back.

## Usage

``` r
z3_finite_domain_sort(ctx, name, size)

z3_finite_domain_size(sort)
```

## Arguments

- ctx:

  A
  [`z3_context()`](https://adamrsophie.github.io/z3r/reference/z3_context.md).

- name:

  Sort name.

- size:

  Number of elements: a whole number up to 2^53, or a string of digits
  for larger sizes (below 2^64).

- sort:

  A finite domain sort.

## Value

A `z3_sort`; for `z3_finite_domain_size()` a number, or a string when
the size exceeds 2^53.

## Examples

``` r
ctx <- z3_context()
S <- z3_finite_domain_sort(ctx, "S", 10)
z3_finite_domain_size(S)
#> [1] 10
```
