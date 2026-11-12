# SMT-LIB2, inspecting expressions and moving between contexts

``` r

library(z3r)
ctx <- z3_context()
x <- z3_int(ctx, "x")
y <- z3_int(ctx, "y")
```

## SMT-LIB2

SMT-LIB2 is the standard text format of SMT solvers. z3r writes and
reads it with Z3’s own printer and parser.

``` r

s <- z3_solver(ctx)
z3_add(s, x > 2, x < 5, x + y == 10)
txt <- z3_to_smtlib(s)
cat(txt)
#> ; 
#> (set-info :status unknown)
#> (declare-fun x () Int)
#> (declare-fun y () Int)
#> (assert
#>  (> x 2))
#> (assert
#>  (< x 5))
#> (assert
#>  (let ((?x11 (+ x y)))
#> (= ?x11 10)))
#> (check-sat)
```

Parsing returns the asserted formulas. Declared names map to the same
constants as z3r’s constructors:

``` r

fs <- z3_from_smtlib(ctx, txt)
fs
#> [[1]]
#> <z3_expr Bool> (> x 2)
#> 
#> [[2]]
#> <z3_expr Bool> (< x 5)
#> 
#> [[3]]
#> <z3_expr Bool> (= (+ x y) 10)
z3_identical(fs[[1]], x > 2)
#> [1] TRUE
```

Text can go straight into a solver or optimizer, and files are read with
[`z3_read_smtlib()`](https://adamrsophie.github.io/z3r/reference/z3_to_smtlib.md):

``` r

s2 <- z3_solver(ctx)
z3_add_smtlib(s2, "(declare-const k Int) (assert (= (* 3 k) 21))")
z3_check(s2)
#> [1] "sat"
z3_model(s2)
#> <z3_model>
#>   k = 7
```

[`z3_eval_smtlib()`](https://adamrsophie.github.io/z3r/reference/z3_to_smtlib.md)
runs commands in Z3’s interpreter, exactly as the `z3` command-line tool
would, so any SMT-LIB2 example from the Z3 guide runs from R:

``` r

cat(z3_eval_smtlib(ctx, "
  (declare-datatypes () ((Color red green blue)))
  (declare-const c Color)
  (assert (not (= c red)))
  (check-sat)
  (get-value (c))
"))
#> sat
#> ((c green))
```

A list of formulas exports as a benchmark with a logic header:

``` r

cat(z3_to_smtlib(list(x > 1, y < x), logic = "QF_LIA"))
#> ; 
#> (set-info :status unknown)
#> (set-logic QF_LIA)
#> (declare-fun x () Int)
#> (declare-fun y () Int)
#> (assert
#>  (> x 1))
#> (assert
#>  (< y x))
#> (check-sat)
```

## Inspecting expressions

``` r

e <- x + 2 * y
z3_kind(e)
#> [1] "app"
z3_decl_info(z3_decl(e))[c("name", "kind", "interpreted")]
#> $name
#> [1] "+"
#> 
#> $kind
#> [1] "add"
#> 
#> $interpreted
#> [1] TRUE
z3_args(e)
#> [[1]]
#> <z3_expr Int> x
#> 
#> [[2]]
#> <z3_expr Int> (* 2 y)
z3_decl_info(z3_decl(z3_extract(z3_bv(ctx, "b", 8), 5, 2)))$parameters
#> [[1]]
#> [1] 5
#> 
#> [[2]]
#> [1] 2
```

A recursive walk turns expressions into other representations. A small
printer that writes infix arithmetic:

``` r

infix <- function(e) {
  k <- z3_kind(e)
  if (k == "numeral") return(format(e))
  info <- z3_decl_info(z3_decl(e))
  args <- vapply(z3_args(e), infix, "")
  if (!info$interpreted) return(info$name)
  op <- switch(info$kind, add = "+", sub = "-", mul = "*", le = "<=", lt = "<",
               ge = ">=", gt = ">", eq = "==", NA)
  if (is.na(op)) return(format(e))
  paste0("(", paste(args, collapse = paste0(" ", op, " ")), ")")
}
infix(x + 2 * y <= 10)
#> [1] "((x + (2 * y)) <= 10)"
```

Quantifiers expose their bound variables and body, in which bound
variables appear as de Bruijn indices:

``` r

q <- z3_forall(list(x, y), x + y >= x)
info <- z3_quantifier_info(q)
info$names
#> [1] "x" "y"
info$body
#> <z3_expr Bool> (>= (+ (:var 1) (:var 0)) (:var 1))
```

Walks that meet the same subterm many times can remember results in
[`z3_ast_map()`](https://adamrsophie.github.io/z3r/reference/z3_ast_map.md),
a dictionary keyed by Z3 terms. Equal terms are the same key, because Z3
shares structurally equal terms:

``` r

seen <- z3_ast_map(ctx)
count <- function(e) {
  if (z3_ast_map_contains(seen, e)) return(invisible())
  z3_ast_map_insert(seen, e, z3_int_val(ctx, length(z3_args(e))))
  for (a in z3_args(e)) count(a)
}
count((x + y) * (x + y) + y)
length(seen)
#> [1] 5
z3_ast_map_get(seen, x + y)
#> <z3_expr Int> 2
```

## Substitution

``` r

z3_substitute(x + y, x, 3)
#> <z3_expr Int> (+ 3 y)
z3_simplify(z3_substitute(x * y, list(x, y), list(2, 5)))
#> <z3_expr Int> 10
v0 <- z3_bound_var(ctx, 0, z3_int_sort(ctx))
z3_substitute_vars(v0 * v0 + 1, list(y))
#> <z3_expr Int> (+ (* y y) 1)
```

From Z3 4.9.0,
[`z3_substitute_funs()`](https://adamrsophie.github.io/z3r/reference/z3_substitute.md)
replaces a function by a definition:

``` r

f <- z3_function(ctx, "f", z3_int_sort(ctx), z3_int_sort(ctx))
z3_simplify(z3_substitute_funs(f(x) + f(2), f, v0 * v0))
#> <z3_expr Int> (+ 4 (* x x))
```

## Contexts and translation

Objects from different contexts never mix.
[`z3_translate()`](https://adamrsophie.github.io/z3r/reference/z3_translate.md)
copies expressions, solvers, optimizers, models and goals into another
context:

``` r

other <- z3_context()
x + z3_int(other, "x")
#> Error:
#> ! Z3 objects belong to different contexts; objects can only be combined within the context that created them
xt <- z3_translate(x + 1, other)
xt + z3_int(other, "x")
#> <z3_expr Int> (+ x 1 x)
st <- z3_translate(s, other)
z3_check(st)
#> [1] "sat"
z3_translate(list(a = x + 1, b = y), other)
#> $a
#> <z3_expr Int> (+ x 1)
#> 
#> $b
#> <z3_expr Int> y
```

## Global settings and logging

``` r

z3_global_param_get("model.completion")
#> [1] "false"
if (z3_version_at_least("4.15.0")) head(z3_global_param_help(ctx)[, c("name", "kind")], 4)
#>                       name kind
#> 1                    proof bool
#> 2        smtlib2_compliant bool
#> 3 memory_high_watermark_mb uint
#> 4                    stats bool
```

`z3_log_open(file)` records every Z3 API call made by z3r, for
reproducing a problem in a bug report.
