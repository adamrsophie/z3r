# Memory, logging and context parameters

`z3_memory_usage()` estimates the memory, in bytes, that Z3 has
allocated in this R process. `z3_log_append()` writes a comment into the
API log opened with
[`z3_log_open()`](https://adamrsophie.github.io/z3r/reference/z3_global_param_set.md).
`z3_update_param()` changes a configuration parameter of an existing
context.

## Usage

``` r
z3_memory_usage()

z3_log_append(text)

z3_update_param(ctx, name, value)
```

## Arguments

- text:

  Text for the log.

- ctx:

  A
  [`z3_context()`](https://adamrsophie.github.io/z3r/reference/z3_context.md).

- name, value:

  Parameter name and value.

## Value

A number, or the input invisibly.

## Examples

``` r
z3_memory_usage()
#> [1] 18281976
ctx <- z3_context()
z3_update_param(ctx, "timeout", 5000)
```
