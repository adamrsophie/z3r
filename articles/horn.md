# Horn clauses and Datalog

``` r

library(z3r)
ctx <- z3_context()
I <- z3_int_sort(ctx)
B <- z3_bool_sort(ctx)
```

Z3’s fixedpoint engine answers reachability questions over recursively
defined relations. With finite relations this is Datalog. With
arithmetic and other theories it solves *constrained Horn clauses*
(CHC), the form that many program verification problems take: “can this
program reach an error state?”. The Spacer engine either finds a
derivation (a counterexample) or an inductive invariant (a proof).

## A loop that never reaches 7

``` text
x := 0
while (*) x := x + 2
assert(x != 7)
```

The reachable values of `x` form a relation `inv`:

``` r

inv <- z3_function(ctx, "inv", I, B)
x <- z3_int(ctx, "x")
fp <- z3_fixedpoint(ctx)
z3_register_relation(fp, inv)
z3_add_rule(fp, inv(0), name = "init")
z3_add_rule(fp, z3_forall(x, z3_implies(inv(x), inv(x + 2))), name = "step")
z3_query(fp, z3_exists(x, inv(x) & x == 7))
#> [1] "unsat"
```

`"unsat"`: 7 is not reachable. The answer is an invariant that proves
it:

``` r

z3_answer(fp)
#> <z3_expr Bool> (forall ((A Int))
#>   (! (= (inv A)
#>         (and (not (= A 7))
#>              (not (= A 5))
#>              (not (= A 3))
#>              (not (= A 1))
#>              (not (<= A (- 1)))))
#>      :weight 0))
```

8 is reachable, and the answer is the derivation:

``` r

z3_query(fp, z3_exists(x, inv(x) & x == 8))
#> [1] "sat"
z3_answer_rule_names(fp)
#> [1] "<null>;step;step;step;step;init"
```

## A two-variable program

``` text
x := 0; y := 0
while (*) { x := x + 1; y := y + 2 }
assert(y == 2 * x)
```

``` r

y <- z3_int(ctx, "y")
x2 <- z3_int(ctx, "x2")
y2 <- z3_int(ctx, "y2")
loop <- z3_function(ctx, "loop", list(I, I), B)
fp <- z3_fixedpoint(ctx)
z3_register_relation(fp, loop)
z3_add_rule(fp, loop(0, 0))
z3_add_rule(fp, z3_forall(list(x, y, x2, y2),
  z3_implies(loop(x, y) & x2 == x + 1 & y2 == y + 2, loop(x2, y2))))
z3_query(fp, z3_exists(list(x, y), loop(x, y) & y != 2 * x))
#> [1] "unsat"
z3_answer(fp)
#> <z3_expr Bool> (forall ((A Int) (B Int))
#>   (! (= (loop A B)
#>         (and (not (<= (+ (* 2 A) (* (- 1) B)) (- 1)))
#>              (not (>= (+ (* 2 A) (* (- 1) B)) 1))))
#>      :weight 0))
```

## Datalog

With the Datalog engine and finite domains, relations are computed
bottom-up:

``` r

N <- z3_finite_domain_sort(ctx, "Node", 16)
edge <- z3_function(ctx, "edge", list(N, N), B)
path <- z3_function(ctx, "path", list(N, N), B)
a <- z3_const(ctx, "a", N)
b <- z3_const(ctx, "b", N)
c <- z3_const(ctx, "c", N)
fp <- z3_fixedpoint(ctx)
z3_set_params(fp, engine = "datalog")
z3_register_relation(fp, edge)
z3_register_relation(fp, path)
for (e in list(c(1, 2), c(2, 3), c(3, 4), c(5, 6))) z3_add_fact(fp, edge, e)
z3_add_rule(fp, z3_forall(list(a, b), z3_implies(edge(a, b), path(a, b))))
z3_add_rule(fp, z3_forall(list(a, b, c), z3_implies(edge(a, b) & path(b, c), path(a, c))))
z3_query(fp, path)
#> [1] "sat"
z3_answer(fp)
#> <z3_expr Bool> (or (and (= (:var 0) 1) (= (:var 1) 2))
#>     (and (= (:var 0) 2) (= (:var 1) 3))
#>     (and (= (:var 0) 3) (= (:var 1) 4))
#>     (and (= (:var 0) 5) (= (:var 1) 6))
#>     (and (= (:var 0) 1) (= (:var 1) 3))
#>     (and (= (:var 0) 2) (= (:var 1) 4))
#>     (and (= (:var 0) 1) (= (:var 1) 4)))
```

## Model-based projection

Spacer’s building blocks are available on their own. Model-based
projection removes variables from a formula, guided by a model: the
result holds in the model and implies the formula with the variables
existentially quantified. Here the model has `x = 3, y = 4`, and
projecting `x` out of `x > 2 & y == x + 1` leaves a constraint on `y`:

``` r

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
```

``` r

w <- z3_model_project(m, x, f, method = "witness")
w$formula
#> <z3_expr Bool> (and (>= y 4))
w$terms
#> [[1]]
#> <z3_expr Int> (+ (- 1) y)
```

The witness says how to recover `x` from `y`.
[`z3_model_extrapolate()`](https://adamrsophie.github.io/z3r/reference/z3_model_project.md)
generalizes the model into a conjunction of literals that implies a
formula, and
[`z3_qe_lite()`](https://adamrsophie.github.io/z3r/reference/z3_model_project.md)
eliminates the variables that cheap rewriting can remove:

``` r

z3_model_extrapolate(m, f | y < 0)
#> <z3_expr Bool> (and (= y (+ x 1)) (> x 2))
z3_qe_lite(x, f)
#> $formula
#> <z3_expr Bool> (> (+ (- 1) y) 2)
#> 
#> $remaining
#> list()
```

## More

- [`z3_fixedpoint_rules()`](https://adamrsophie.github.io/z3r/reference/z3_fixedpoint.md),
  `z3_param_help(fp)`, `z3_statistics(fp)`.
- Spacer:
  [`z3_ground_answer()`](https://adamrsophie.github.io/z3r/reference/z3_fixedpoint.md),
  [`z3_answer_trace()`](https://adamrsophie.github.io/z3r/reference/z3_fixedpoint.md),
  [`z3_add_invariant()`](https://adamrsophie.github.io/z3r/reference/z3_fixedpoint.md),
  [`z3_reachable()`](https://adamrsophie.github.io/z3r/reference/z3_fixedpoint.md),
  [`z3_cover()`](https://adamrsophie.github.io/z3r/reference/z3_fixedpoint.md)
  and
  [`z3_add_cover()`](https://adamrsophie.github.io/z3r/reference/z3_fixedpoint.md);
  `z3_query(fp, q, level = k)` starts the search at level `k`.
- Rules in SMT-LIB2 Horn format can be given to a solver created with
  `z3_solver(ctx, logic = "HORN")`.
