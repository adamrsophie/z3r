# Convert between symcompR and Z3 expressions

`z3_from_symbolic()` translates a symbolic expression from the
[symcompR](https://github.com/adamrsophie/symcompR) package into a Z3
expression, and `z3_to_symbolic()` translates back. symcompR is
optional; these functions need it installed.

## Usage

``` r
z3_from_symbolic(ctx, expr, sorts = list())

z3_to_symbolic(x)
```

## Arguments

- ctx:

  A
  [`z3_context()`](https://adamrsophie.github.io/z3r/reference/z3_context.md).

- expr:

  A symcompR expression (an object of class `sym`).

- sorts:

  Optional named list of sorts for symbols.

- x:

  A `z3_expr`.

## Value

A `z3_expr`, or for `z3_to_symbolic()` a symcompR expression.

## Details

Only translations that preserve the mathematical meaning exactly are
made; anything else is an error naming the unsupported part.

**Supported.** Integers and rationals (exact); symbols; `+`, `-`, `*`,
`/`, powers with integer exponents (and rational exponents, which become
Z3's real power); `Eq`, `Ne`, `Lt`, `Le`, `Gt`, `Ge`; `And`, `Or`,
`Not`, `Implies`, `Xor`, `Equivalent`, `ITE`; `Abs`, `floor`, `ceiling`,
`Mod` (with Python's sign convention, which is also symcompR's: the
result takes the sign of the divisor), `Max`, `Min`, and `Piecewise`
with a final `TRUE` branch; `true` and `false`.

**Not supported.** Floats (they are inexact), constants such as `pi`,
`E` and `oo`, complex numbers, transcendental functions, derivatives,
integrals, sums and other unevaluated operators.

**Symbols and sorts.** A symbol becomes a Z3 constant of the same name.
Its sort comes from `sorts` (a named list of `z3_sort`s) when given;
otherwise from its assumptions: `integer = TRUE` gives Int and
`real = TRUE` gives Real. A symbol without assumptions is Bool where a
Boolean is expected (as an argument of `And`, `Or`, ...), and an error
elsewhere, because SymPy-style symbols without assumptions are complex.

In the other direction, Int constants become symbols with
`integer = TRUE`, Real constants symbols with `real = TRUE`, and Bool
constants plain symbols. Z3's `div` and `mod` are translated with
`floor` and `Abs` so that the result keeps Z3's (Euclidean) meaning.

## Examples

``` r
if (requireNamespace("symcompR", quietly = TRUE)) {
  x <- symcompR::Symbol("x", integer = TRUE)
  y <- symcompR::Symbol("y", integer = TRUE)
  ctx <- z3_context()
  e <- z3_from_symbolic(ctx, symcompR::Eq(x^2 + y^2, 25))
  s <- z3_solver(ctx)
  z3_add(s, e, z3_int(ctx, "x") > 3, z3_int(ctx, "y") > 0)
  z3_check(s)
  z3_model(s)
  z3_to_symbolic(z3_int(ctx, "x") * 2 + 1)
}
#> 2*x + 1
```
