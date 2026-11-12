# Dictionaries keyed by Z3 terms

An AST map stores Z3 expressions under Z3 expression keys. Z3 shares
structurally equal terms, so `x + 1` built twice is the same key (but
`1 + x` is another one). Keys and values must be expressions of the
map's context; R numbers are not converted, since a number could stand
for an Int or a Real.

## Usage

``` r
z3_ast_map(ctx)

z3_ast_map_insert(map, key, value)

z3_ast_map_get(map, key)

z3_ast_map_contains(map, key)

z3_ast_map_erase(map, key)

z3_ast_map_reset(map)

z3_ast_map_keys(map)

z3_ast_map_values(map)

# S3 method for class 'z3_ast_map'
length(x)

# S3 method for class 'z3_ast_map'
format(x, ...)
```

## Arguments

- ctx:

  A
  [`z3_context()`](https://adamrsophie.github.io/z3r/reference/z3_context.md).

- map, x:

  A `z3_ast_map`.

- key, value:

  Expressions (`z3_expr`) of the map's context.

- ...:

  Unused.

## Value

`z3_ast_map()` returns a new `z3_ast_map`. `z3_ast_map_insert()`,
`z3_ast_map_erase()` and `z3_ast_map_reset()` return `map` invisibly.
`z3_ast_map_get()` returns a `z3_expr` or `NULL`,
`z3_ast_map_contains()` `TRUE` or `FALSE`,
[`length()`](https://rdrr.io/r/base/length.html) an integer,
`z3_ast_map_keys()` and `z3_ast_map_values()` lists of expressions, and
[`format()`](https://rdrr.io/r/base/format.html) a string.

## Details

- `z3_ast_map(ctx)` creates an empty map.

- `z3_ast_map_insert(map, key, value)` stores `value` under `key`,
  replacing any earlier value.

- `z3_ast_map_get(map, key)` returns the value stored under `key`, or
  `NULL` when there is none; `z3_ast_map_contains(map, key)` tests for
  the key.

- `z3_ast_map_erase(map, key)` removes a key (nothing happens when it is
  absent); `z3_ast_map_reset(map)` removes every key.

- [`length()`](https://rdrr.io/r/base/length.html) counts the keys.
  `z3_ast_map_keys()` and `z3_ast_map_values()` list the keys and the
  values; for an unchanged map both come in the same order.

- [`format()`](https://rdrr.io/r/base/format.html),
  [`print()`](https://rdrr.io/r/base/print.html) and
  [`as.character()`](https://rdrr.io/r/base/character.html) show the map
  as Z3 prints it.

A map is changed in place: every copy of a `z3_ast_map` object refers to
the same map.

## Examples

``` r
ctx <- z3_context()
x <- z3_int(ctx, "x")
y <- z3_int(ctx, "y")
m <- z3_ast_map(ctx)
z3_ast_map_insert(m, x + 1, y)
z3_ast_map_insert(m, x, z3_int_val(ctx, 3))
z3_ast_map_get(m, x + 1)
#> <z3_expr Int> y
z3_ast_map_contains(m, y)
#> [1] FALSE
length(m)
#> [1] 2
m
#> <z3_ast_map>
#> (ast-map
#>   ((+ x 1)
#>    y)
#>   (x
#>    3))
z3_ast_map_erase(m, x)
z3_ast_map_keys(m)
#> [[1]]
#> <z3_expr Int> (+ x 1)
#> 
```
