# z3r 0.1.0

First release of z3r, a native R interface to the Z3 theorem prover.

z3r gives R direct access to Z3's C API: expressions over all of Z3's
theories, incremental solving, optimization, tactics, fixedpoint engines,
user propagators and SMT-LIB2. It targets Z3 5.4.1 and builds against every
Z3 release from 4.8.13. Functions that need a newer Z3 than the one z3r was
built with report the release they need.

## Installation

* `configure` finds Z3 through `PKG_CPPFLAGS` and `PKG_LIBS`,
  `Z3_INCLUDE_DIR` and `Z3_LIB_DIR`, `pkg-config` or common installation
  prefixes, and checks that the headers and the library are the same
  release. On Windows, `configure.win` accepts an official release archive
  through `Z3_DIR` and installs its DLLs and license next to z3r.
* With `Z3R_DOWNLOAD=true`, and on R-universe, an installation that finds
  no usable Z3 downloads the pinned official Z3 5.4.1 release, verifies its
  SHA-256 checksum and installs the library inside the package. Nothing is
  downloaded otherwise.
* `z3_version()` and `z3_build_info()` report the Z3 release, how it was
  found, the loaded library and the compiler.

## Expressions

* Sorts: Boolean, integer, real, bit-vector, floating point, rounding
  mode, string, sequence, regular expression, character, array, set,
  algebraic datatype, finite domain and uninterpreted sorts, and the
  complex, finite field and finite set sorts of Z3 5.
* Exact numerals from strings, R numbers and `gmp` numbers, with a
  documented policy for doubles (`options(z3r.doubles)`). `z3_value()`
  converts results to R without loss of precision.
* R operators and math functions on Z3 terms, with sort checking:
  arithmetic, comparisons (also lexicographic order on strings), Boolean
  logic, `sqrt()`, and `a[i]` on arrays.
* Batch construction of constants (`z3_ints()`, `z3_reals()`,
  `z3_bools()`), n-ary sums and products (`z3_sum()`, `z3_prod()`), and
  linear systems from dense or sparse coefficient matrices with exact
  coefficients (`z3_linear()`).
* Inspection of terms and declarations, substitution, simplification with
  parameters, quantifiers and lambdas with patterns and weights, and a
  dictionary keyed by terms (`z3_ast_map()`).
* Translation of objects between contexts (`z3_translate()`) and to and
  from the symcompR computer algebra package, refusing any conversion that
  would change the meaning of an expression.

## Solving

* Solvers with assertions, tracked assertions, assumptions, scopes
  (`z3_push()`, `z3_pop()`, `z3_with_scope()`), unsatisfiable cores,
  proofs, consequences, cubes, units and trails, congruence classes,
  solving for variables, initial values, typed parameters and statistics.
* One-call shortcuts: `z3_solve()` returns a model or `NULL`, and
  `z3_prove()` returns `TRUE` or `FALSE` with a counterexample.
* `z3_check_result()` returns the status, model, core, proof, timing,
  statistics, parameters, a fingerprint of the input and the build in one
  object, for batches and reports.
* Models: each `z3_model()` is an independent copy; values by name
  (`m$x`, `m[["x"]]`), batch evaluation of expressions (`z3_eval()` with a
  list) and conversion of all or selected values (`z3_model_values()`);
  enumeration of distinct models (`z3_all_models()`, which can stream
  them to a callback); hand-built models.
* Per-check timeouts (`z3_check(timeout = )`) that leave the solver's other
  parameters in place, interruption with Ctrl-C, and `z3_interrupt()` from
  inside callbacks.
* Optimization with hard and soft constraints, multiple objectives with
  priorities, unbounded and non-attained optima, bounds and the best model
  found when a check returns `"unknown"`, and model callbacks.
* User propagators, clause callbacks and proof checker callbacks written in
  R. Errors and interrupts in callbacks are raised in R after the check,
  and R code never runs on Z3's worker threads; a result found by a solver
  copy that could not run the callbacks is refused.

## Theories and engines

* Linear and nonlinear arithmetic, bit-vectors with overflow predicates,
  IEEE floating point, arrays (multi-index, map, default values, lambdas),
  sets, strings, sequences, regular expressions and characters.
* Algebraic datatypes: recursive, mutually recursive, enumerations, tuples,
  lists and polymorphic datatypes.
* Uninterpreted and recursive functions, special relations and
  pseudo-Boolean constraints.
* Tactics, probes, goals and simplifiers.
* The fixedpoint engine for constrained Horn clauses (Spacer) and Datalog,
  with lemma callbacks, external relations, model-based projection and
  quantifier elimination.
* Real closed fields, algebraic number arithmetic and polynomial
  subresultants.
* SMT-LIB2 parsing, export (with tracked constraints and the inputs of
  solvers with simplifiers, so the text has the solver's answer) and the
  SMT-LIB2 command interpreter; DIMACS export of clausal problems.

## Safety

* Every Z3 object is reference counted and keeps its context alive, so
  objects can be garbage collected in any order.
* Z3 errors become R errors. Objects restored from a saved session, and
  objects from different contexts, are refused with a clear message.
