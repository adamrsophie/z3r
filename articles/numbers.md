# Exact numbers, sorts and operators

``` r

library(z3r)
ctx <- z3_context()
```

Every Z3 expression has a *sort*: its type. z3r keeps sorts strict and
numbers exact. This vignette explains how R values become Z3 values,
what each operator means for each sort, and how results come back to R.

## Sorts

``` r

z3_int_sort(ctx)
#> <z3_sort> Int
z3_real_sort(ctx)
#> <z3_sort> Real
z3_bool_sort(ctx)
#> <z3_sort> Bool
z3_bv_sort(ctx, 16)
#> <z3_sort> (_ BitVec 16)
z3_string_sort(ctx)
#> <z3_sort> String
z3_array_sort(z3_int_sort(ctx), z3_bool_sort(ctx))
#> <z3_sort> (Array Int Bool)
z3_uninterpreted_sort(ctx, "Person")
#> <z3_sort> Person
```

`Int` is the mathematical integers, with no overflow. `Real` is the
exact reals: rationals, and algebraic numbers such as the square root of
2 in models. Neither is a machine type. Fixed-width machine integers are
bit-vectors (see
[`vignette("bitvectors")`](https://adamrsophie.github.io/z3r/articles/bitvectors.md))
and IEEE doubles are the floating point sorts (see
[`vignette("floating-point")`](https://adamrsophie.github.io/z3r/articles/floating-point.md)).

Constants are created with a name and a sort. The same name and sort
give the same constant:

``` r

x <- z3_int(ctx, "x")
z3_identical(x, z3_int(ctx, "x"))
#> [1] TRUE
v <- z3_reals(ctx, c("a", "b", "c"))
z3_sort(v$a)
#> <z3_sort> Real
z3_sort_kind(v$a)
#> [1] "real"
```

## From R values to Z3 values

[`z3_int_val()`](https://adamrsophie.github.io/z3r/reference/z3_int_val.md)
and
[`z3_real_val()`](https://adamrsophie.github.io/z3r/reference/z3_int_val.md)
build exact numerals. Strings are the safe way to give large or
fractional values:

``` r

z3_int_val(ctx, "123456789012345678901234567890")
#> <z3_expr Int> 123456789012345678901234567890
z3_real_val(ctx, "1/3")
#> <z3_expr Real> (/ 1.0 3.0)
z3_real_val(ctx, "0.125")
#> <z3_expr Real> (/ 1.0 8.0)
z3_real_val(ctx, "-2.5e-3")
#> <z3_expr Real> (- (/ 1.0 400.0))
```

Z3’s own numeral parser silently misreads some malformed strings, so z3r
validates them first:

``` r

z3_real_val(ctx, "1/-3")
#> Error:
#> ! "1/-3" is not a decimal such as "0.125" or a fraction such as "1/3"
z3_real_val(ctx, "1.2.3")
#> Error:
#> ! "1.2.3" is not a decimal such as "0.125" or a fraction such as "1/3"
```

R numbers mixed with expressions are converted by a fixed rule:

- whole numbers become exact integers;
- whole numbers beyond 2^53 are refused, because R may already have
  rounded them;
- other doubles become the rational of their shortest decimal form, so
  `0.1` is 1/10, the value R prints;
- `NA`, `NaN` and `Inf` are refused.

``` r

r <- z3_real(ctx, "r")
r + 0.1
#> <z3_expr Real> (+ r (/ 1.0 10.0))
x + 2^53
#> <z3_expr Int> (+ x 9007199254740992)
x + 2^60
#> Error:
#> ! the double 1.152921504606847e+18 is beyond 2^53, so R may already have rounded it; give large integers exactly as strings with z3_int_val() or as gmp::bigz
```

`options(z3r.doubles = "error")` makes every non-integer double an
error, which forces exact strings everywhere. `gmp` numbers convert
exactly:

``` r

x * gmp::as.bigz("100000000000000000000")
#> <z3_expr Int> (* x 100000000000000000000)
r + gmp::as.bigq(1, 7)
#> <z3_expr Real> (+ r (/ 1.0 7.0))
```

## Operators

| Operator | Int / Real | Bool | bit-vector | floating point |
|----|----|----|----|----|
| `+ - *` | arithmetic | error | modular | IEEE, round to nearest even |
| `/` | exact real division | error | error | IEEE division |
| `^` | power | error | error | error |
| `%/% %%` | Int: SMT-LIB div, mod | error | error | `%%`: IEEE remainder |
| `== !=` | equation | equivalence | equation | structural equality |
| `< <= > >=` | comparison | error | error | IEEE comparison |
| `& \| !` | error | logic | bitwise | error |

Int and Real mix by promoting the Int exactly. Everything else must have
matching sorts:

``` r

p <- z3_bool(ctx, "p")
x + r
#> <z3_expr Real> (+ (to_real x) r)
x + p
#> Error:
#> ! '+' needs Int or Real arguments, got Bool; use z3_ite(p, 1, 0) to count Booleans
```

Z3’s C API would have accepted `x + p` by converting the Boolean to 0 or
1; z3r refuses, because that is almost always a mistake.

### Division

`/` is real division, as in R:

``` r

x / 2
#> <z3_expr Real> (/ (to_real x) 2.0)
z3_sort_kind(x / 2)
#> [1] "real"
```

`%/%` and `%%` are integer division and modulus with the SMT-LIB
(Euclidean) convention: the remainder is never negative. This agrees
with R for positive divisors and differs for negative ones:

``` r

i <- function(v) z3_int_val(ctx, v)
c(z3 = z3_value(z3_simplify(i(-7) %/% 2)), R = -7 %/% 2)
#> z3  R 
#> -4 -4
c(z3 = z3_value(z3_simplify(i(7) %/% -2)), R = 7 %/% -2)
#> z3  R 
#> -3 -4
c(z3 = z3_value(z3_simplify(i(7) %% -2)), R = 7 %% -2)
#> z3  R 
#>  1 -1
```

[`z3_rem()`](https://adamrsophie.github.io/z3r/reference/z3_sum.md) has
the sign of the divisor. Division by zero is not an error in SMT:
`x / 0` is some unspecified value, the same each time.

### Powers

A literal whole exponent up to 64 is expanded into a product, so the
result keeps the sort of the base. Other powers use Z3’s real power
operator:

``` r

x^3
#> <z3_expr Int> (* x x x)
z3_sort_kind(x^3)
#> [1] "int"
z3_sort_kind(x^x)
#> [1] "real"
```

### Scalars only

z3r expressions are single formulas. R’s recycling would silently build
the wrong formula, so vectors are refused:

``` r

x + 1:3
#> Error:
#> ! R values combined with Z3 expressions must have length 1 (got length 3); z3r expressions are scalars, so build element-wise constraints with lapply() or Map()
```

Build several constraints with
[`lapply()`](https://rdrr.io/r/base/lapply.html) or
[`Map()`](https://rdrr.io/r/base/funprog.html) instead.

### No accidental logic

A formula never turns into `TRUE` or `FALSE`:

``` r

if (x > 0) "positive"
#> Error in `if (x > 0) ...`:
#> ! argument is not interpretable as logical
```

## From Z3 values back to R

[`z3_value()`](https://adamrsophie.github.io/z3r/reference/z3_value.md)
returns a double only when the value is exactly representable; otherwise
a `gmp` number, or the exact string when gmp is missing:

``` r

z3_value(z3_int_val(ctx, 42))
#> [1] 42
z3_value(z3_real_val(ctx, "1/4"))
#> [1] 0.25
z3_value(z3_real_val(ctx, "1/3"))
#> Big Rational ('bigq') :
#> [1] 1/3
z3_value(z3_int_val(ctx, "9007199254740993"))
#> Big Integer ('bigz') :
#> [1] 9007199254740993
z3_value(z3_real_val(ctx, "1/3"), exact = FALSE)
#> [1] 0.3333333
```

Irrational algebraic numbers come from nonlinear real problems. They
have no exact R value, so
[`z3_value()`](https://adamrsophie.github.io/z3r/reference/z3_value.md)
asks for `exact = FALSE`:

``` r

s <- z3_solver(ctx)
z3_add(s, r^3 == 2)
z3_check(s)
#> [1] "sat"
cube_root <- z3_eval(z3_model(s), r)
cube_root
#> <z3_expr Real> (root-obj (+ (^ x 3) (- 2)) 1)
z3_value(cube_root)
#> Error:
#> ! (root-obj (+ (^ x 3) (- 2)) 1) is an irrational algebraic number with no exact R representation; use exact = FALSE for a double or z3_decimal() for digits
z3_value(cube_root, exact = FALSE)
#> [1] 1.259921
z3_decimal(cube_root, 30)
#> [1] "1.259921049894873164767210607278?"
```

[`vignette("algebraic")`](https://adamrsophie.github.io/z3r/articles/algebraic.md)
shows exact arithmetic on such numbers.

## Simplification and printing

``` r

z3_simplify(x + 2 * x - 3 + 3)
#> <z3_expr Int> (* 3 x)
z3_simplify((x + 1)^2, som = TRUE)
#> <z3_expr Int> (+ 1 (* 2 x) (* x x))
head(z3_simplify_help(ctx)[, c("name", "kind")])
#>                name kind
#> 1   ite_extra_rules bool
#> 2              flat bool
#> 3       flat_and_or bool
#> 4 sort_disjunctions bool
#> 5          elim_and bool
#> 6          elim_ite bool
```

Expressions print in SMT-LIB2 prefix form.
[`z3_set_print_mode()`](https://adamrsophie.github.io/z3r/reference/z3_set_print_mode.md)
switches a context to Z3’s low-level form, which shows shared subterms.
