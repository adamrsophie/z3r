# Bit-vectors: machine integers and program verification

``` r

library(z3r)
ctx <- z3_context()
```

Bit-vectors model the fixed-width integers of hardware and of languages
such as C: arithmetic wraps around, and the same bits can be read as
signed or unsigned numbers. They are the right sort for reasoning about
programs, hashes, cryptographic code and circuits.

## Values and wrap-around

``` r

b8 <- function(v) z3_bv_val(ctx, v, 8)
z3_simplify(b8(255) + 1)
#> <z3_expr (_ BitVec 8)> #x00
z3_value(z3_simplify(b8(100) * 3))
#> [1] 44
z3_value(b8(200))
#> [1] 200
z3_value(b8(200), signed = TRUE)
#> [1] -56
```

Literals must fit the width, as an unsigned or as a two’s complement
signed value:

``` r

b8(-128)
#> <z3_expr (_ BitVec 8)> #x80
b8(256)
#> Error:
#> ! the value 256 does not fit in a bit-vector of width 8
```

## Signed and unsigned operations

Comparison, division and right shift depend on how the bits are read, so
z3r provides both versions and does not overload `<` or `/` for
bit-vectors:

``` r

x <- z3_bv(ctx, "x", 8)
x < 3
#> Error:
#> ! '<' is not defined for bit-vectors because signed and unsigned semantics differ; use z3_bvult(), z3_bvslt(), z3_bvudiv(), z3_bvsdiv() and related functions
val <- function(e) z3_value(z3_simplify(e))
c(unsigned = val(z3_bvult(b8(200), b8(100))), signed = val(z3_bvslt(b8(200), b8(100))))
#> unsigned   signed 
#>    FALSE     TRUE
c(udiv = val(z3_bvudiv(b8(200), b8(3))), sdiv = z3_value(z3_simplify(z3_bvsdiv(b8(200), b8(3))), signed = TRUE))
#> udiv sdiv 
#>   66  -18
c(lshr = val(z3_bvlshr(b8(0x80), b8(7))), ashr = val(z3_bvashr(b8(0x80), b8(7))))
#> lshr ashr 
#>    1  255
```

`&`, `|` and `!` are bitwise on bit-vectors;
[`z3_bvxor()`](https://adamrsophie.github.io/z3r/reference/z3_bitvectors.md),
[`z3_bvshl()`](https://adamrsophie.github.io/z3r/reference/z3_bitvectors.md),
rotations,
[`z3_concat()`](https://adamrsophie.github.io/z3r/reference/z3_bitvectors.md),
[`z3_extract()`](https://adamrsophie.github.io/z3r/reference/z3_bitvectors.md)
and the extensions complete the set:

``` r

val(b8(0xF0) & b8(0x3C))
#> [1] 48
val(z3_extract(b8(0xAB), 7, 4))
#> [1] 10
val(z3_concat(z3_bv_val(ctx, 0xA, 4), z3_bv_val(ctx, 0xB, 4)))
#> [1] 171
val(z3_sign_ext(b8(0x80), 8))
#> [1] 65408
val(z3_rotate_left(b8(0x81), 1))
#> [1] 3
```

## Overflow

[`z3_bvadd_no_overflow()`](https://adamrsophie.github.io/z3r/reference/z3_bitvectors.md)
and friends state that an operation does not overflow. Asking for a
counterexample to “no overflow” finds inputs that break a computation:

``` r

a <- z3_bv(ctx, "a", 32)
b <- z3_bv(ctx, "b", 32)
s <- z3_solver(ctx)
# The classic midpoint bug: (a + b) / 2 overflows for large indices
z3_add(s, z3_bvsge(a, 0), z3_bvsge(b, 0), !z3_bvadd_no_overflow(a, b, signed = TRUE))
z3_check(s)
#> [1] "sat"
m <- z3_model(s)
c(a = z3_value(z3_eval(m, a)), b = z3_value(z3_eval(m, b)))
#>          a          b 
#> 2147418111 1073807364
# The fix a + (b - a) / 2 never overflows when 0 <= a <= b
s <- z3_solver(ctx)
z3_add(s, z3_bvsge(a, 0), z3_bvsle(a, b),
       !z3_bvadd_no_overflow(a, z3_bvsdiv(b - a, z3_bv_val(ctx, 2, 32)), signed = TRUE))
z3_check(s)
#> [1] "unsat"
```

## Proving bit tricks correct

The branch-free absolute value `(x ^ (x >> 31)) - (x >> 31)` equals the
obvious definition for all 2^32 inputs:

``` r

v <- z3_bv(ctx, "v", 32)
m <- z3_bvashr(v, 31)
s <- z3_solver(ctx)
z3_add(s, z3_bvxor(v, m) - m != z3_ite(z3_bvslt(v, 0), -v, v))
z3_check(s)
#> [1] "unsat"
```

“Is x a power of two” as `x & (x - 1) == 0`:

``` r

x <- z3_bv(ctx, "x", 16)
s <- z3_solver(ctx)
is_pow2 <- z3_or(lapply(0:15, function(k) x == 2^k))
z3_add(s, x != 0, ((x & (x - 1)) == 0) != is_pow2)
z3_check(s)
#> [1] "unsat"
```

Counting bits with a well-known parallel trick, checked against a direct
sum of the bits:

``` r

x <- z3_bv(ctx, "x", 8)
k <- function(v) z3_bv_val(ctx, v, 8)
c1 <- x - (z3_bvlshr(x, k(1)) & k(0x55))
c2 <- (c1 & k(0x33)) + (z3_bvlshr(c1, k(2)) & k(0x33))
c3 <- (c2 + z3_bvlshr(c2, k(4))) & k(0x0F)
naive <- z3_sum(lapply(0:7, function(i) z3_zero_ext(z3_extract(x, i, i), 7)))
s <- z3_solver(ctx)
z3_add(s, c3 != naive)
z3_check(s)
#> [1] "unsat"
```

## Between bit-vectors and integers

[`z3_bv2int()`](https://adamrsophie.github.io/z3r/reference/z3_bitvectors.md)
reads the bits as an unbounded integer (signed or not) and
[`z3_int2bv()`](https://adamrsophie.github.io/z3r/reference/z3_bitvectors.md)
reduces an integer modulo 2^width:

``` r

val(z3_bv2int(b8(250)))
#> [1] 250
val(z3_bv2int(b8(250), signed = TRUE))
#> [1] -6
val(z3_int2bv(z3_int_val(ctx, 300), 8))
#> [1] 44
```

## Reversing a hash

A tiny hash function, inverted by the solver:

``` r

h <- function(x) {
  x <- z3_bvxor(x, z3_bvlshr(x, z3_bv_val(ctx, 7, 16)))
  x <- x * 0x2B57
  z3_bvxor(x, z3_bvlshr(x, z3_bv_val(ctx, 5, 16)))
}
x <- z3_bv(ctx, "x", 16)
s <- z3_solver(ctx)
z3_add(s, h(x) == 0xBEEF)
z3_check(s)
#> [1] "sat"
pre <- z3_eval(z3_model(s), x)
z3_value(pre)
#> [1] 62300
z3_value(z3_simplify(h(pre)))
#> [1] 48879
```
