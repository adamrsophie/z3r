# SMT-LIB2 import and export

Exchange constraints with other SMT solvers and tools in the SMT-LIB 2
format, using Z3's own printer and parser.

## Usage

``` r
z3_to_smtlib(x, logic = "", state = FALSE)

z3_from_smtlib(ctx, text, sorts = list(), decls = list())

z3_read_smtlib(ctx, file, sorts = list(), decls = list())

z3_add_smtlib(x, text = NULL, file = NULL)

z3_eval_smtlib(ctx, text)
```

## Arguments

- x:

  A `z3_solver`, `z3_optimize` or `z3_fixedpoint`; for `z3_to_smtlib()`
  also a list of Boolean `z3_expr`.

- logic:

  SMT-LIB logic for the benchmark header.

- state:

  For a solver, write Z3's internal solver state instead of a benchmark.

- ctx:

  A
  [`z3_context()`](https://adamrsophie.github.io/z3r/reference/z3_context.md).

- text:

  SMT-LIB2 text, a single string (multiple strings are joined with
  newlines).

- sorts, decls:

  Named lists of `z3_sort` objects and of functions (`z3_function` or
  declarations), available to the text under those names.

- file:

  Path to an SMT-LIB2 file.

## Value

`z3_to_smtlib()` and `z3_eval_smtlib()` return a string;
`z3_from_smtlib()` and `z3_read_smtlib()` return a list of `z3_expr`;
`z3_add_smtlib()` returns `x` invisibly.

## Details

- `z3_to_smtlib()` writes a solver's assertions, or a list of Boolean
  expressions, as a standard SMT-LIB2 benchmark with the needed
  declarations and a final `(check-sat)`. For an optimizer it writes the
  assertions and objectives; for a fixedpoint engine its rules. With
  `state = TRUE`, a solver is written in Z3's own format, which after a
  check also records Z3's internal model conversion steps and is not
  portable to other solvers.

- `z3_from_smtlib()` parses SMT-LIB2 text and returns the asserted
  formulas as a list of `z3_expr`. Declarations in the text create
  constants in `ctx`; a declared constant is the same object as one made
  with the same name and sort, for example by
  [`z3_int()`](https://adamrsophie.github.io/z3r/reference/z3_const.md).
  Commands other than declarations and assertions (such as `check-sat`)
  are ignored. `sorts` and `decls` name existing sorts and functions
  that the text uses without declaring them, under names of your choice.

- `z3_read_smtlib()` does the same for a file.

- `z3_add_smtlib()` parses text or a file directly into a solver,
  optimizer or fixedpoint engine.

- `z3_eval_smtlib()` runs SMT-LIB2 commands in Z3's interpreter and
  returns its output text, exactly as the `z3` command-line tool would
  print it. Its declarations are kept in the context. Its `set-option`
  commands change Z3's parameters beyond this call (some of them for
  every context), and they stay changed.

Parse errors are reported as R errors.

## Examples

``` r
ctx <- z3_context()
x <- z3_int(ctx, "x")
s <- z3_solver(ctx)
z3_add(s, x > 2, x < 5)
txt <- z3_to_smtlib(s)
cat(txt)
#> ; 
#> (set-info :status unknown)
#> (declare-fun x () Int)
#> (assert
#>  (> x 2))
#> (assert
#>  (< x 5))
#> (check-sat)
fs <- z3_from_smtlib(ctx, txt)
s2 <- z3_solver(ctx)
z3_add(s2, fs)
z3_check(s2)
#> [1] "sat"
z3_eval_smtlib(ctx, "(declare-const y Int) (assert (> y 3)) (check-sat)")
#> [1] "sat\n"
```
