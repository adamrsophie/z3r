# Sequences, regular expressions and characters

Z3 sequences are finite lists over any element sort; strings are
sequences of characters. The string functions
([`z3_length()`](https://adamrsophie.github.io/z3r/reference/z3_strings.md),
[`z3_concat()`](https://adamrsophie.github.io/z3r/reference/z3_bitvectors.md),
[`z3_contains()`](https://adamrsophie.github.io/z3r/reference/z3_strings.md),
[`z3_prefixof()`](https://adamrsophie.github.io/z3r/reference/z3_strings.md),
[`z3_suffixof()`](https://adamrsophie.github.io/z3r/reference/z3_strings.md),
[`z3_substr()`](https://adamrsophie.github.io/z3r/reference/z3_strings.md),
[`z3_str_at()`](https://adamrsophie.github.io/z3r/reference/z3_strings.md),
[`z3_indexof()`](https://adamrsophie.github.io/z3r/reference/z3_strings.md),
[`z3_replace()`](https://adamrsophie.github.io/z3r/reference/z3_strings.md))
work on any sequence.

## Usage

``` r
z3_seq_sort(element)

z3_re_sort(seq_sort)

z3_char_sort(ctx)

z3_sort_basis(sort)

z3_seq_empty(sort)

z3_seq_unit(x)

z3_seq_nth(s, i)

z3_last_indexof(s, t)

z3_replace_all(s, t, dst)

z3_replace_re(s, re, dst)

z3_replace_re_all(s, re, dst)

z3_seq_power(s, n)

z3_seq_map(f, s)

z3_seq_mapi(f, i, s)

z3_seq_foldl(f, acc, s)

z3_seq_foldli(f, i, acc, s)

z3_re(s)

z3_in_re(s, re)

z3_re_star(re)

z3_re_plus(re)

z3_re_option(re)

z3_re_complement(re)

z3_re_union(...)

z3_re_concat(...)

z3_re_intersect(...)

z3_re_diff(re1, re2)

z3_re_loop(re, lo, hi)

z3_re_power(re, n)

z3_re_range(lo, hi, ctx = NULL)

z3_re_empty(sort)

z3_re_full(sort)

z3_re_allchar(sort)

z3_char_val(ctx, code)

z3_char_to_int(x)

z3_char_to_bv(x)

z3_char_from_bv(x)

z3_char_is_digit(x)

z3_char_le(x, t)

z3_str_from_code(x)

z3_str_to_code(x)
```

## Arguments

- element, sort, seq_sort:

  Sorts.

- ctx:

  A
  [`z3_context()`](https://adamrsophie.github.io/z3r/reference/z3_context.md).

- x:

  An element, character, or Int expression.

- s, t, dst:

  Sequence (or string) expressions.

- i, n:

  Int expressions or R whole numbers.

- re, re1, re2:

  Regular expressions (or strings).

- f:

  An array-valued function (for example a
  [`z3_lambda()`](https://adamrsophie.github.io/z3r/reference/z3_array.md)).

- acc:

  Initial accumulator for folds.

- ...:

  Regular expressions or strings.

- lo, hi:

  Bounds: single-character strings for `z3_re_range()`, counts for
  `z3_re_loop()`.

- code:

  A Unicode code point.

## Value

A `z3_expr` or `z3_sort`.

## Details

- Sorts: `z3_seq_sort(element)`, `z3_re_sort(seq_sort)`,
  `z3_char_sort(ctx)`, and `z3_sort_basis()` for the element sort.

- Sequences: `z3_seq_empty(sort)`, `z3_seq_unit(x)` (one element),
  `z3_seq_nth(s, i)` (element `i`, 0-based), `z3_last_indexof(s, t)` (Z3
  before 4.8.15 misses a match at the last position), `z3_replace_all()`
  (Z3 4.15.5), `z3_seq_power(s, n)` (`s` repeated `n` times, Z3 5.3.0),
  and the higher-order `z3_seq_map()`, `z3_seq_mapi()`,
  `z3_seq_foldl()`, `z3_seq_foldli()` (Z3 4.13.2), whose function
  argument is an array such as one made with
  [`z3_lambda()`](https://adamrsophie.github.io/z3r/reference/z3_array.md).

- Regular expressions: `z3_re()` turns a string (or sequence) into a
  regular expression matching exactly it; `z3_in_re(s, re)` tests
  membership. Combinators: `z3_re_star()`, `z3_re_plus()`,
  `z3_re_option()`, `z3_re_union()`, `z3_re_concat()`,
  `z3_re_intersect()`, `z3_re_complement()`, `z3_re_diff()` (Z3 4.8.14),
  `z3_re_loop(re, lo, hi)`, `z3_re_power(re, n)` (Z3 4.8.15),
  `z3_re_range("a", "z", ctx)` (a character range), and the constants
  `z3_re_empty()`, `z3_re_full()` and `z3_re_allchar()` of a regular
  expression sort. Where a regular expression is expected, a string is
  accepted and converted with `z3_re()`. `z3_replace_re()` and
  `z3_replace_re_all()` replace matches (Z3 4.15.5); Z3's solver is
  incomplete for these and may answer `"unknown"`.

- Characters: `z3_char_val(ctx, code)` (Z3 4.8.15), `z3_char_to_int()`,
  `z3_char_to_bv()`, `z3_char_from_bv()`, `z3_char_is_digit()`,
  `z3_char_le()`, and `z3_str_from_code()`, `z3_str_to_code()` (Z3
  4.8.15) between one-character strings and code points.

## Examples

``` r
ctx <- z3_context()
s <- z3_string(ctx, "s")
digits <- z3_re_plus(z3_re_range("0", "9", ctx))
sol <- z3_solver(ctx)
z3_add(sol, z3_in_re(s, z3_re_concat("id-", digits)), z3_length(s) == 6)
z3_check(sol)
#> [1] "sat"
z3_value(z3_eval(z3_model(sol), s))
#> [1] "id-000"

I <- z3_int_sort(ctx)
q <- z3_const(ctx, "q", z3_seq_sort(I))
sol <- z3_solver(ctx)
z3_add(sol, z3_length(q) == 3, z3_seq_nth(q, 0) == 7,
       z3_contains(q, z3_concat(z3_seq_unit(z3_int_val(ctx, 1)), z3_seq_unit(z3_int_val(ctx, 2)))))
z3_check(sol)
#> [1] "sat"
z3_eval(z3_model(sol), q)
#> <z3_expr (Seq Int)> (seq.++ (seq.unit 7) (seq.unit 1) (seq.unit 2))
```
