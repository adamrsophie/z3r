# z3r <img src="man/figures/logo.png" align="right" height="139" alt="z3r logo" />

**Native R Interface to the Z3 Theorem Prover**

<!-- badges: start -->
[![R-CMD-check](https://github.com/adamrsophie/z3r/actions/workflows/R-CMD-check.yaml/badge.svg)](https://github.com/adamrsophie/z3r/actions/workflows/R-CMD-check.yaml)
<!-- badges: end -->

z3r gives R direct access to [Z3](https://github.com/Z3Prover/z3), the
satisfiability modulo theories (SMT) solver, through Z3's C API. There is no
Python, no reticulate, and no separate Z3 process: constraints are Z3 objects
in memory, built with ordinary R operators.

```r
library(z3r)

ctx <- z3_context()
x <- z3_int(ctx, "x")
y <- z3_int(ctx, "y")

s <- z3_solver(ctx)
z3_add(s, x + y == 10, x > 2 * y, y > 0)
z3_check(s)
#> [1] "sat"
z3_model(s)
#> <z3_model>
#>   y = 1
#>   x = 9
```

z3r is an independent third-party package. It is not developed, endorsed or
supported by Microsoft or by the Z3 team.

## What it covers

z3r targets Z3 5.4.1 and builds against every release from 4.8.13. CI
runs the full test suite on one release from each series: Z3 4.8.13,
4.8.17, 4.9.1, 4.10.2, 4.11.2, 4.12.6, 4.13.3, 4.14.1, 4.15.8, 4.16.0,
5.0.0, 5.1.0, 5.3.3 and 5.4.1. A monthly workflow runs it on all 43
releases from 4.8.13 to 5.4.1.

* **Theories:** unbounded integers, exact reals and algebraic numbers,
  Booleans, bit-vectors, IEEE floating point, arrays, sets, strings,
  sequences, regular expressions, characters, algebraic datatypes,
  uninterpreted and recursive functions, quantifiers, special relations,
  pseudo-Boolean constraints, and the complex numbers, finite fields and
  finite sets of Z3 5.
* **Natural syntax:** R operators (`+ - * / ^ %/% %% == != < <= > >= & | !`)
  with strict sort checking. Booleans are not numbers, bit-vectors never mix
  with unbounded integers, and R's recycling rules never apply to formulas.
* **Solving:** `"sat"`, `"unsat"` and `"unknown"` kept distinct; models;
  push/pop and assumptions; unsatisfiable cores; proofs; consequences;
  cubes; timeouts and deterministic resource limits; Ctrl-C.
* **Optimization:** objectives, weighted soft constraints, priorities,
  unbounded and non-attained optima reported as such.
* **Engines and tools:** tactics, probes, simplifiers, constrained Horn
  clauses (Spacer) and Datalog, real closed fields, user propagators written
  in R, SMT-LIB2 import and export, and expression inspection and
  substitution.
* **Exact results:** values come back as doubles only when exactly
  representable, otherwise as `gmp` numbers or exact strings.
* **Interoperability:** exact translation to and from the
  [symcompR](https://github.com/adamrsophie/symcompR) computer algebra
  system.

## Installation

z3r needs the Z3 C library and headers, version 4.8.13 or later (the first
release with Z3's Unicode string API). Nothing is downloaded during
installation. Ubuntu 22.04 and 24.04 and Debian 12 ship Z3 4.8.12, which is
too old; use a newer distribution package (Debian 13 and Ubuntu 26.04 ship
4.13.3) or an official release archive there.
Features that need a newer Z3 than the one z3r was built with report so
when they are called.

### Any platform: an official release

Download the archive for your platform from the
[Z3 releases page](https://github.com/Z3Prover/z3/releases) (for example
`z3-5.4.1-arm64-osx-13.3.zip`), unpack it, and point z3r at it:

```sh
Z3_INCLUDE_DIR=/path/to/z3-5.4.1/include Z3_LIB_DIR=/path/to/z3-5.4.1/bin \
  R -e 'remotes::install_github("adamrsophie/z3r")'
```

### macOS

```sh
brew install z3
```

```r
remotes::install_github("adamrsophie/z3r")
```

Verified with the official Z3 5.4.1 and 5.3.3 archives and with Homebrew's
Z3 4.16.0 on Apple silicon.

### Linux

```sh
sudo apt-get install libz3-dev    # Debian 13, Ubuntu 26.04 or later
sudo dnf install z3-devel         # Fedora
```

```r
remotes::install_github("adamrsophie/z3r")
```

Verified on Ubuntu (arm64) with the distribution's `libz3-dev` 4.13.3.

### Windows

Download an official release such as `z3-5.4.1-x64-win.zip` from the
[Z3 releases page](https://github.com/Z3Prover/z3/releases), unpack it, and
point z3r at the unpacked folder:

```r
Sys.setenv(Z3_DIR = "C:/path/to/z3-5.4.1-x64-win")
remotes::install_github("adamrsophie/z3r")
```

Rtools links against `libz3.dll` directly, and the DLL is copied next to
z3r's own DLL. The Windows build is written and covered by the CI workflow,
but it has not been run on Windows yet.

### Choosing a specific Z3

The configure script looks, in order, at `PKG_CPPFLAGS`/`PKG_LIBS`, at
`Z3_INCLUDE_DIR`/`Z3_LIB_DIR` (or `Z3_DIR` on Windows), at `pkg-config z3`,
and at the usual prefixes (`/opt/homebrew`, `/usr/local`, `/usr`). It links
the chosen library by its full path and refuses headers and libraries of
different versions:

```sh
Z3_INCLUDE_DIR=/opt/z3/include Z3_LIB_DIR=/opt/z3/bin R CMD INSTALL z3r
```

`z3_version()` reports the library z3r runs with and the headers it was
compiled against.

### CRAN

z3r is not on CRAN. A CRAN release needs a Z3 binary for CRAN's Windows and
macOS builders, which requires coordinating with the CRAN maintainers of the
build toolchains (Rtools and the macOS recipes); this has not been done.

## Learning more

The [website](https://adamrsophie.github.io/z3r/) has the full reference and
seventeen articles, from `vignette("z3r")` (SMT solving for R users) to one
article per theory and engine: exact numbers, solving, optimization, logic
puzzles, bit-vectors, floating point, arrays and sets, strings and regular
expressions, datatypes, functions and quantifiers, tactics, Horn clauses,
algebraic numbers, SMT-LIB2 and inspection, user propagators, the Z3 5
theories, and symcompR.

## License

GPL-3. Z3 itself is distributed by Microsoft under the MIT license.
