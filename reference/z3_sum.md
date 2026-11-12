# Arithmetic functions

- `z3_sum()` and `z3_prod()` add or multiply any number of arithmetic
  (or same-width bit-vector) expressions; lists are spliced.

- `z3_div()`, `z3_mod()` and `z3_rem()` are Z3's integer division,
  modulus and remainder. `div` and `mod` follow the SMT-LIB Euclidean
  convention (`0 <= mod(a, b) < abs(b)`); `rem` takes the sign of the
  divisor as in SMT-LIB. See
  [z3_operators](https://adamrsophie.github.io/z3r/reference/z3_operators.md)
  for how this differs from R.

- `z3_abs()` is the absolute value: Z3's `abs` for Int and Real on Z3
  4.13.2 and later, and `(ite (< x 0) (- x) x)` before.

- `z3_to_real()` converts Int to Real; `z3_to_int()` is the floor of a
  Real; `z3_is_int()` states that a Real is a whole number.

## Usage

``` r
z3_sum(...)

z3_prod(...)

z3_div(a, b)

z3_mod(a, b)

z3_rem(a, b)

z3_abs(x)

z3_to_real(x)

z3_to_int(x)

z3_is_int(x)
```

## Arguments

- ...:

  Arithmetic expressions or lists of them.

- a, b, x:

  Arithmetic expressions.

## Value

A `z3_expr`.

## Examples

``` r
ctx <- z3_context()
v <- z3_ints(ctx, c("a", "b", "c"))
z3_sum(v)
#> <z3_expr Int> (+ a b c)
z3_simplify(z3_mod(z3_int_val(ctx, -7), 2))
#> <z3_expr Int> 1
```
