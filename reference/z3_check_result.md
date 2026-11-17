# Check and record the result

`z3_check_result()` checks a solver or optimizer like
[`z3_check()`](https://adamrsophie.github.io/z3r/reference/z3_check.md)
and returns everything about the answer in one object, for batches and
reports that should not rebuild it from the solver's changing state:

## Usage

``` r
z3_check_result(x, ..., timeout = NULL)

z3_build_info()
```

## Arguments

- x:

  A `z3_solver` or `z3_optimize`.

- ...:

  Assumptions, as for
  [`z3_check()`](https://adamrsophie.github.io/z3r/reference/z3_check.md).

- timeout:

  Timeout for this check in milliseconds, or `NULL`.

## Value

`z3_check_result()` returns an object of class `z3_check_result`, a list
with the elements above. `z3_build_info()` returns a list.

## Details

- `status`: `"sat"`, `"unsat"` or `"unknown"`, and `reason` (why Z3 gave
  up, for `"unknown"`, else `NULL`).

- `model` (for `"sat"`), `core` (the unsatisfiable core, for `"unsat"`)
  and `proof` (for `"unsat"` in a context made with
  `z3_context(proof = TRUE)`); each is `NULL` where it does not apply.

- `objectives`: for an optimizer, its objective terms.

- `seconds`: the wall-clock time of the check.

- `statistics`: Z3's statistics after the check.

- `params`: the parameters set on the solver, as Z3's text.

- `fingerprint`: a structural hash of the assertions, tracked labels and
  assumptions, and for an optimizer its objectives and soft constraints.
  Two checks of the same formulas have the same fingerprint in one Z3
  version; it is not a cryptographic hash. It is `NA` when z3r cannot
  list the assertions (a solver with a simplifier that SMT-LIB was
  parsed into).

- `build`: `z3_build_info()`.

`z3_build_info()` reports how z3r was built and which Z3 it runs with:
the Z3 version (headers and loaded library), where configure found Z3
(`provider` is `"download"` for the pinned release configure downloads,
else `"system"`; `source` says how it was found), the downloaded archive
and its SHA-256 checksum, the path of the loaded Z3 library and the
compiler.

## Examples

``` r
ctx <- z3_context()
x <- z3_int(ctx, "x")
s <- z3_solver(ctx)
z3_add(s, x > 2, x < 5)
r <- z3_check_result(s)
r
#> <z3_check_result: sat in 0.001 s>
#> model: 1 constants
#> fingerprint: be02bf95c26d6fe9 
#> Z3 5.4.1.0 (system)
r$model$x
#> <z3_expr Int> 3
r$fingerprint
#> [1] "be02bf95c26d6fe9"
z3_build_info()$provider
#> [1] "system"
```
