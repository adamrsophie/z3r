# Model-based projection and light quantifier elimination

Spacer's utilities for eliminating variables from a formula.

## Usage

``` r
z3_model_project(
  model,
  vars,
  formula,
  method = c("plain", "witness", "skolem")
)

z3_model_extrapolate(model, formula)

z3_qe_lite(vars, formula)
```

## Arguments

- model:

  A `z3_model`.

- vars:

  A constant or a list of constants.

- formula:

  A Boolean expression.

- method:

  `"plain"`, `"witness"` or `"skolem"`.

## Value

An expression, or a list as described above.

## Details

- `z3_model_project(model, vars, formula)` performs model-based
  projection: it returns a formula free of `vars` that is true in
  `model` and implies `exists vars. formula`. `formula` must be true in
  `model` (an error otherwise). Variables Z3 cannot eliminate, such as
  some arrays, stay in the result. With `method = "witness"` (Z3 4.13.4)
  the result is a list with the projected `formula` and, in `vars` and
  `terms`, terms for the eliminated variables that keep the formula true
  in the model when substituted for them; with `method = "skolem"`, the
  list holds the Skolem terms recorded by arithmetic projection.

- `z3_model_extrapolate(model, formula)` returns a conjunction of
  literals that is true in `model` and implies `formula` (an implicant
  of `formula` taken from the model); `formula` must be true in `model`.

- `z3_qe_lite(vars, formula)` eliminates, where it is cheap to do so,
  the variables `vars` from `exists vars. formula`. It returns a list
  with the result `formula` and the variables that `remaining` free in
  it.

## Examples

``` r
ctx <- z3_context()
x <- z3_int(ctx, "x")
y <- z3_int(ctx, "y")
f <- x > 2 & y == x + 1
sol <- z3_solver(ctx)
z3_add(sol, f)
z3_check(sol)
#> [1] "sat"
m <- z3_model(sol)
z3_model_project(m, x, f)
#> <z3_expr Bool> (not (<= y 3))
z3_model_extrapolate(m, f | y < 0)
#> <z3_expr Bool> (and (= y (+ x 1)) (> x 2))
z3_qe_lite(x, y == x + 1 & x > 2)
#> $formula
#> <z3_expr Bool> (> (+ (- 1) y) 2)
#> 
#> $remaining
#> list()
#> 
```
