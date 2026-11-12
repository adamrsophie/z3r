test_that("named formats and width aliases give the standard sorts", {
  ctx <- z3_context()
  widths <- list(half = c(5L, 11L), single = c(8L, 24L), double = c(11L, 53L), quadruple = c(15L, 113L))
  bits <- c(half = 16, single = 32, double = 64, quadruple = 128)
  for (name in names(widths)) {
    s <- z3_fp_sort(ctx, name)
    expect_identical(unname(z3_fp_sort_info(s)), widths[[name]])
    expect_true(s == z3_fp_sort(ctx, widths[[name]][1], widths[[name]][2]))
    expect_true(s == z3_fp_sort(ctx, bits[[name]]))
  }
  expect_true(z3_fp16_sort(ctx) == z3_fp_sort(ctx, "half"))
  expect_true(z3_fp32_sort(ctx) == z3_fp_sort(ctx, "single"))
  expect_true(z3_fp64_sort(ctx) == z3_fp_sort(ctx, "double"))
  expect_true(z3_fp128_sort(ctx) == z3_fp_sort(ctx, "quadruple"))
  expect_true(z3_fp_sort(ctx, 32L) == z3_fp32_sort(ctx))
  expect_error(z3_fp_sort(ctx, "octuple"), "must name a format")
  expect_error(z3_fp_sort(ctx, 24), "must name a format")
  expect_error(z3_fp_sort(ctx, c(16, 32)), "must name a format")
})

test_that("rounding modes accept their SMT-LIB names", {
  ctx <- z3_context()
  long <- c(RNE = "roundNearestTiesToEven", RNA = "roundNearestTiesToAway",
            RTP = "roundTowardPositive", RTN = "roundTowardNegative", RTZ = "roundTowardZero")
  for (short in names(long)) {
    m <- z3_rounding(ctx, long[[short]])
    expect_identical(z3_value(m), long[[short]])
    expect_true(z3_value(z3_simplify(m == z3_rounding(ctx, short))))
    # The name z3_value() reports is accepted back.
    expect_true(z3_value(z3_simplify(z3_rounding(ctx, z3_value(m)) == m)))
  }
  F <- z3_fp32_sort(ctx)
  expect_identical(z3_value(z3_fp_val(ctx, 0.1, F, "roundTowardZero")),
                   z3_value(z3_fp_val(ctx, 0.1, F, "RTZ")))
  expect_identical(z3_value(z3_rounding(ctx)), "roundNearestTiesToEven")
  expect_error(z3_rounding(ctx, "UP"))
  expect_error(z3_rounding(ctx, "round"))
})

test_that("exact literals are the numerals that rounding gives", {
  ctx <- z3_context()
  H <- z3_fp16_sort(ctx)
  F <- z3_fp32_sort(ctx)
  D <- z3_fp64_sort(ctx)
  Q <- z3_fp128_sort(ctx)
  tiny <- z3_fp_sort(ctx, 3, 4)
  wide32 <- z3_fp_sort(ctx, 8, 33)  # 32 fraction bits, the widest narrow format
  rne <- z3_rounding(ctx)
  agrees <- function(v, s) {
    lit <- z3_fp_val(ctx, v, s)
    rounded <- z3_simplify(z3_fp_to_fp(rne, z3_fp_val(ctx, v, D), s))
    isTRUE(z3_value(z3_simplify(lit == rounded))) && identical(z3_value(lit), v)
  }
  cases <- list(
    list(H, c(3, -7, 2047, -2048, 65504, 1.5, -0.75, 2^-14, 2^-20, 2^-24, -3 * 2^-24, 0.333251953125)),
    list(F, c(5, 16777216, 0.5, -2.75, 1 + 2^-23, 2^-149, -2^-126, (2 - 2^-23) * 2^127)),
    list(D, c(2^31 - 1, -2^31, 0.1, 1e300, 5e-324)),
    list(Q, c(2^31 - 1, -2^31, 1.5, 0.1, 1e300)),
    list(tiny, c(15, -15, 1.875, 0.25, 0.125, 2^-5, -3 * 2^-5)),
    list(wide32, c(1 + 2^-32, 2 - 2^-32, -(2^32 - 1), 2^-126 * (1 + 2^-32)))
  )
  for (case in cases) for (v in case[[2]]) expect_true(agrees(v, case[[1]]), info = format(v, digits = 17))

  # The same numerals as from their fields.
  same <- function(a, b) z3_value(z3_simplify(a == b))
  expect_true(same(z3_fp_val(ctx, 1.5, H), z3_fp_from_parts(H, FALSE, 0, 512)))
  expect_true(same(z3_fp_val(ctx, -0.75, H), z3_fp_from_parts(H, TRUE, -1, 512)))
  expect_true(same(z3_fp_val(ctx, 2^-20, H), z3_fp_from_parts(H, FALSE, -15, 16)))
  expect_true(same(z3_fp_val(ctx, 0.125, tiny), z3_fp_from_parts(tiny, FALSE, -3, 4)))

  # Values a format cannot hold are still refused.
  refused <- list(
    list(H, c(2049, 65536, 1e10, 0.1, 2^-25)),
    list(F, c(16777217, 1 + 2^-24, 0.1, 2^-150, 1e39)),
    list(tiny, c(16, 17, 0.3, 2^-6))
  )
  for (case in refused) for (v in case[[2]]) {
    expect_error(z3_fp_val(ctx, v, case[[1]]), "not exactly representable", info = format(v, digits = 17))
  }

  # R integers combined with expressions take the same path.
  h <- z3_fp(ctx, "h", H)
  expect_identical(z3_value(z3_simplify(z3_fp_val(ctx, 1.5, H) + 2L)), 3.5)
  expect_error(h + 2049L, "not exactly representable")
})

test_that("signed zeros and infinities keep their sign in every format", {
  ctx <- z3_context()
  sorts <- list(z3_fp16_sort(ctx), z3_fp32_sort(ctx), z3_fp64_sort(ctx), z3_fp128_sort(ctx),
                z3_fp_sort(ctx, 3, 4))
  for (s in sorts) {
    expect_identical(1 / z3_value(z3_fp_val(ctx, 0, s)), Inf)
    expect_identical(1 / z3_value(z3_fp_val(ctx, -0, s)), -Inf)
    expect_true(z3_value(z3_simplify(z3_fp_val(ctx, -0, s) == z3_fp_zero(s, negative = TRUE))))
    expect_identical(z3_value(z3_fp_inf(s, negative = TRUE)), -Inf)
    expect_identical(1 / z3_value(z3_fp_zero(s, negative = TRUE)), -Inf)
  }
})

test_that("numeral parts give sign, exponent and fraction as numbers", {
  ctx <- z3_context()
  H <- z3_fp16_sort(ctx)
  D <- z3_fp64_sort(ctx)
  Q <- z3_fp128_sort(ctx)
  parts <- function(x, ...) z3_fp_numeral_parts(x, ..., numeric = TRUE)
  # Without numeric = TRUE the list keeps its earlier fields.
  expect_identical(names(z3_fp_numeral_parts(z3_fp_val(ctx, 1.5, D))),
                   c("class", "negative", "significand", "exponent", "sign_bv", "exponent_bv",
                     "significand_bv"))
  expect_identical(names(z3_fp_numeral_parts(z3_fp_nan(D))), c("class", "negative"))

  p <- parts(z3_fp_val(ctx, -1.5, D))
  expect_identical(p$class, "normal")
  expect_true(p$negative)
  expect_identical(p$sign, 1L)
  expect_identical(p$exponent_value, 0)
  expect_identical(p$exponent, "0")
  expect_identical(p$fraction, 2^51)
  expect_identical(parts(z3_fp_val(ctx, 1.5, D), biased = TRUE)$exponent_value, 1023)

  sub <- parts(z3_fp_val(ctx, 2^-20, H))
  expect_identical(sub$class, "subnormal")
  expect_identical(sub$sign, 0L)
  expect_identical(sub$exponent_value, -14)
  expect_identical(sub$fraction, 16)

  zero <- parts(z3_fp_zero(D, negative = TRUE))
  expect_identical(zero$class, "zero")
  expect_identical(zero$sign, 1L)
  expect_identical(zero$fraction, 0)

  inf <- parts(z3_fp_inf(H))
  expect_identical(inf$class, "infinite")
  expect_identical(inf$sign, 0L)
  expect_identical(inf$exponent_value, 16)
  expect_identical(inf$fraction, 0)
  expect_null(inf$exponent)

  nan <- parts(z3_fp_nan(D))
  expect_identical(nan$class, "nan")
  expect_null(nan$sign)
  expect_null(nan$fraction)

  # Fractions beyond 2^53 are decimal strings; wider than 64 bits as well.
  expect_identical(parts(z3_fp_val(ctx, 1.5, z3_fp_sort(ctx, 8, 60)))$fraction, "288230376151711744")
  q <- parts(z3_fp_val(ctx, 1.5, Q))
  expect_identical(q$fraction, "2596148429267413814265248164610048")
  expect_identical(q$exponent_value, 0)
  expect_identical(q$significand, "1.5")
  expect_identical(parts(z3_fp_val(ctx, 1, Q))$fraction, 0)

  expect_error(parts(z3_fp(ctx, "x", D)), "not a floating point numeral")
  expect_error(parts(z3_int_val(ctx, 1)), "not a floating point numeral")
})
