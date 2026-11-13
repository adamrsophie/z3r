# Recursive functions

`z3_rec_function()` declares a function whose definition may refer to
itself. Define it with `z3_rec_define(f, args, body)`, where `args` are
constants standing for the parameters. Z3 unfolds the definition on
demand during solving. Unfolding need not terminate (Z3 4.8 releases
search `fact(x) == 120` in the example without end), so give such checks
a time limit.

## Usage

``` r
z3_rec_function(ctx, name, domain, range)

z3_rec_define(f, args, body)
```

## Arguments

- ctx:

  A
  [`z3_context()`](https://adamrsophie.github.io/z3r/reference/z3_context.md).

- name:

  Name of the function.

- domain:

  A sort or a list of argument sorts.

- range:

  Result sort.

- f:

  A function from `z3_rec_function()`.

- args:

  A constant or list of constants, one per argument.

- body:

  The function body.

## Value

`z3_rec_function()` returns a `z3_function`; `z3_rec_define()` returns
`f` invisibly.

## Examples

``` r
ctx <- z3_context()
I <- z3_int_sort(ctx)
fact <- z3_rec_function(ctx, "fact", I, I)
n <- z3_int(ctx, "n")
z3_rec_define(fact, n, z3_ite(n <= 0, 1, n * fact(n - 1)))
s <- z3_solver(ctx)
x <- z3_int(ctx, "x")
z3_add(s, fact(x) == 120)
# Unfolding need not terminate, so the check gets a time limit.
if (z3_check(s, timeout = 10000) == "sat") z3_eval(z3_model(s), x)
#> <z3_expr Int> 5
```
