# Global Z3 parameters and logging

Global parameters apply to the whole R process: every context created
afterwards, and some settings of existing contexts. Prefer per-solver
parameters
([`z3_set_params()`](https://adamrsophie.github.io/z3r/reference/z3_set_params.md))
or context configuration
([`z3_context()`](https://adamrsophie.github.io/z3r/reference/z3_context.md))
when they exist. Values are given as R scalars and passed to Z3 as
strings.

## Usage

``` r
z3_global_param_set(name, value)

z3_global_param_get(name)

z3_global_param_reset()

z3_global_param_help(ctx)

z3_log_open(file)

z3_log_close()

z3_warnings(enabled)
```

## Arguments

- name:

  Parameter name.

- value:

  A single logical, number or string.

- ctx:

  A
  [`z3_context()`](https://adamrsophie.github.io/z3r/reference/z3_context.md)
  used to read the descriptions.

- file:

  Path of the log file.

- enabled:

  `TRUE` or `FALSE`.

## Value

`z3_global_param_get()` returns a string; the others return their input
invisibly.

## Details

- `z3_global_param_set(name, value)` sets one parameter, for example
  `"smt.random_seed"` or `"verbose"`.

- `z3_global_param_get(name)` returns the current value as a string, or
  `NA` when the name is unknown.

- `z3_global_param_reset()` restores all defaults.

- `z3_global_param_help()` lists the global parameters (Z3 4.15.0 or
  later; in older releases Z3 frees its own parameter table when the
  list is released).

- `z3_log_open(file)`/`z3_log_close()` record every Z3 API call into a
  file for bug reports. `z3_warnings(enabled)` turns Z3's warning
  messages on or off.

## Examples

``` r
old <- z3_global_param_get("model.completion")
z3_global_param_set("model.completion", TRUE)
z3_global_param_get("model.completion")
#> [1] "true"
z3_global_param_set("model.completion", old)
```
