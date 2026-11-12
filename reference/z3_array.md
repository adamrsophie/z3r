# Arrays

Z3 arrays are total functions from a domain sort to a range sort, with
no length. `z3_select(a, i)` reads index `i`; `z3_store(a, i, v)` is a
new array equal to `a` except that index `i` maps to `v`.
`z3_const_array()` maps every index to the same value.

## Usage

``` r
z3_array(ctx, name, domain, range)

z3_select(a, i, ...)

z3_store(a, i, v)

z3_const_array(domain, value)

z3_array_map(f, ...)

z3_array_default(a)

z3_array_ext(a, b)

z3_as_array(f)

z3_lambda(vars, body)

z3_array_sort_info(x)
```

## Arguments

- ctx:

  A
  [`z3_context()`](https://adamrsophie.github.io/z3r/reference/z3_context.md).

- name:

  Name of the array constant.

- domain, range:

  `z3_sort` objects for indices (or a list of them) and elements.

- a:

  An array expression.

- i:

  An index (expression or R scalar), or for `z3_store()` on a
  multi-index array, a list of indices.

- ...:

  Further indices for multi-index arrays, or arrays for
  `z3_array_map()`.

- v:

  A value (expression or R scalar).

- value:

  The value of every element of a constant array.

- f:

  A `z3_function` or `z3_func_decl`.

- b:

  Another array.

- vars:

  A constant or list of constants to bind.

- body:

  An expression.

- x:

  An array expression or array sort.

## Value

A `z3_expr`.

`z3_array_sort_info()` returns a list with the index sorts (`domain`)
and element sort (`range`).

## Details

Arrays may have several index sorts (give `domain` as a list; Z3 4.8.15
or later); then `z3_select(a, i, j)` takes one index per sort and
`z3_store(a, list(i, j), v)` takes the indices as a list.

- `z3_array_map(f, a, b, ...)` applies a function pointwise.

- `z3_array_default(a)` is the value at all but finitely many indices.

- `z3_array_ext(a, b)` is an index at which two different arrays differ.

- `z3_as_array(f)` turns a function into an array.

- `z3_lambda(vars, body)` builds an array from an expression over bound
  constants, as in `z3_lambda(i, i * i)`.

## Examples

``` r
ctx <- z3_context()
a <- z3_array(ctx, "a", z3_int_sort(ctx), z3_int_sort(ctx))
i <- z3_int(ctx, "i")
s <- z3_solver(ctx)
z3_add(s, z3_select(z3_store(a, i, 10), i) != 10)
z3_check(s)
#> [1] "unsat"
zeros <- z3_const_array(z3_int_sort(ctx), 0)
z3_simplify(z3_select(zeros, 5))
#> <z3_expr Int> 0
```
