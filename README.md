# z3r: Native R Interface to the Z3 Theorem Prover

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

* Integers of unbounded size, exact rationals, Booleans, bit-vectors, arrays,
  Unicode strings, uninterpreted sorts and functions.
* R operators (`+ - * / ^ %/% %% == != < <= > >= & | !`) with strict sort
  checking: Booleans are not numbers, bit-vectors are never mixed with
  unbounded integers, and R's recycling rules are never applied to formulas.
* Solving with `"sat"`, `"unsat"` and `"unknown"` kept distinct, models,
  incremental solving with push/pop and assumptions, tracked assertions and
  unsatisfiable cores, timeouts, deterministic resource limits, statistics.
* Optimization: minimize, maximize, weighted soft constraints, multiple
  objectives, with unbounded and non-attained optima reported as such.
* Quantifiers, tactics and goals, and SMT-LIB2 import and export.
* Exact conversion of results: values come back as doubles only when exactly
  representable, otherwise as `gmp` numbers or exact strings.

`documentation/` is not part of the repository; see `NEWS.md` and the
vignettes for what is implemented.

## Installation

z3r needs the Z3 C library and headers, version 4.8.12 or later. Nothing is
downloaded during installation.

### macOS

```sh
brew install z3
```

```r
remotes::install_github("adamrsophie/z3r")
```

Verified with Homebrew Z3 4.16.0 and with the official Z3 5.3.3 release
archive on Apple silicon.

### Linux

```sh
sudo apt-get install libz3-dev    # Debian, Ubuntu
sudo dnf install z3-devel         # Fedora
```

```r
remotes::install_github("adamrsophie/z3r")
```

Verified on Ubuntu (arm64) with the distribution's `libz3-dev` 4.13.3.

### Windows

Download an official release such as `z3-4.16.0-x64-win.zip` from the
[Z3 releases page](https://github.com/Z3Prover/z3/releases), unpack it, and
point z3r at the unpacked folder:

```r
Sys.setenv(Z3_DIR = "C:/path/to/z3-4.16.0-x64-win")
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

* `vignette("z3r")`: SMT solving for R users, exact numbers, incremental
  solving, cores, optimization, and a scheduling example.
* `vignette("theories")`: bit-vectors (including proving a bit trick correct
  for all inputs), arrays, strings, functions, quantifiers, tactics, SMT-LIB2.
* `?z3_operators`, `?z3_int_val`: operator semantics and numeric conversion.

## License

GPL-3. Z3 itself is distributed by Microsoft under the MIT license.
