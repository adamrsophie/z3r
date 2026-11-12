# String operations

Z3 strings are sequences of Unicode characters. Character literals in R
are converted to Z3 string values exactly. Positions are 0-based, as in
SMT-LIB.

## Usage

``` r
z3_length(s)

z3_contains(s, t)

z3_prefixof(t, s)

z3_suffixof(t, s)

z3_substr(s, offset, length)

z3_str_at(s, i)

z3_indexof(s, t, offset = 0)

z3_replace(s, src, dst)

z3_str_to_int(s)

z3_int_to_str(x)

z3_str_lt(s, t)

z3_str_le(s, t)
```

## Arguments

- s, t, src, dst:

  String expressions or R strings.

- offset, length, i:

  Int expressions or R whole numbers.

- x:

  An Int (for `z3_int_to_str()`) or String expression.

## Value

A `z3_expr`.

## Details

- `z3_length()`: number of characters (an Int).

- [`z3_concat()`](https://adamrsophie.github.io/z3r/reference/z3_bitvectors.md):
  concatenation (see
  [z3_bitvectors](https://adamrsophie.github.io/z3r/reference/z3_bitvectors.md);
  it also joins strings).

- `z3_contains(s, t)`, `z3_prefixof(t, s)`, `z3_suffixof(t, s)`:
  containment tests. Note the SMT-LIB argument order:
  `z3_prefixof(t, s)` is "`t` is a prefix of `s`".

- `z3_substr(s, offset, length)`, `z3_str_at(s, i)`: substrings.

- `z3_indexof(s, t, offset)`: first position of `t` in `s` at or after
  `offset`, or -1.

- `z3_replace(s, src, dst)`: replaces the first occurrence of `src`.

- `z3_str_to_int()`, `z3_int_to_str()`: conversion between decimal
  strings and non-negative integers (`-1` for invalid strings).

- `z3_str_lt()`, `z3_str_le()`: lexicographic order by code point.

## Examples

``` r
ctx <- z3_context()
s <- z3_string(ctx, "s")
sol <- z3_solver(ctx)
z3_add(sol, z3_length(s) == 5, z3_prefixof("ab", s), z3_contains(s, "zz"))
z3_check(sol)
#> [1] "sat"
z3_value(z3_eval(z3_model(sol), s))
#> [1] "abAzz"
```
