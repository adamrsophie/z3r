# Create a Z3 context

A context owns every Z3 object (sorts, expressions, solvers, models)
created from it. Objects from different contexts cannot be combined.

## Usage

``` r
z3_context(..., concurrent_dec_ref = FALSE)
```

## Arguments

- ...:

  Named configuration parameters.

- concurrent_dec_ref:

  `TRUE` to enable Z3's concurrent reference counting for this context
  (see Details); must be given by name.

## Value

An object of class `z3_context`.

## Details

The context stays alive as long as any object created from it is alive,
even after the context variable itself has been removed, so objects can
be garbage collected in any order. Z3 contexts are not thread safe; use
each context from a single R process.

Z3 objects are external pointers. They cannot be saved with
[`saveRDS()`](https://rdrr.io/r/base/readRDS.html) or
[`save()`](https://rdrr.io/r/base/save.html) and restored in another
session; a restored object reports that it is no longer valid. Use
[`z3_to_smtlib()`](https://adamrsophie.github.io/z3r/reference/z3_to_smtlib.md)
to store constraints.

Configuration parameters fix properties of the context for its whole
life. The most useful are `proof = TRUE` (needed for
[`z3_proof()`](https://adamrsophie.github.io/z3r/reference/z3_proof.md)),
`unsat_core`, `timeout` (milliseconds, for every solver of the context),
`auto_config`, `well_sorted_check` and `model_validate`. Values are R
scalars of the parameter's type (`TRUE`/`FALSE`, a whole number, or a
string); unknown names and values of the wrong type are errors.

`concurrent_dec_ref = TRUE` makes Z3 queue reference count decrements
and apply them the next time the context creates an object or takes a
reference to one, so that other threads could release objects safely
(`Z3_enable_concurrent_dec_ref`, Z3 4.10.0 or later). z3r releases
objects only from R's main thread, so this only delays when Z3 frees
memory.

## See also

[`z3_version()`](https://adamrsophie.github.io/z3r/reference/z3_version.md)

## Examples

``` r
ctx <- z3_context()
ctx
#> <z3_context>
pctx <- z3_context(proof = TRUE)
```
