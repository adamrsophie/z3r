# Sets

Sets are arrays from an element sort to Bool. `z3_set_sort()` builds the
sort; `z3_empty_set()` and `z3_full_set()` the extremes; `z3_set_add()`,
`z3_set_del()`, `z3_set_union()`, `z3_set_intersect()`,
`z3_set_difference()` and `z3_set_complement()` combine sets;
`z3_set_member()` and `z3_set_subset()` are tests.

## Usage

``` r
z3_set_sort(element)

z3_empty_set(element)

z3_full_set(element)

z3_set_add(s, x)

z3_set_del(s, x)

z3_set_union(...)

z3_set_intersect(...)

z3_set_difference(s, t)

z3_set_complement(s)

z3_set_member(x, s)

z3_set_subset(s, t)
```

## Arguments

- element:

  Element sort.

- s, t:

  Set expressions.

- x:

  An element.

- ...:

  Sets, or lists of sets.

## Value

A `z3_sort` or `z3_expr`.

## Examples

``` r
ctx <- z3_context()
I <- z3_int_sort(ctx)
a <- z3_const(ctx, "a", z3_set_sort(I))
s <- z3_solver(ctx)
z3_add(s, z3_set_member(3, a), z3_set_subset(a, z3_set_add(z3_empty_set(I), 3)))
z3_check(s)
#> [1] "sat"
```
