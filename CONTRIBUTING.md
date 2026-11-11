# Contributing to z3r

Bug reports and pull requests are welcome at
<https://github.com/adamrsophie/z3r/issues>.

## Reporting a bug

Please include:

* the output of `z3r::z3_version()` and `sessionInfo()`;
* a minimal example, ideally with the SMT-LIB2 text from `z3_to_smtlib()`;
* for a wrong answer, what the `z3` command-line tool returns for the same
  SMT-LIB2 text.

## Development setup

1. Install Z3 with its headers (see the README).
2. Install the development dependencies:
   `install.packages(c("Rcpp", "roxygen2", "testthat", "gmp", "knitr", "rmarkdown"))`.
3. After editing C++ export annotations or roxygen comments, run
   `Rcpp::compileAttributes()` and `roxygen2::roxygenise()`.
4. Run `R CMD build .` and `R CMD check --as-cran z3r_*.tar.gz`.

## Rules for native code

* Every Z3 handle is held through `z3r::Ref<T>` (see `src/z3r.h`), which takes
  a reference on construction and releases it on destruction, and keeps the
  context alive.
* Check the Z3 error state after every call that can fail (`own()`,
  `own_ast()` and `check()` do this) before using the result.
* Do the sort checking in z3r. Z3's C API silently coerces some ill-sorted
  terms, for example Booleans in arithmetic.
* Never call R from a thread other than the main thread, and never use a Z3
  context from two threads.

## Tests

Tests should check mathematical results, not only that calls succeed. Where
possible compare with an independent route: brute force over a small domain,
R's own arithmetic, or the `z3` command-line tool on exported SMT-LIB2.
