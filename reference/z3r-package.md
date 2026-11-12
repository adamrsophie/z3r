# z3r: Native R Interface to the Z3 Theorem Prover

z3r binds R to the C API of the Z3 satisfiability modulo theories (SMT)
solver. Symbolic variables are created in a
[`z3_context()`](https://adamrsophie.github.io/z3r/reference/z3_context.md),
combined with ordinary R operators into constraints, and handed to a
[`z3_solver()`](https://adamrsophie.github.io/z3r/reference/z3_solver.md)
or a
[`z3_optimize()`](https://adamrsophie.github.io/z3r/reference/z3_optimize.md)
object.

## Details

z3r is an independent third-party package. It is not developed, endorsed
or supported by Microsoft or by the Z3 development team.

## Getting started

    ctx <- z3_context()
    x <- z3_int(ctx, "x")
    y <- z3_int(ctx, "y")
    s <- z3_solver(ctx)
    z3_add(s, x + y == 10, x > 2 * y, y > 0)
    z3_check(s)
    z3_model(s)

See
[`vignette("z3r")`](https://adamrsophie.github.io/z3r/articles/z3r.md)
for an introduction to SMT solving from R.

## See also

Useful links:

- <https://adamrsophie.github.io/z3r/>

- <https://github.com/adamrsophie/z3r>

- Report bugs at <https://github.com/adamrsophie/z3r/issues>

## Author

**Maintainer**: Adam R Sophie <adamrsophie@gmail.com> \[copyright
holder\]

Authors:

- Adam R Sophie <adamrsophie@gmail.com> \[copyright holder\]
