# Special relations

Binary relations with built-in axioms, decided by dedicated Z3
procedures: `z3_linear_order()`, `z3_partial_order()`,
`z3_piecewise_linear_order()` and `z3_tree_order()` over a sort, and
`z3_transitive_closure()` of a binary relation. `id` distinguishes
several relations of the same kind and sort.

## Usage

``` r
z3_linear_order(sort, id = 0)

z3_partial_order(sort, id = 0)

z3_piecewise_linear_order(sort, id = 0)

z3_tree_order(sort, id = 0)

z3_transitive_closure(f)
```

## Arguments

- sort:

  The sort the relation is over.

- id:

  A non-negative integer.

- f:

  A binary Boolean `z3_function`.

## Value

A `z3_function` taking two arguments.

## Examples

``` r
ctx <- z3_context()
U <- z3_uninterpreted_sort(ctx, "U")
le <- z3_partial_order(U)
a <- z3_const(ctx, "a", U)
b <- z3_const(ctx, "b", U)
s <- z3_solver(ctx)
z3_add(s, le(a, b), le(b, a), a != b)
z3_check(s)
#> [1] "unsat"
```
