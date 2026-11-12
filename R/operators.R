#' Operators on Z3 expressions
#'
#' Arithmetic, comparison and logical operators build new symbolic
#' expressions instead of computing R values.
#'
#' | Operator | Int / Real | Bool | bit-vector |
#' |---|---|---|---|
#' | `+`, `-`, `*` | arithmetic | error | modular arithmetic |
#' | `/` | exact real division | error | error, see [z3_bvudiv()] |
#' | `^` | power | error | error |
#' | `%/%`, `%%` | Int only: SMT-LIB `div`, `mod` | error | error |
#' | `==`, `!=` | equation | equivalence | equation |
#' | `<`, `<=`, `>`, `>=` | comparison | error | error, see [z3_bvult()] |
#' | `&`, `\|`, `!` | error | and, or, not | bitwise and, or, not |
#'
#' Int and Real operands can be mixed; the Int operand is converted to Real
#' exactly. Every other combination of sorts is an error: Booleans are not
#' numbers, and bit-vectors are never mixed with unbounded integers.
#'
#' **Division.** `/` is real division, as in R, so `x / 2` is Real even when
#' `x` is an Int. `%/%` and `%%` are Z3's integer `div` and `mod`, which
#' follow the SMT-LIB (Euclidean) convention: the remainder `a %% b` is
#' always between 0 and `abs(b) - 1`. This agrees with R for positive
#' divisors but **differs for negative divisors**: in Z3 `7 %/% -2` is `-3`
#' and `7 %% -2` is `1`, while R gives `-4` and `-1`. Division by zero is
#' not an error; Z3 treats `x / 0` as an unspecified value.
#'
#' **Powers.** `x^k` for a literal whole number `0 <= k <= 64` is expanded
#' into a product and keeps the sort of `x`. Other powers use Z3's power
#' operator, which is Real-valued; constraints with symbolic exponents are
#' often beyond Z3's decision procedures and may return `"unknown"`.
#'
#' **R values.** An R scalar combined with a Z3 expression is converted to
#' the sort of the other operand; see [z3_int_val()] for the conversion
#' rules for doubles. Only length-one R values are accepted: z3r
#' expressions are scalars, and R's recycling rules have no meaning for a
#' single formula. Build several constraints with [lapply()] or [Map()].
#'
#' **R's own logic.** `&&` and `||` cannot be overloaded; use [z3_and()]
#' and [z3_or()]. A Z3 expression is never converted to `TRUE` or `FALSE`,
#' so `if (x > 0)` is an error rather than a silent mistake. Note that
#' `isTRUE()` returns `FALSE` for any Z3 expression.
#'
#' @param e1,e2 `z3_expr` objects or R scalars.
#' @return A `z3_expr`.
#' @examples
#' ctx <- z3_context()
#' x <- z3_int(ctx, "x")
#' y <- z3_real(ctx, "y")
#' p <- z3_bool(ctx, "p")
#' x + 1
#' x / 2           # Real division
#' x %/% 2         # integer division
#' x^2 + y^2 <= 1
#' p & (x > 0) | !p
#' @name z3_operators
NULL

#' @rdname z3_operators
#' @export
Ops.z3_expr <- function(e1, e2) {
  if (missing(e2)) return(cpp_unary(.Generic, e1))
  cpp_binary(.Generic, e1, e2)
}

# When both operands have Ops methods (for example a z3_expr and a
# gmp::bigz), use the z3_expr method.
#' @exportS3Method base::chooseOpsMethod
chooseOpsMethod.z3_expr <- function(x, y, mx, my, cl, reverse) TRUE

#' @export
Summary.z3_expr <- function(..., na.rm = FALSE) {
  switch(.Generic,
    sum = z3_sum(...),
    prod = z3_prod(...),
    all = z3_and(...),
    any = z3_or(...),
    stop(sprintf("%s() is not defined for Z3 expressions", .Generic), call. = FALSE)
  )
}

#' @export
Math.z3_expr <- function(x, ...) {
  if (.Generic == "abs") return(z3_abs(x))
  stop(sprintf("%s() has no exact counterpart in Z3", .Generic), call. = FALSE)
}

#' Logical connectives
#'
#' Symbolic versions of R's logical operators. Arguments are `z3_expr`
#' Booleans, R `TRUE`/`FALSE`, or lists of them.
#'
#' `z3_and()` and `z3_or()` need at least one argument, from which they
#' take the context; to combine a list that may be empty, include the
#' identity, as in `z3_and(z3_bool_val(ctx, TRUE), constraints)`.
#' `z3_ite(cond, a, b)` is "if `cond` then `a` else `b`" as an
#' expression; `a` and `b` must have compatible sorts. `z3_distinct()`
#' states that all its arguments are pairwise different.
#'
#' @param ... Boolean expressions, or for `z3_distinct()` expressions of a
#'   common sort. Lists are spliced.
#' @param p,q Boolean expressions.
#' @param cond A Boolean expression.
#' @param a,b Expressions of compatible sorts.
#' @return A `z3_expr`.
#' @examples
#' ctx <- z3_context()
#' p <- z3_bool(ctx, "p")
#' q <- z3_bool(ctx, "q")
#' z3_and(p, q)
#' z3_implies(p, q)
#' x <- z3_int(ctx, "x")
#' z3_ite(x > 0, x, -x)
#' v <- z3_ints(ctx, c("a", "b", "c"))
#' z3_distinct(v)
#' @export
z3_and <- function(...) bool_nary("and", flatten_args(...))

#' @rdname z3_and
#' @export
z3_or <- function(...) bool_nary("or", flatten_args(...))

#' @rdname z3_and
#' @export
z3_not <- function(p) cpp_op("not", list(p), integer())

#' @rdname z3_and
#' @export
z3_implies <- function(p, q) cpp_op("implies", list(p, q), integer())

#' @rdname z3_and
#' @export
z3_xor <- function(p, q) cpp_op("xor", list(p, q), integer())

#' @rdname z3_and
#' @export
z3_iff <- function(p, q) cpp_op("iff", list(p, q), integer())

#' @rdname z3_and
#' @export
z3_ite <- function(cond, a, b) cpp_op("ite", list(cond, a, b), integer())

#' @rdname z3_and
#' @export
z3_distinct <- function(...) cpp_op("distinct", flatten_args(...), integer())

bool_nary <- function(op, args) {
  if (length(args) == 0L) {
    stop(sprintf("z3_%s() needs at least one argument to determine the context", op),
         call. = FALSE)
  }
  cpp_op(op, args, integer())
}

#' Arithmetic functions
#'
#' * `z3_sum()` and `z3_prod()` add or multiply any number of arithmetic
#'   (or same-width bit-vector) expressions; lists are spliced.
#' * `z3_div()`, `z3_mod()` and `z3_rem()` are Z3's integer division,
#'   modulus and remainder. `div` and `mod` follow the SMT-LIB Euclidean
#'   convention (`0 <= mod(a, b) < abs(b)`); `rem` takes the sign of the
#'   divisor as in SMT-LIB. See [z3_operators] for how this differs from
#'   R.
#' * `z3_abs()` is the absolute value.
#' * `z3_to_real()` converts Int to Real; `z3_to_int()` is the floor of a
#'   Real; `z3_is_int()` states that a Real is a whole number.
#'
#' @param ... Arithmetic expressions or lists of them.
#' @param a,b,x Arithmetic expressions.
#' @return A `z3_expr`.
#' @examples
#' ctx <- z3_context()
#' v <- z3_ints(ctx, c("a", "b", "c"))
#' z3_sum(v)
#' z3_simplify(z3_mod(z3_int_val(ctx, -7), 2))
#' @export
z3_sum <- function(...) cpp_op("sum", flatten_args(...), integer())

#' @rdname z3_sum
#' @export
z3_prod <- function(...) cpp_op("prod", flatten_args(...), integer())

#' @rdname z3_sum
#' @export
z3_div <- function(a, b) cpp_op("div", list(a, b), integer())

#' @rdname z3_sum
#' @export
z3_mod <- function(a, b) cpp_op("mod", list(a, b), integer())

#' @rdname z3_sum
#' @export
z3_rem <- function(a, b) cpp_op("rem", list(a, b), integer())

#' @rdname z3_sum
#' @export
z3_abs <- function(x) cpp_op("abs", list(x), integer())

#' @rdname z3_sum
#' @export
z3_to_real <- function(x) cpp_op("to_real", list(x), integer())

#' @rdname z3_sum
#' @export
z3_to_int <- function(x) cpp_op("to_int", list(x), integer())

#' @rdname z3_sum
#' @export
z3_is_int <- function(x) cpp_op("is_int", list(x), integer())

#' Cardinality and pseudo-Boolean constraints
#'
#' `z3_atmost(..., k)` and `z3_atleast(..., k)` state that at most (at
#' least) `k` of the Boolean arguments are true. `z3_pble()`, `z3_pbge()`
#' and `z3_pbeq()` compare a weighted sum of Booleans, `sum(coeffs[i] *
#' lits[i])`, with `k`. Coefficients and `k` are integers. Z3 solves these
#' with dedicated procedures, which is much faster than encoding them with
#' [z3_ite()].
#'
#' @param ... Boolean expressions or lists of them.
#' @param lits A list of Boolean expressions.
#' @param coeffs Integer coefficients, one per literal.
#' @param k The bound, an integer.
#' @return A Boolean `z3_expr`.
#' @examples
#' ctx <- z3_context()
#' p <- z3_bools(ctx, letters[1:4])
#' s <- z3_solver(ctx)
#' z3_add(s, z3_atleast(p, k = 3), z3_pble(p, c(1, 2, 3, 4), 6))
#' z3_check(s)
#' unlist(z3_model_values(z3_model(s)))
#' @export
z3_atmost <- function(..., k) cpp_pb("atmost", flatten_args(...), integer(), pb_int(k, "k"))

#' @rdname z3_atmost
#' @export
z3_atleast <- function(..., k) cpp_pb("atleast", flatten_args(...), integer(), pb_int(k, "k"))

#' @rdname z3_atmost
#' @export
z3_pble <- function(lits, coeffs, k) cpp_pb("pble", as_list(lits), pb_int(coeffs, "coeffs"), pb_int(k, "k"))

#' @rdname z3_atmost
#' @export
z3_pbge <- function(lits, coeffs, k) cpp_pb("pbge", as_list(lits), pb_int(coeffs, "coeffs"), pb_int(k, "k"))

#' @rdname z3_atmost
#' @export
z3_pbeq <- function(lits, coeffs, k) cpp_pb("pbeq", as_list(lits), pb_int(coeffs, "coeffs"), pb_int(k, "k"))

pb_int <- function(x, what) {
  if (!is.numeric(x) || anyNA(x) || any(x != floor(x)) || any(abs(x) > .Machine$integer.max)) {
    stop(sprintf("%s must be whole numbers within R's integer range", what), call. = FALSE)
  }
  as.integer(x)
}

#' Divisibility
#'
#' `z3_divides(a, b)` states that the integer `a` divides `b`. `a` is
#' usually a numeral.
#'
#' @param a,b Int expressions or R whole numbers.
#' @return A Boolean `z3_expr`.
#' @examples
#' ctx <- z3_context()
#' x <- z3_int(ctx, "x")
#' s <- z3_solver(ctx)
#' z3_add(s, z3_divides(7, x), x > 50, x < 60)
#' z3_check(s)
#' z3_eval(z3_model(s), x)
#' @export
z3_divides <- function(a, b) cpp_misc_op("divides", list(a, b), integer())
