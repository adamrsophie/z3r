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
  by full path (on Linux with a run-time path that `LD_LIBRARY_PATH`
  cannot override), and refuses mismatched headers and libraries.
  `configure.win` supports official Windows release archives through
  `Z3_DIR` and installs their DLLs and license beside z3r. z3r refuses to
  load with a Z3 library other than the release it was compiled against.
* Ctrl-C interrupts a running check.
* `z3_trace()` switches Z3's debug traces; `z3_context()` can enable
  concurrent reference count decrements.

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
* Strings with SMT-LIB escapes or from raw bytes, and their escaped and
  byte forms (`z3_string_escaped()`, `z3_string_bytes()`); rationals from
  a numerator and denominator; integer constant names; `z3_sort_name()`.
* Quantifiers and lambdas over de Bruijn indexed variables
  (`z3_quantifier_db()`) and printable patterns (`z3_pattern()`).
* `z3_ast_map()`, a dictionary keyed by Z3 terms, and
  `z3_format_terms()`.

## Solving

* Solvers with assertions, tracked assertions, assumptions,
  push/pop/reset, `z3_with_scope()`, model enumeration
  (`z3_all_models()`), models with staleness protection, hand-built models,
  unsatisfiable cores, proofs, consequences, cubes, units and trails,
  congruence classes, solving for variables, initial values, typed
  parameters, statistics (also as Z3's text, `z3_statistics_text()`) and
  DIMACS export.
* Timeouts for a single check (`z3_check(timeout = )`), which keep the
  other parameters of a solver or optimizer, and `z3_interrupt()`, which
  stops a solver's or optimizer's check from inside its own callbacks.
* Optimization with hard and soft constraints, objectives (including the
  cost of soft constraint groups) and priorities, unbounded and
  non-attained optima, model callbacks, and, when a check returns
  `"unknown"`, the bounds and the value of the best model found so far.
* User propagators, clause callbacks and the proof checker callback. R
  errors and interrupts are raised safely after the check, callbacks
  cannot reenter their own solver, and callback closures do not keep
  their solver or context alive.

## Theories and engines

* Arithmetic, bit-vectors with overflow predicates, IEEE floating point,
  arrays (multi-index, map, default, lambdas), sets, strings, sequences,
  regular expressions and characters.
* Algebraic datatypes: recursive, mutually recursive, enumerations, tuples
  (`z3_tuple_fields()`), lists and polymorphic datatypes.
* Floating point formats by name or width (`z3_fp_sort(ctx, "half")`),
  SMT-LIB rounding mode names, and numeric parts of numerals.
* Function interpretations with defaults set after creation
  (`z3_model_set_default()`); Datalog relation sorts
  (`z3_relation_columns()`).
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
