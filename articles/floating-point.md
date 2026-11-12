# Floating point: reasoning about IEEE 754 arithmetic

``` r

library(z3r)
ctx <- z3_context()
D <- z3_fp64_sort(ctx)       # IEEE double: R's numbers
H <- z3_fp16_sort(ctx)       # half precision
```

Z3 models IEEE 754 binary floating point exactly: rounding, signed
zeros, subnormal numbers, infinities and NaN. It can answer questions
such as “can this formula ever overflow?” or “which input makes these
two expressions differ?” about the arithmetic R itself performs.

## Values

``` r

z3_fp_val(ctx, 0.1, D)
#> <z3_expr (_ FloatingPoint 11 53)> (fp #b0 #b01111111011 #x999999999999a)
z3_value(z3_fp_val(ctx, 0.1, D))
#> [1] 0.1
z3_value(z3_fp_nan(D))
#> [1] NaN
z3_value(z3_fp_inf(D, negative = TRUE))
#> [1] -Inf
1 / z3_value(z3_fp_zero(D, negative = TRUE))
#> [1] -Inf
```

An R number goes into a format only if the format holds it exactly;
rounding must be explicit:

``` r

f <- z3_fp(ctx, "f", H)
f + 0.1
#> Error:
#> ! the double 0.10000000000000001 is not exactly representable in (_ FloatingPoint 5 11); round it explicitly with z3_fp_val(ctx, value, sort, rounding)
z3_value(z3_fp_val(ctx, 0.1, H, "RNE"))
#> [1] 0.09997559
```

## Arithmetic agrees with R

`+`, `-`, `*` and `/` round to nearest, ties to even, exactly as R does:

``` r

fv <- function(v) z3_fp_val(ctx, v, D)
z3_value(z3_simplify(fv(0.1) + fv(0.2))) == 0.1 + 0.2
#> [1] TRUE
z3_value(z3_simplify(fv(0.1) + fv(0.2))) == 0.3
#> [1] FALSE
```

Other rounding modes are explicit:

``` r

two <- fv(2)
up <- z3_value(z3_simplify(z3_fp_sqrt(z3_rounding(ctx, "RTP"), two)))
down <- z3_value(z3_simplify(z3_fp_sqrt(z3_rounding(ctx, "RTN"), two)))
sprintf("%.17g", c(down, up))
#> [1] "1.4142135623730949" "1.4142135623730951"
```

## Equality: structural and IEEE

`==` is structural: it compares values as Z3 terms, so `NaN == NaN` is
true and `0 == -0` is false.
[`z3_fp_eq()`](https://adamrsophie.github.io/z3r/reference/z3_fp.md) is
IEEE equality, where the opposite holds:

``` r

nan <- z3_fp_nan(D)
c(structural = z3_value(z3_simplify(nan == nan)), ieee = z3_value(z3_simplify(z3_fp_eq(nan, nan))))
#> structural       ieee 
#>       TRUE      FALSE
zero <- z3_fp_zero(D)
mzero <- z3_fp_zero(D, negative = TRUE)
c(structural = z3_value(z3_simplify(zero == mzero)), ieee = z3_value(z3_simplify(z3_fp_eq(zero, mzero))))
#> structural       ieee 
#>      FALSE       TRUE
```

## Questions about floating point code

Is there a double `x` with `x + 1 == x`?

``` r

x <- z3_fp(ctx, "x", D)
s <- z3_solver(ctx)
z3_add(s, x + 1 == x, z3_fp_is_normal(x), x > 0)
z3_check(s)
#> [1] "sat"
v <- z3_value(z3_eval(z3_model(s), x))
v
#> [1] 1.214975e+16
v + 1 == v
#> [1] TRUE
```

Is addition associative? Find a counterexample in half precision:

``` r

a <- z3_fp(ctx, "a", H)
b <- z3_fp(ctx, "b", H)
c <- z3_fp(ctx, "c", H)
s <- z3_solver(ctx)
z3_add(s, !z3_fp_is_nan(a), !z3_fp_is_nan(b), !z3_fp_is_nan(c),
       !z3_fp_eq((a + b) + c, a + (b + c)))
z3_check(s)
#> [1] "sat"
m <- z3_model(s)
vapply(list(a = a, b = b, c = c), function(t) z3_value(z3_eval(m, t)), 0)
#>            a            b            c 
#>     12.75781 -32768.00000  32752.00000
```

Does `x * x == 2` have a half precision solution? Only because of
rounding:

``` r

h <- z3_fp(ctx, "h", H)
s <- z3_solver(ctx)
z3_add(s, h * h == z3_fp_val(ctx, 2, H), h > 0)
z3_check(s)
#> [1] "sat"
z3_value(z3_eval(z3_model(s), h))
#> [1] 1.414062
```

## Conversions

``` r

rne <- z3_rounding(ctx)
z3_simplify(z3_fp_to_real(fv(0.75)))
#> <z3_expr Real> (/ 3.0 4.0)
z3_value(z3_simplify(z3_fp_to_sbv(rne, fv(-3.7), 8)), signed = TRUE)
#> [1] -4
z3_value(z3_simplify(z3_fp_to_fp(rne, z3_real_val(ctx, "1/3"), D)))
#> [1] 0.3333333
bits <- z3_simplify(z3_fp_to_ieee_bv(fv(1)))
format(z3_value(bits))
#> Big Integer ('bigz') object of length 20:
#>  [1] 1  0  0  0  2  0  0  0  1  0  0  0  0  0  NA NA 0  0  0  0
z3_value(z3_simplify(z3_fp_from_bv(bits, D)))
#> [1] 1
z3_fp_numeral_parts(fv(1.5))[c("class", "negative", "significand", "exponent")]
#> $class
#> [1] "normal"
#> 
#> $negative
#> [1] FALSE
#> 
#> $significand
#> [1] "1.5"
#> 
#> $exponent
#> [1] "0"
```

## Performance

Z3 decides floating point by translating to bit-vector circuits.
Questions involving multiplication or division of double precision
values build large circuits and can be slow; half or single precision
explorations, as above, are much faster and often reveal the same
phenomena.
