# Complex numbers, finite fields and finite sets (Z3 5.x)

Theories added in the Z3 5 series. Each needs z3r to be built against a
Z3 that has it; otherwise the functions report the version they need.

## Usage

``` r
z3_complex_sort(ctx)

z3_complex(re, im)

z3_real_part(z)

z3_imag_part(z)

z3_ff_sort(ctx, p)

z3_ff_size(sort)

z3_ff_add(...)

z3_ff_mul(...)

z3_ff_neg(x)

z3_ff_bitsum(...)

z3_finite_set_sort(element)

z3_fs_element_sort(sort)

z3_fs_empty(sort)

z3_fs_singleton(x)

z3_fs_union(s, t)

z3_fs_intersect(s, t)

z3_fs_difference(s, t)

z3_fs_member(x, s)

z3_fs_subset(s, t)

z3_fs_size(s)

z3_fs_range(lo, hi)

z3_fs_map(f, s)

z3_fs_filter(f, s)
```

## Arguments

- ctx:

  A
  [`z3_context()`](https://adamrsophie.github.io/z3r/reference/z3_context.md).

- re, im:

  Real parts.

- z:

  A complex expression.

- p:

  A prime, as a string or whole number.

- sort:

  A finite set or finite field sort.

- ...:

  Field elements.

- x:

  An element.

- element:

  Element sort.

- s, t:

  Finite sets.

- lo, hi:

  Int bounds.

- f:

  An array-valued function.

## Value

A `z3_expr`, a `z3_sort`, or for `z3_ff_size()` a string.

## Details

- Complex numbers (Z3 5.3.2): `z3_complex_sort(ctx)`,
  `z3_complex(re, im)`, `z3_real_part(z)`, `z3_imag_part(z)`. `+`, `-`,
  `*`, `/`,
  [`z3_sum()`](https://adamrsophie.github.io/z3r/reference/z3_sum.md),
  [`z3_prod()`](https://adamrsophie.github.io/z3r/reference/z3_sum.md)
  and equality accept complex expressions, with Int and Real embedded
  exactly; R complex numbers such as `1+2i` are literals under the usual
  double policy. Ordering is not defined for complex numbers.

- Finite fields (Z3 5.3.0): `z3_ff_sort(ctx, p)` is the field of
  integers modulo the prime `p` (given as a string for large primes);
  `z3_ff_add()`, `z3_ff_mul()`, `z3_ff_neg()` and `z3_ff_bitsum()` (sum
  of the arguments weighted by successive powers of two) operate in it;
  [`z3_val()`](https://adamrsophie.github.io/z3r/reference/z3_int_val.md)
  makes field elements. `z3_ff_size()` returns `p`.

- Finite sets (Z3 5.0.0): `z3_finite_set_sort(element)` is the sort of
  finite sets with cardinality, unlike the array-based sets of
  [`z3_set_sort()`](https://adamrsophie.github.io/z3r/reference/z3_set_sort.md).
  Operations: `z3_fs_empty()`, `z3_fs_singleton()`, `z3_fs_union()`,
  `z3_fs_intersect()`, `z3_fs_difference()`, `z3_fs_member()`,
  `z3_fs_subset()`, `z3_fs_size()` (an Int), `z3_fs_range(lo, hi)` (the
  integers from `lo` to `hi`), and the higher-order `z3_fs_map(f, s)`
  and `z3_fs_filter(f, s)` with array arguments such as
  [`z3_lambda()`](https://adamrsophie.github.io/z3r/reference/z3_array.md).
  `z3_fs_element_sort()` gives the element sort.

## Examples

``` r
ctx <- z3_context()
if (z3_version_at_least("5.3.2")) {
  z <- z3_const(ctx, "z", z3_complex_sort(ctx))
  s <- z3_solver(ctx)
  z3_add(s, z3_real_part(z) == 3, z3_imag_part(z) == -4)
  z3_check(s)
}
#> [1] "sat"
if (z3_version_at_least("5.0.0")) {
  I <- z3_int_sort(ctx)
  a <- z3_const(ctx, "a", z3_finite_set_sort(I))
  s <- z3_solver(ctx)
  z3_add(s, z3_fs_size(a) == 3, z3_fs_subset(a, z3_fs_range(z3_int_val(ctx, 1), 4)))
  z3_check(s)
}
#> [1] "unknown"
```
