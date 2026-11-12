test_that("arithmetic operators build the expected terms", {
  ctx <- z3_context()
  x <- z3_int(ctx, "x")
  y <- z3_int(ctx, "y")
  expect_identical(format(x + y), "(+ x y)")
  expect_identical(format(x - y), "(- x y)")
  expect_identical(format(x * y), "(* x y)")
  expect_identical(format(-x), "(- x)")
  expect_identical(format(+x), "x")
  expect_identical(format(1 - x), "(- 1 x)")
  expect_identical(format(x %/% y), "(div x y)")
  expect_identical(format(x %% y), "(mod x y)")
  expect_identical(z3_sort_kind(x + y), "int")
})

test_that("division is real division, as in R", {
  ctx <- z3_context()
  x <- z3_int(ctx, "x")
  expect_identical(z3_sort_kind(x / 2), "real")
  expect_identical(solve_values(ctx, x / 2 == 3.5, vars = list(x = x)), list(x = 7))
  expect_identical(check_status(ctx, x / 2 == 1.25), "unsat")
})

test_that("integer div and mod follow SMT-LIB, not R", {
  ctx <- z3_context()
  val <- function(e) z3_value(z3_simplify(e))
  i <- function(v) z3_int_val(ctx, v)
  # Euclidean division: 0 <= a %% b < |b|
  expect_identical(val(i(7) %/% 2), 3)
  expect_identical(val(i(7) %% 2), 1)
  expect_identical(val(i(-7) %/% 2), -4)
  expect_identical(val(i(-7) %% 2), 1)
  expect_identical(val(i(7) %/% -2), -3)
  expect_identical(val(i(7) %% -2), 1)
  expect_identical(val(i(-7) %/% -2), 4)
  expect_identical(val(i(-7) %% -2), 1)
  # For positive divisors this agrees with R.
  for (a in -9:9) {
    expect_identical(val(i(a) %/% 4), as.numeric(a %/% 4))
    expect_identical(val(i(a) %% 4), as.numeric(a %% 4))
  }
  # rem takes the sign of the divisor
  expect_identical(val(z3_rem(i(-7), 2)), 1)
  expect_identical(val(z3_rem(i(7), -2)), -1)
  expect_identical(val(z3_div(i(9), 4)), 2)
  expect_identical(val(z3_mod(i(-9), 4)), 3)
  expect_error(z3_real(ctx, "r") %/% 2, "Int arguments")
})

test_that("powers keep the sort for small literal exponents", {
  ctx <- z3_context()
  x <- z3_int(ctx, "x")
  expect_identical(format(x^2), "(* x x)")
  expect_identical(z3_sort_kind(x^3), "int")
  expect_identical(format(x^1), "x")
  expect_identical(format(x^0), "1")
  expect_identical(z3_sort_kind(x^x), "real")
  expect_identical(z3_sort_kind(x^0.5), "real")
  r <- z3_real(ctx, "r")
  expect_identical(check_status(ctx, r^2 == 2, r > 0), "sat")
})

test_that("Int and Real mix by exact promotion", {
  ctx <- z3_context()
  x <- z3_int(ctx, "x")
  r <- z3_real(ctx, "r")
  expect_identical(format(x + r), "(+ (to_real x) r)")
  expect_identical(format(r < x), "(< r (to_real x))")
  expect_identical(format(x == r), "(= (to_real x) r)")
  expect_identical(check_status(ctx, x == r, r == 0.5), "unsat")
})

test_that("invalid sort combinations are rejected", {
  ctx <- z3_context()
  x <- z3_int(ctx, "x")
  p <- z3_bool(ctx, "p")
  b <- z3_bv(ctx, "b", 8)
  s <- z3_string(ctx, "s")
  expect_error(x + p, "needs Int or Real")
  expect_error(x == p, "same sort")
  expect_error(p < 1, "needs Int or Real")
  expect_error(x & p, "needs Bool")
  expect_error(!x, "needs Bool")
  expect_error(x + TRUE, "needs Int or Real")
  expect_error(b + x, "bit-vector")
  expect_error(b < 3, "signed and unsigned")
  expect_error(b / 2, "signed and unsigned")
  expect_error(s + 1, "needs Int or Real")
  expect_error(x == "a", "string literals")
  expect_error(s < "a", "needs Int or Real")
  expect_error(b == z3_bv(ctx, "c", 16), "equal width")
})

test_that("R operands must be scalars", {
  ctx <- z3_context()
  x <- z3_int(ctx, "x")
  expect_error(x + 1:3, "length 1")
  expect_error(x == c(1, 2), "length 1")
  expect_error(x + numeric(0), "length 1")
  expect_error(x + NA, "NA")
  expect_error(x + NA_integer_, "NA")
  expect_error(x + factor("a"), "factors")
  expect_error(x + list(1), "cannot be used")
})

test_that("symbolic expressions never become R logicals", {
  ctx <- z3_context()
  x <- z3_int(ctx, "x")
  expect_error(if (x > 0) 1, "not interpretable as logical")
  expect_error(while (x > 0) break, "not interpretable as logical")
  expect_error(as.logical(x > 0), "symbolic")
  expect_error((x > 0) && TRUE)
  expect_false(isTRUE(x > 0))
})

test_that("logical connectives", {
  ctx <- z3_context()
  p <- z3_bool(ctx, "p")
  q <- z3_bool(ctx, "q")
  expect_identical(format(p & q), "(and p q)")
  expect_identical(format(p | q), "(or p q)")
  expect_identical(format(!p), "(not p)")
  expect_identical(format(z3_implies(p, q)), "(=> p q)")
  expect_identical(format(z3_xor(p, q)), "(xor p q)")
  expect_identical(format(z3_iff(p, q)), "(= p q)")
  expect_identical(format(p == q), "(= p q)")
  expect_identical(format(z3_and(p, q, TRUE)), "(and p q true)")
  expect_identical(format(z3_or(list(p, q))), "(or p q)")
  expect_identical(format(z3_not(p)), "(not p)")
  expect_identical(format(xor(p, q)), "(and (or p q) (not (and p q)))")
  expect_identical(format(all(p, q)), "(and p q)")
  expect_identical(format(any(p, q)), "(or p q)")
  expect_error(z3_and(), "at least one")
  expect_error(z3_implies(p, 1), "needs Bool")
})

test_that("truth tables of the connectives are correct", {
  ctx <- z3_context()
  p <- z3_bool(ctx, "p")
  q <- z3_bool(ctx, "q")
  ops <- list(
    and = list(function(a, b) z3_and(a, b), function(a, b) a && b),
    or = list(function(a, b) z3_or(a, b), function(a, b) a || b),
    implies = list(z3_implies, function(a, b) !a || b),
    xor = list(z3_xor, xor),
    iff = list(z3_iff, function(a, b) a == b)
  )
  for (op in ops) {
    for (a in c(TRUE, FALSE)) for (b in c(TRUE, FALSE)) {
      e <- z3_simplify(op[[1]](z3_bool_val(ctx, a), z3_bool_val(ctx, b)))
      expect_identical(z3_value(e), op[[2]](a, b))
    }
  }
})

test_that("ite and distinct", {
  ctx <- z3_context()
  x <- z3_int(ctx, "x")
  r <- z3_real(ctx, "r")
  expect_identical(format(z3_ite(x > 0, x, -x)), "(ite (> x 0) x (- x))")
  expect_identical(z3_sort_kind(z3_ite(x > 0, x, r)), "real")
  expect_error(z3_ite(x, 1, 2), "needs Bool")
  expect_error(z3_ite(x > 0, x, z3_bool(ctx, "p")), "same sort")
  v <- z3_ints(ctx, c("a", "b", "c"))
  expect_identical(check_status(ctx, z3_distinct(v), lapply(v, function(e) e >= 0),
                                lapply(v, function(e) e <= 1)), "unsat")
  expect_error(z3_distinct(x), "at least two")
})

test_that("sum, prod and abs", {
  ctx <- z3_context()
  v <- z3_ints(ctx, c("a", "b", "c"))
  expect_identical(format(z3_sum(v)), "(+ a b c)")
  expect_identical(format(z3_prod(v$a, 2)), "(* a 2)")
  expect_identical(format(sum(v$a, v$b, 1)), "(+ a b 1)")
  expect_identical(format(abs(v$a)),
                   if (z3_version_at_least("4.13.2")) "(abs a)" else "(ite (< a 0) (- a) a)")
  expect_identical(z3_value(z3_simplify(abs(z3_int_val(ctx, -5)))), 5)
  expect_error(sqrt(v$a), "no exact counterpart")
  expect_error(max(v$a, v$b), "not defined")
})

test_that("conversion between Int and Real", {
  ctx <- z3_context()
  r <- z3_real(ctx, "r")
  x <- z3_int(ctx, "x")
  expect_identical(z3_value(z3_simplify(z3_to_int(z3_real_val(ctx, "-7/2")))), -4)
  expect_identical(z3_sort_kind(z3_to_real(x)), "real")
  expect_identical(check_status(ctx, z3_is_int(r), r > 1, r < 2), "unsat")
})

test_that("Ops dispatch prefers z3_expr over gmp methods", {
  skip_if_not_installed("gmp")
  ctx <- z3_context()
  x <- z3_int(ctx, "x")
  expect_s3_class(x * gmp::as.bigz(3), "z3_expr")
  expect_s3_class(gmp::as.bigz(3) * x, "z3_expr")
  expect_s3_class(gmp::as.bigz(3) < x, "z3_expr")
})

test_that("z3_decimal() truncates and marks inexact digits", {
  ctx <- z3_context()
  expect_identical(z3_decimal(z3_real_val(ctx, "2/3"), 2), "0.66?")
})
