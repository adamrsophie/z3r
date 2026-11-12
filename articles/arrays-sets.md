# Arrays, sets and lambdas

``` r

library(z3r)
ctx <- z3_context()
I <- z3_int_sort(ctx)
B <- z3_bool_sort(ctx)
```

## Arrays are total maps

A Z3 array maps every value of its index sort to a value of its element
sort. There is no length: arrays model memory, tables and functions.

``` r

a <- z3_array(ctx, "a", I, I)
i <- z3_int(ctx, "i")
j <- z3_int(ctx, "j")
s <- z3_solver(ctx)
# Reading back what was written always succeeds...
z3_add(s, z3_select(z3_store(a, i, 42), i) != 42)
z3_check(s)
#> [1] "unsat"
# ...and writing at i never changes another index
s <- z3_solver(ctx)
z3_add(s, i != j, z3_select(z3_store(a, i, 42), j) != z3_select(a, j))
z3_check(s)
#> [1] "unsat"
```

Constant arrays give every index the same value:

``` r

zeros <- z3_const_array(I, 0)
z3_simplify(z3_select(z3_store(zeros, 3, 7), 3))
#> <z3_expr Int> 7
```

## Verifying a swap

Swapping two elements twice restores the array, for every array and
every pair of indices:

``` r

swap <- function(arr, p, q) {
  vp <- z3_select(arr, p)
  vq <- z3_select(arr, q)
  z3_store(z3_store(arr, p, vq), q, vp)
}
s <- z3_solver(ctx)
k <- z3_int(ctx, "k")
z3_add(s, z3_select(swap(swap(a, i, j), i, j), k) != z3_select(a, k))
z3_check(s)
#> [1] "unsat"
```

## Several indices

Arrays with several index sorts need Z3 4.8.15 or later.

``` r

grid <- z3_array(ctx, "grid", list(I, I), B)
s <- z3_solver(ctx)
z3_add(s, z3_select(z3_store(grid, list(1, 2), TRUE), 1, 2) == FALSE)
z3_check(s)
#> [1] "unsat"
```

## Lambdas, maps and extensionality

[`z3_lambda()`](https://adamrsophie.github.io/z3r/reference/z3_array.md)
builds an array from an expression over bound constants:

``` r

x <- z3_int(ctx, "x")
square <- z3_lambda(x, x * x)
z3_simplify(z3_select(square, 9))
#> <z3_expr Int> 81
```

[`z3_array_map()`](https://adamrsophie.github.io/z3r/reference/z3_array.md)
applies a function pointwise,
[`z3_array_default()`](https://adamrsophie.github.io/z3r/reference/z3_array.md)
reads the value outside finitely many stores, and
[`z3_array_ext()`](https://adamrsophie.github.io/z3r/reference/z3_array.md)
names an index where two different arrays differ:

``` r

b <- z3_array(ctx, "b", I, I)
plus <- z3_function(ctx, "plus", list(I, I), I)
z3_array_map(plus, a, b)
#> <z3_expr (Array Int Int)> ((_ map plus) a b)
s <- z3_solver(ctx)
w <- z3_array_ext(a, b)
z3_add(s, a != b, z3_select(a, w) == z3_select(b, w))
z3_check(s)
#> [1] "unsat"
```

## Sets

Sets are arrays to Bool, with the usual operations:

``` r

S <- z3_set_sort(I)
evens <- z3_const(ctx, "evens", S)
small <- z3_set_add(z3_set_add(z3_set_add(z3_empty_set(I), 1), 2), 3)
s <- z3_solver(ctx)
z3_add(s, z3_set_member(2, evens), !z3_set_member(3, evens),
       z3_set_subset(z3_set_intersect(evens, small), z3_set_add(z3_empty_set(I), 2)))
z3_check(s)
#> [1] "sat"
z3_simplify(z3_set_member(2, z3_set_union(small, z3_set_complement(small))))
#> <z3_expr Bool> true
```

Z3 5 adds a separate theory of *finite* sets with cardinality; see
[`vignette("z3-5-theories")`](https://adamrsophie.github.io/z3r/articles/z3-5-theories.md).
