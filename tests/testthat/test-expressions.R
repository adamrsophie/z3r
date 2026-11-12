test_that("constants have the requested sorts", {
  ctx <- z3_context()
  expect_identical(z3_sort_kind(z3_int(ctx, "x")), "int")
  expect_identical(z3_sort_kind(z3_real(ctx, "y")), "real")
  expect_identical(z3_sort_kind(z3_bool(ctx, "p")), "bool")
  expect_identical(z3_sort_kind(z3_bv(ctx, "b", 16)), "bitvector")
  expect_identical(z3_bv_width(z3_bv(ctx, "b", 16)), 16L)
  expect_identical(z3_sort_kind(z3_string(ctx, "s")), "string")
  expect_true(z3_sort(z3_int(ctx, "x")) == z3_int_sort(ctx))
  expect_true(z3_sort(z3_int(ctx, "x")) != z3_real_sort(ctx))
  expect_identical(format(z3_bv_sort(ctx, 8)), "(_ BitVec 8)")
})

test_that("same name and sort give the same constant", {
  ctx <- z3_context()
  expect_true(z3_identical(z3_int(ctx, "x"), z3_int(ctx, "x")))
  expect_false(z3_identical(z3_int(ctx, "x"), z3_int(ctx, "y")))
  f1 <- z3_fresh_const(ctx, "t", z3_int_sort(ctx))
  f2 <- z3_fresh_const(ctx, "t", z3_int_sort(ctx))
  expect_false(z3_identical(f1, f2))
})

test_that("plural constructors return named lists", {
  ctx <- z3_context()
  v <- z3_ints(ctx, c("a", "b", "c"))
  expect_named(v, c("a", "b", "c"))
  expect_identical(format(v$b), "b")
  expect_error(z3_ints(ctx, c("a", "a")), "distinct")
  expect_length(z3_bools(ctx, c("p", "q")), 2L)
  expect_identical(z3_sort_kind(z3_reals(ctx, "r")$r), "real")
})

test_that("integer values are exact at any size", {
  ctx <- z3_context()
  big <- "123456789012345678901234567890123456789"
  expect_identical(format(z3_int_val(ctx, big)), big)
  expect_identical(format(z3_simplify(z3_int_val(ctx, big) + 1)),
                   "123456789012345678901234567890123456790")
  expect_identical(format(z3_int_val(ctx, "-7")), "(- 7)")
  expect_identical(format(z3_int_val(ctx, 42L)), "42")
  expect_identical(format(z3_int_val(ctx, 2^53)), "9007199254740992")
  expect_error(z3_int_val(ctx, 2^53 + 2), "beyond 2\\^53")
  expect_error(z3_int_val(ctx, 1.5), "not a whole number")
  expect_error(z3_int_val(ctx, "1.5"), "is not an integer")
  expect_error(z3_int_val(ctx, "12a"), "is not an integer")
  expect_error(z3_int_val(ctx, ""), "is not an integer")
  expect_error(z3_int_val(ctx, NA), "NA")
})

test_that("rational values are exact", {
  ctx <- z3_context()
  expect_identical(format(z3_real_val(ctx, "1/3")), "(/ 1.0 3.0)")
  expect_identical(format(z3_real_val(ctx, "2/4")), "(/ 1.0 2.0)")
  expect_identical(format(z3_real_val(ctx, "-1/3")), "(- (/ 1.0 3.0))")
  expect_identical(format(z3_real_val(ctx, "0.125")), "(/ 1.0 8.0)")
  expect_identical(format(z3_real_val(ctx, "1e-3")), "(/ 1.0 1000.0)")
  # Exponents Z3 would read modulo 2^64 are refused.
  expect_error(z3_real_val(ctx, "1e18446744073709551616"), "exponent is too large")
  expect_error(z3_real_val(ctx, "1e-4294967296"), "exponent is too large")
  expect_identical(format(z3_real_val(ctx, "1e0000000000000000002")), "100.0")
  expect_identical(format(z3_real_val(ctx, "1.5E3")), "1500.0")
  expect_identical(format(z3_real_val(ctx, 0.1)), "(/ 1.0 10.0)")
  expect_identical(format(z3_real_val(ctx, 3L)), "3.0")
  # Inputs that Z3's own parser would silently misread are rejected.
  expect_error(z3_real_val(ctx, "1/-3"), "is not a decimal")
  expect_error(z3_real_val(ctx, "1.2.3"), "is not a decimal")
  expect_error(z3_real_val(ctx, ""), "is not a decimal")
  expect_error(z3_real_val(ctx, "1/0"), "denominator")
  expect_error(z3_real_val(ctx, Inf), "Inf")
  expect_error(z3_real_val(ctx, NaN), "NaN")
})

test_that("doubles follow the documented conversion policy", {
  ctx <- z3_context()
  x <- z3_real(ctx, "x")
  expect_identical(format(x + 0.1), "(+ x (/ 1.0 10.0))")
  expect_identical(format(x + 1/3), "(+ x (/ 3333333333333333.0 10000000000000000.0))")
  expect_identical(format(x + 2), "(+ x 2.0)")
  old <- options(z3r.doubles = "error")
  on.exit(options(old))
  expect_error(x + 0.1, "z3r.doubles")
  expect_identical(format(x + 2), "(+ x 2.0)")
  options(z3r.doubles = "bogus")
  expect_error(x + 0.5, "must be \"decimal\" or \"error\"")
})

test_that("gmp numbers convert exactly", {
  skip_if_not_installed("gmp")
  ctx <- z3_context()
  x <- z3_int(ctx, "x")
  r <- z3_real(ctx, "r")
  big <- gmp::as.bigz("100000000000000000000000000001")
  expect_identical(format(x + big), "(+ x 100000000000000000000000000001)")
  expect_identical(format(big + x), "(+ 100000000000000000000000000001 x)")
  expect_identical(format(r + gmp::as.bigq(1, 3)), "(+ r (/ 1.0 3.0))")
  expect_identical(format(z3_int_val(ctx, big)), "100000000000000000000000000001")
})

test_that("z3_value converts values without losing precision", {
  ctx <- z3_context()
  expect_identical(z3_value(z3_int_val(ctx, 42)), 42)
  expect_identical(z3_value(z3_int_val(ctx, -7)), -7)
  expect_identical(z3_value(z3_real_val(ctx, "1/4")), 0.25)
  expect_identical(z3_value(z3_real_val(ctx, "-3/8")), -0.375)
  expect_identical(z3_value(z3_bool_val(ctx, TRUE)), TRUE)
  expect_identical(z3_value(z3_string_val(ctx, "abc")), "abc")
  expect_equal(z3_value(z3_real_val(ctx, "1/3"), exact = FALSE), 1 / 3)
  expect_identical(z3_value(z3_int_val(ctx, 2^53)), 2^53)
  expect_error(z3_value(z3_int(ctx, "x")), "symbolic")
  expect_error(z3_value(1), "must be a z3_expr")
})

test_that("values that do not fit a double become gmp or strings", {
  ctx <- z3_context()
  big <- "123456789012345678901234567890"
  if (requireNamespace("gmp", quietly = TRUE)) {
    expect_identical(as.character(z3_value(z3_int_val(ctx, big))), big)
    expect_s3_class(z3_value(z3_int_val(ctx, big)), "bigz")
    q <- z3_value(z3_real_val(ctx, "1/3"))
    expect_s3_class(q, "bigq")
    expect_identical(as.character(q), "1/3")
  }
  # 2^53 + 1 is not a double; it must not be rounded to 2^53.
  v <- z3_value(z3_int_val(ctx, "9007199254740993"))
  expect_identical(as.character(v), "9007199254740993")
  expect_identical(z3_decimal(z3_real_val(ctx, "1/3"), 5), "0.33333?")
})

test_that("as.numeric, as.integer and as.logical are explicit conversions", {
  ctx <- z3_context()
  expect_identical(as.numeric(z3_real_val(ctx, "1/2")), 0.5)
  expect_equal(as.numeric(z3_real_val(ctx, "1/3")), 1 / 3)
  expect_identical(as.integer(z3_int_val(ctx, 12)), 12L)
  expect_error(as.integer(z3_int_val(ctx, "99999999999")), "integer range")
  expect_identical(as.logical(z3_bool_val(ctx, FALSE)), FALSE)
  expect_error(as.logical(z3_int_val(ctx, 1)), "not a Boolean")
  expect_error(as.logical(z3_bool(ctx, "p")), "symbolic")
})

test_that("strings are exact Unicode values", {
  ctx <- z3_context()
  s <- "héllo ✓ \\u{41} \U0001F600"
  expect_identical(z3_value(z3_string_val(ctx, s)), s)
  expect_identical(z3_value(z3_string_val(ctx, "")), "")
  expect_identical(z3_value(z3_simplify(z3_length(z3_string_val(ctx, s)))), 16)
  expect_identical(nchar(s), 16L)
})

test_that("printing shows sort and term", {
  ctx <- z3_context()
  x <- z3_int(ctx, "x")
  expect_output(print(x + 1), "<z3_expr Int> \\(\\+ x 1\\)")
  expect_identical(as.character(x * 2), "(* x 2)")
  expect_true(z3_is_expr(x))
  expect_false(z3_is_expr(1))
})

test_that("values of more sorts convert to R", {
  ctx <- z3_context()
  # A power-of-two denominator beyond 2^64 is still an exact double.
  expect_identical(z3_value(z3_real_val(ctx, "1/1180591620717411303424")), 2^-70)
  skip_if_not(z3_version_at_least("4.8.15"))
  expect_identical(z3_value(z3_char_val(ctx, 955)), "λ")
  s <- z3_string(ctx, "s")
  sol <- z3_solver(ctx)
  z3_add(sol, s == z3_str_from_code(z3_int_val(ctx, 0)))
  expect_identical(z3_check(sol), "sat")
  v <- z3_eval(z3_model(sol), s)
  expect_error(z3_value(v), "U\\+0000")
  expect_s3_class(z3_model_values(z3_model(sol))$s, "z3_expr")
})
