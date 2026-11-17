# Linear constraints from a coefficient matrix

`z3_linear()` builds the rows of a linear system `A %*% x op b` as Z3
constraints in one native call, which is much faster than building them
with R operators row by row.

## Usage

``` r
z3_linear(A, x, op = "<=", b, nrow = NULL)
```

## Arguments

- A:

  A matrix, or a data frame or list with `row`, `col` and `value`
  (1-based row and column numbers).

- x:

  A list of Int or Real terms, one per column of `A`.

- op:

  Comparison: `"<="`, `"<"`, `">="`, `">"`, `"=="` or `"!="`.

- b:

  Right-hand sides: a vector or list.

- nrow:

  Number of rows, for a triplet `A` (by default the largest row number).

## Value

A list of Boolean `z3_expr`, one per row, named by the row names of a
dense `A`.

## Details

`A` is either a dense matrix (numeric, integer or character) or a sparse
matrix in triplet form: a data frame (or list) with columns `row`, `col`
and `value`, together with `nrow`. Coefficients and right-hand sides are
converted exactly, as R operands of Z3 terms are: integers and whole
doubles exactly, other doubles by the `z3r.doubles` policy, and strings
such as `"1/3"` as exact rationals. Use character values (or `gmp`
numbers in a list) for exact fractions. Missing values are refused. Zero
coefficients are left out.

No R recycling is applied: `x` must have one variable per column, and
`op` and `b` must have length 1 (used for every row) or one element per
row.

## Examples

``` r
ctx <- z3_context()
x <- z3_ints(ctx, c("a", "b", "c"))
A <- rbind(c(1, 2, 0), c(0, 1, -1))
cons <- z3_linear(A, x, "<=", c(10, 0))
cons
#> [[1]]
#> <z3_expr Bool> (<= (+ (* 1 a) (* 2 b)) 10)
#> 
#> [[2]]
#> <z3_expr Bool> (<= (+ (* 1 b) (* (- 1) c)) 0)
#> 
s <- z3_solver(ctx)
z3_add(s, cons, z3_linear(diag(3), x, ">=", 1))
z3_check(s)
#> [1] "sat"
# Sparse triplets with exact fractions:
y <- z3_reals(ctx, c("p", "q"))
z3_linear(data.frame(row = c(1, 1), col = 1:2, value = c("1/3", "2/3")),
          y, "==", "1")
#> [[1]]
#> <z3_expr Bool> (= (+ (* (/ 1.0 3.0) p) (* (/ 2.0 3.0) q)) (to_real 1))
#> 
```
