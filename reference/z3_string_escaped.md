# String values as escaped text or bytes

`z3_string_escaped()` returns a string value in SMT-LIB form: characters
outside printable ASCII are written as `\u{...}` escapes (hexadecimal
code points), and so is a backslash followed by `u`. Unlike
[`z3_value()`](https://adamrsophie.github.io/z3r/reference/z3_value.md),
it works for strings containing U+0000, and
`z3_string_val(ctx, z3_string_escaped(x), escaped = TRUE)` gives `x`
back.

## Usage

``` r
z3_string_escaped(x)

z3_string_bytes(x)
```

## Arguments

- x:

  A `z3_expr` that is a string value, such as the result of
  [`z3_eval()`](https://adamrsophie.github.io/z3r/reference/z3_eval.md).

## Value

A character string for `z3_string_escaped()`; a raw vector for
`z3_string_bytes()`.

## Details

`z3_string_bytes()` returns the characters of a string value as a raw
vector, one byte per character. Every character must be below U+0100;
the result is the input of
[`z3_string_val()`](https://adamrsophie.github.io/z3r/reference/z3_int_val.md)
for raw vectors.

## Examples

``` r
ctx <- z3_context()
z3_string_escaped(z3_string_val(ctx, "caf\u00e9"))
#> [1] "caf\\u{e9}"
z3_string_bytes(z3_string_val(ctx, as.raw(c(0, 65, 255))))
#> [1] 00 41 ff
```
