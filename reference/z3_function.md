# Uninterpreted functions

Declares a function symbol with typed arguments and result. Z3 treats it
as an arbitrary function: the only built-in fact is that equal arguments
give equal results.

## Usage

``` r
z3_function(ctx, name, domain, range)

z3_fresh_function(ctx, prefix, domain, range)
```

## Arguments

- ctx:

  A
  [`z3_context()`](https://adamrsophie.github.io/z3r/reference/z3_context.md).

- name:

  Name of the function.

- domain:

  A list of `z3_sort` objects, one per argument (or a single sort for a
  one-argument function).

- range:

  The result `z3_sort`.

- prefix:

  Prefix for the generated name of `z3_fresh_function()`.

## Value

A function of class `z3_function`. Its declaration is available with
[`z3_decl()`](https://adamrsophie.github.io/z3r/reference/z3_inspect.md).

## Details

The result is an R function, so `f(x)` builds the application. R values
passed as arguments are converted to the declared argument sorts.

## Examples

``` r
ctx <- z3_context()
I <- z3_int_sort(ctx)
f <- z3_function(ctx, "f", list(I), I)
x <- z3_int(ctx, "x")
s <- z3_solver(ctx)
z3_add(s, f(f(x)) == x, f(x) == 1, x != 1)
z3_check(s)
#> [1] "sat"
z3_model(s)
#> <z3_model>
#>   x = 0
#>   f(0) = 1
#>   f(...) = 0 otherwise
```
