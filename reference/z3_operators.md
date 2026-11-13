# Operators on Z3 expressions

Arithmetic, comparison and logical operators build new symbolic
expressions instead of computing R values.

## Usage

``` r
# S3 method for class 'z3_expr'
Ops(e1, e2)
```

## Arguments

- e1, e2:

  `z3_expr` objects or R scalars.

## Value

A `z3_expr`.

## Details

|  |  |  |  |
|----|----|----|----|
| Operator | Int / Real | Bool | bit-vector |
| `+`, `-`, `*` | arithmetic | error | modular arithmetic |
| `/` | exact real division | error | error, see [`z3_bvudiv()`](https://adamrsophie.github.io/z3r/reference/z3_bitvectors.md) |
| `^` | power | error | error |
| `%/%`, `%%` | Int only: SMT-LIB `div`, `mod` | error | error |
| `==`, `!=` | equation | equivalence | equation |
| `<`, `<=`, `>`, `>=` | comparison | error | error, see [`z3_bvult()`](https://adamrsophie.github.io/z3r/reference/z3_bitvectors.md) |
| `&`, `\|`, `!` | error | and, or, not | bitwise and, or, not |

Int and Real operands can be mixed; the Int operand is converted to Real
exactly. Every other combination of sorts is an error: Booleans are not
numbers, and bit-vectors are never mixed with unbounded integers.

**Division.** `/` is real division, as in R, so `x / 2` is Real even
when `x` is an Int. `%/%` and `%%` are Z3's integer `div` and `mod`,
which follow the SMT-LIB (Euclidean) convention: the remainder `a %% b`
is always between 0 and `abs(b) - 1`. This agrees with R for positive
divisors but **differs for negative divisors**: in Z3 `7 %/% -2` is `-3`
and `7 %% -2` is `1`, while R gives `-4` and `-1`. Division by zero is
not an error; Z3 treats `x / 0` as an unspecified value.

**Powers.** `x^k` for a literal whole number `0 <= k <= 64` is expanded
into a product and keeps the sort of `x`. Other powers use Z3's power
operator, which is Real-valued; constraints with symbolic exponents are
often beyond Z3's decision procedures and may return `"unknown"`.

**R values.** An R scalar combined with a Z3 expression is converted to
the sort of the other operand; see
[`z3_int_val()`](https://adamrsophie.github.io/z3r/reference/z3_int_val.md)
for the conversion rules for doubles. Only length-one R values are
accepted: z3r expressions are scalars, and R's recycling rules have no
meaning for a single formula. Build several constraints with
[`lapply()`](https://rdrr.io/r/base/lapply.html) or
[`Map()`](https://rdrr.io/r/base/funprog.html). Classed values such as
dates are refused; convert them first. A
[`bit64::integer64`](https://bit64.r-lib.org/reference/bit64-package.html)
works on the right of an operator (`x == i64`); on the left, bit64's own
operator method runs instead and fails, so write `z3_int_val(ctx, i64)`
there.

**R's own logic.** `&&` and `||` cannot be overloaded; use
[`z3_and()`](https://adamrsophie.github.io/z3r/reference/z3_and.md) and
[`z3_or()`](https://adamrsophie.github.io/z3r/reference/z3_and.md). A Z3
expression is never converted to `TRUE` or `FALSE`, so `if (x > 0)` is
an error rather than a silent mistake. Note that
[`isTRUE()`](https://rdrr.io/r/base/Logic.html) returns `FALSE` for any
Z3 expression.

## Examples

``` r
ctx <- z3_context()
x <- z3_int(ctx, "x")
y <- z3_real(ctx, "y")
p <- z3_bool(ctx, "p")
x + 1
#> <z3_expr Int> (+ x 1)
x / 2           # Real division
#> <z3_expr Real> (/ (to_real x) 2.0)
x %/% 2         # integer division
#> <z3_expr Int> (div x 2)
x^2 + y^2 <= 1
#> <z3_expr Bool> (<= (+ (to_real (* x x)) (* y y)) 1.0)
p & (x > 0) | !p
#> <z3_expr Bool> (or (and p (> x 0)) (not p))
```
