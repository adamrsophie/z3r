# Symbolic constants

Create named symbolic constants (the "variables" of an SMT problem). Two
constants with the same name and sort in the same context are the same
constant.

## Usage

``` r
z3_const(ctx, name, sort)

z3_int(ctx, name)

z3_real(ctx, name)

z3_bool(ctx, name)

z3_bv(ctx, name, width)

z3_string(ctx, name)

z3_ints(ctx, name)

z3_reals(ctx, name)

z3_bools(ctx, name)

z3_fresh_const(ctx, prefix, sort)
```

## Arguments

- ctx:

  A
  [`z3_context()`](https://adamrsophie.github.io/z3r/reference/z3_context.md).

- name:

  Name of the constant, a single string or a whole number (an integer
  symbol). For the plural forms, a character vector of names.

- sort:

  A `z3_sort`, see
  [z3_sorts](https://adamrsophie.github.io/z3r/reference/z3_sorts.md).

- width:

  Bit-vector width in bits.

- prefix:

  Prefix of the generated name for `z3_fresh_const()`.

## Value

A `z3_expr`, or a named list of them for the plural forms.

## Details

`z3_ints()`, `z3_reals()` and `z3_bools()` create several constants at
once and return them as a named list.

A name may also be a whole number from 0 to 2^30 - 1, which makes a Z3
integer symbol. Z3 prints integer symbol `n` as `k!n`, but it is a
different name from the string `"k!n"`, so the two give different
constants.

## See also

[`z3_int_val()`](https://adamrsophie.github.io/z3r/reference/z3_int_val.md)
for numeric constants,
[`z3_array()`](https://adamrsophie.github.io/z3r/reference/z3_array.md),
[`z3_function()`](https://adamrsophie.github.io/z3r/reference/z3_function.md).

## Examples

``` r
ctx <- z3_context()
x <- z3_int(ctx, "x")
y <- z3_real(ctx, "y")
p <- z3_bool(ctx, "p")
b <- z3_bv(ctx, "b", 8)
v <- z3_ints(ctx, c("a", "b", "c"))
v$a + v$b + v$c
#> <z3_expr Int> (+ a b c)
z3_int(ctx, 7L)
#> <z3_expr Int> k!7
```
