# Build and query models

`z3_model(ctx)` creates an empty model. `z3_model_set()` assigns a value
to a constant; `z3_model_set_function()` defines a function by a table
of entries and a default value. `z3_model_set_default()` replaces the
default (`else`) value of a function the model already interprets and
keeps its entries; it works on models returned by solvers too. The
default value of a function may use
[`z3_bound_var()`](https://adamrsophie.github.io/z3r/reference/z3_inspect.md)
`i` for argument `i + 1`. A hand-made model can be used with
[`z3_eval()`](https://adamrsophie.github.io/z3r/reference/z3_eval.md) to
evaluate expressions under chosen values.

## Usage

``` r
z3_model_set(model, constant, value)

z3_model_set_function(model, f, entries = list(), otherwise)

z3_model_set_default(model, f, otherwise)

z3_model_has(model, x)

z3_model_universes(model)

z3_as_array_function(x)
```

## Arguments

- model:

  A `z3_model`.

- constant:

  A constant (`z3_expr`).

- value:

  A value of the constant's sort.

- f:

  A `z3_function` or `z3_func_decl`.

- entries:

  A list of entries, each `list(args = list(...), value = v)`.

- otherwise:

  The value for all other arguments.

- x:

  A constant, function, or (for `z3_as_array_function()`) an array
  value.

## Value

`z3_model_set()`, `z3_model_set_function()` and `z3_model_set_default()`
return `model` invisibly; the others return the requested information.

## Details

`z3_model_has()` tests whether a model interprets a constant or
function. `z3_model_universes()` returns, for each uninterpreted sort of
the model, the finite list of values the model uses for it.
`z3_as_array_function()` returns the function behind an array value of
the form `(_ as-array f)`, or `NULL`.

## Examples

``` r
ctx <- z3_context()
x <- z3_int(ctx, "x")
f <- z3_function(ctx, "f", z3_int_sort(ctx), z3_int_sort(ctx))
m <- z3_model(ctx)
z3_model_set(m, x, 5)
z3_model_set_function(m, f, list(list(args = list(5), value = 50)), 0)
z3_eval(m, f(x) + f(1))
#> <z3_expr Int> 50
z3_model_set_default(m, f, z3_bound_var(ctx, 0, z3_int_sort(ctx)) * 2)
z3_eval(m, f(x) + f(1))
#> <z3_expr Int> 52
```
