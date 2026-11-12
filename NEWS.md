# z3r 0.1.0

First release. z3r targets Z3 5.4.1 and supports every Z3 release from
4.8.13; features that need a newer Z3 than the one z3r was built with say
which version they need.

## Native foundation

* Contexts created with `Z3_mk_context_rc`; every Z3 object is reference
  counted and keeps its context alive, so objects can be garbage collected
  in any order. Context configuration (`z3_context(proof = TRUE, ...)`) is
  validated.
* Z3 errors become R errors without leaking references. Objects restored
  from saved sessions fail with a clear message instead of crashing. Objects
  from different contexts are rejected; `z3_translate()` copies between
  contexts.
* `configure` locates Z3 through `PKG_CPPFLAGS`/`PKG_LIBS`,
  `Z3_INCLUDE_DIR`/`Z3_LIB_DIR`, `pkg-config` or common prefixes, links it
  by full path, and refuses mismatched headers and libraries.
  `configure.win` supports official Windows release archives through
  `Z3_DIR` and installs their DLLs and license beside z3r. z3r refuses to
  load with a Z3 library other than the release it was compiled against.
* Ctrl-C interrupts a running check.

## Expressions

* Int, Real, Bool, bit-vector, floating point, rounding mode, String,
  sequence, regular expression, character, array, set, datatype, finite
  domain and uninterpreted sorts, plus the complex, finite field and finite
  set sorts of Z3 5, with complex arithmetic and R complex literals.
* Exact numerals from strings, R numbers and `gmp` numbers, with a
  documented conversion policy for doubles. `z3_value()` converts results to
  R without loss of precision.
* R operators with z3r's own sort checking.
* Inspection (`z3_kind()`, `z3_decl()`, `z3_args()`, `z3_decl_info()`,
  `z3_quantifier_info()`), substitution, simplification with parameters,
  identifiers and print modes.

## Solving

* Solvers with assertions, tracked assertions, assumptions,
  push/pop/reset, `z3_with_scope()`, model enumeration
  (`z3_all_models()`), models with staleness protection, hand-built models,
  unsatisfiable cores, proofs, consequences, cubes, units and trails,
  congruence classes, solving for variables, initial values, typed
  parameters, statistics and DIMACS export.
* Optimization with hard and soft constraints, objectives (including the
  cost of soft constraint groups) and priorities, unbounded and
  non-attained optima, and model callbacks.
* User propagators, clause callbacks and the proof checker callback. R
  errors and interrupts are raised safely after the check, callbacks
  cannot reenter their own solver, and callback closures do not keep
  their solver or context alive.

## Theories and engines

* Arithmetic, bit-vectors with overflow predicates, IEEE floating point,
  arrays (multi-index, map, default, lambdas), sets, strings, sequences,
  regular expressions and characters.
* Algebraic datatypes: recursive, mutually recursive, enumerations, tuples,
  lists and polymorphic datatypes.
* Uninterpreted and recursive functions, quantifiers with patterns and
  weights, special relations, pseudo-Boolean constraints.
* Tactics, probes, goals and simplifiers.
* The fixedpoint engine for constrained Horn clauses (Spacer) and Datalog,
  Spacer lemma callbacks (`z3_on_lemma()`), external Datalog relations
  (`z3_fixedpoint_external()`), and Spacer's model-based projection
  (`z3_model_project()`), model extrapolation and `z3_qe_lite()`.
* Real closed fields, algebraic number arithmetic and polynomial
  subresultants.
* SMT-LIB2 export, parsing and the SMT-LIB2 command interpreter.
* Translation to and from the symcompR computer algebra package.
