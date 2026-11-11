# z3r 0.1.0

First version.

## Native foundation

* Contexts created with `Z3_mk_context_rc`; every Z3 object is reference
  counted and keeps its context alive, so objects can be garbage collected in
  any order.
* Z3 errors become R errors without leaking references. Objects restored from
  saved sessions fail with a clear message instead of crashing.
* Objects from different contexts are rejected.
* `configure` locates Z3 through `PKG_CPPFLAGS`/`PKG_LIBS`,
  `Z3_INCLUDE_DIR`/`Z3_LIB_DIR`, `pkg-config` or common prefixes, links it by
  full path, and refuses mismatched headers and libraries. `configure.win`
  supports official Windows release archives through `Z3_DIR`.

## Expressions

* Int, Real, Bool, bit-vector, String, array and uninterpreted sorts.
* Exact numerals from strings, R numbers and `gmp` numbers, with a documented
  conversion policy for doubles (`options(z3r.doubles = "error")` makes it
  strict).
* R operators with z3r's own sort checking; `/` is real division,
  `%/%` and `%%` are SMT-LIB integer division and modulus.
* `z3_value()` converts values to R without loss of precision.

## Solving

* Solvers with assertions, tracked assertions, assumptions, push/pop/reset,
  models with staleness protection, unsatisfiable cores, reason for unknown,
  typed parameters, statistics.
* Ctrl-C interrupts a running solver check and raises an R interrupt.
* Optimization with hard and soft constraints, minimize and maximize, multiple
  objectives and priorities, unbounded and non-attained optima.

## Theories and tools

* Bit-vector arithmetic, signed and unsigned comparison and division, bitwise
  operations, shifts, rotations, concatenation, extraction, extension,
  conversions and overflow predicates.
* Arrays, strings, uninterpreted functions, quantifiers with patterns.
* Tactics, goals and solvers built from tactics.
* SMT-LIB2 export, parsing, and the SMT-LIB2 command interpreter.
