# Functions, quantifiers and special relations

``` r

library(z3r)
ctx <- z3_context()
I <- z3_int_sort(ctx)
B <- z3_bool_sort(ctx)
x <- z3_int(ctx, "x")
y <- z3_int(ctx, "y")
```

## Uninterpreted functions

An uninterpreted function has typed arguments and a result, and nothing
else is known about it except congruence: equal arguments give equal
results. That is often all an argument needs, and it lets Z3 abstract
away details.

``` r

f <- z3_function(ctx, "f", I, I)
f
#> <z3_function> (declare-fun f (Int) Int)
s <- z3_solver(ctx)
z3_add(s, x == y, f(x) != f(y))
z3_check(s)
#> [1] "unsat"
```

Models give a table and a default for the function:

``` r

s <- z3_solver(ctx)
z3_add(s, f(1) == 10, f(2) == 20, f(f(1)) == 1)
z3_check(s)
#> [1] "sat"
z3_model(s)
#> <z3_model>
#>   f(2) = 20
#>   f(10) = 1
#>   f(...) = 10 otherwise
z3_model_functions(z3_model(s))$f$entries[[1]]
#> $args
#> $args[[1]]
#> <z3_expr Int> 2
#> 
#> 
#> $value
#> <z3_expr Int> 20
```

Functions over uninterpreted sorts describe abstract structures:

``` r

G <- z3_uninterpreted_sort(ctx, "G")
op <- z3_function(ctx, "op", list(G, G), G)
e <- z3_const(ctx, "e", G)
a <- z3_const(ctx, "a", G)
b <- z3_const(ctx, "b", G)
c <- z3_const(ctx, "c", G)
s <- z3_solver(ctx)
z3_add(s,
  z3_forall(list(a, b, c), op(op(a, b), c) == op(a, op(b, c))),
  z3_forall(a, op(e, a) == a & op(a, e) == a),
  z3_forall(a, op(a, a) == e))
# In a group where every element is its own inverse, op is commutative:
z3_add(s, !z3_forall(list(a, b), op(a, b) == op(b, a)))
z3_check(s)
#> [1] "unsat"
```

## Quantifiers

[`z3_forall()`](https://adamrsophie.github.io/z3r/reference/z3_forall.md)
and
[`z3_exists()`](https://adamrsophie.github.io/z3r/reference/z3_forall.md)
bind ordinary constants. Inside the quantifier the constants are bound;
elsewhere they keep their meaning.

``` r

s <- z3_solver(ctx)
z3_add(s, z3_forall(x, f(x) >= x), f(3) < 3)
z3_check(s)
#> [1] "unsat"
s <- z3_solver(ctx)
z3_add(s, z3_exists(x, f(x) == 7 & x > 100))
z3_check(s)
#> [1] "sat"
```

Patterns tell Z3 when to instantiate a quantifier; weights and
identifiers tune and trace instantiation:

``` r

q <- z3_forall(x, f(f(x)) == x, patterns = list(list(f(x))), weight = 1, qid = "involution")
z3_quantifier_info(q)[c("type", "names", "weight")]
#> $type
#> [1] "forall"
#> 
#> $names
#> [1] "x"
#> 
#> $weight
#> [1] 1
```

Z3 itself represents bound variables by de Bruijn indices, and
[`z3_quantifier_db()`](https://adamrsophie.github.io/z3r/reference/z3_forall.md)
builds quantifiers and lambdas in that form directly:
`z3_bound_var(ctx, 0, I)` is the variable bound by the innermost binder.

``` r

v <- z3_bound_var(ctx, 0, I)
q <- z3_quantifier_db(list(I), "n", f(v) > v)
q
#> <z3_expr Bool> (forall ((n Int)) (! (> (f n) n) :weight 0))
s <- z3_solver(ctx)
z3_add(s, q, f(2) == 2)
z3_check(s)
#> [1] "unsat"
```

Quantified formulas are outside the fragments Z3 decides completely, so
`"unknown"` answers are possible.

## Recursive functions

Recursive definitions are unfolded on demand. Unfolding need not
terminate (Z3 4.8 releases search this example without end), so the
check gets a time limit:

``` r

fact <- z3_rec_function(ctx, "fact", I, I)
n <- z3_int(ctx, "n")
z3_rec_define(fact, n, z3_ite(n <= 0, 1, n * fact(n - 1)))
s <- z3_solver(ctx)
z3_add(s, fact(x) == 720)
status <- z3_check(s, timeout = 10000)
status
#> [1] "sat"
if (status == "sat") z3_eval(z3_model(s), x)
#> <z3_expr Int> 6
```

They combine with datatypes. The length of a list:

``` r

L <- z3_list_sort(ctx, "IntList", I)
len <- z3_rec_function(ctx, "len", L$sort, I)
xs <- z3_const(ctx, "xs", L$sort)
z3_rec_define(len, xs, z3_ite(L$is_nil(xs), 0, 1 + len(L$tail(xs))))
ys <- z3_const(ctx, "ys", L$sort)
s <- z3_solver(ctx)
z3_add(s, len(ys) == 3, L$head(ys) == 5)
status <- z3_check(s, timeout = 10000)
status
#> [1] "sat"
if (status == "sat") z3_eval(z3_model(s), ys)
#> <z3_expr IntList> (cons 5 (cons 4 (cons 8 nil)))
```

## Special relations

Orders and transitive closures have dedicated decision procedures:

``` r

U <- z3_uninterpreted_sort(ctx, "U")
u <- lapply(1:3, function(i) z3_const(ctx, paste0("u", i), U))
le <- z3_partial_order(U)
s <- z3_solver(ctx)
z3_add(s, le(u[[1]], u[[2]]), le(u[[2]], u[[3]]), !le(u[[1]], u[[3]]))
z3_check(s)
#> [1] "unsat"
reach <- z3_function(ctx, "edge", list(U, U), B)
path <- z3_transitive_closure(reach)
s <- z3_solver(ctx)
z3_add(s, reach(u[[1]], u[[2]]), reach(u[[2]], u[[3]]), !path(u[[1]], u[[3]]))
z3_check(s)
#> [1] "unsat"
```

[`z3_linear_order()`](https://adamrsophie.github.io/z3r/reference/z3_linear_order.md),
[`z3_tree_order()`](https://adamrsophie.github.io/z3r/reference/z3_linear_order.md)
and
[`z3_piecewise_linear_order()`](https://adamrsophie.github.io/z3r/reference/z3_linear_order.md)
are the other relations.
