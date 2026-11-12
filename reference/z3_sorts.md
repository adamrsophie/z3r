# Z3 sorts

Sorts are the types of Z3 expressions. Every constant and every
expression has exactly one sort, and Z3 operations only combine
expressions of compatible sorts.

## Usage

``` r
z3_int_sort(ctx)

z3_real_sort(ctx)

z3_bool_sort(ctx)

z3_bv_sort(ctx, width)

z3_string_sort(ctx)

z3_array_sort(domain, range)

z3_uninterpreted_sort(ctx, name)
```

## Arguments

- ctx:

  A
  [`z3_context()`](https://adamrsophie.github.io/z3r/reference/z3_context.md).

- width:

  Bit-vector width in bits, a positive integer.

- domain, range:

  Sorts of array indices and array elements.

- name:

  Name of the uninterpreted sort.

## Value

An object of class `z3_sort`.

## Details

- `z3_int_sort()`: mathematical integers of unbounded size.

- `z3_real_sort()`: exact real numbers (rationals and, in models of
  nonlinear problems, algebraic numbers). Not floating point.

- `z3_bool_sort()`: the Booleans.

- `z3_bv_sort()`: fixed-width bit-vectors (machine integers).

- `z3_string_sort()`: Unicode strings.

- `z3_array_sort()`: total maps from a domain sort to a range sort.

- `z3_uninterpreted_sort()`: an abstract sort with no structure except
  equality.

## See also

[`z3_sort()`](https://adamrsophie.github.io/z3r/reference/z3_sort.md) to
get the sort of an expression,
[`z3_const()`](https://adamrsophie.github.io/z3r/reference/z3_const.md).

## Examples

``` r
ctx <- z3_context()
z3_int_sort(ctx)
#> <z3_sort> Int
z3_bv_sort(ctx, 8)
#> <z3_sort> (_ BitVec 8)
z3_array_sort(z3_int_sort(ctx), z3_bool_sort(ctx))
#> <z3_sort> (Array Int Bool)
```
