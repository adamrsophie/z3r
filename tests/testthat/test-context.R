test_that("contexts are created and report the Z3 version", {
  ctx <- z3_context()
  expect_s3_class(ctx, "z3_context")
  v <- z3_version()
  expect_match(v$runtime, "^[0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+$")
  expect_output(print(ctx), "z3_context")
})

test_that("objects keep their context alive after the context is removed", {
  make <- function() {
    ctx <- z3_context()
    x <- z3_int(ctx, "x")
    s <- z3_solver(ctx)
    z3_add(s, x > 5, x < 7)
    list(x = x, s = s)
  }
  obj <- make()
  gc()
  gc()
  expect_identical(z3_check(obj$s), "sat")
  expect_identical(z3_value(z3_eval(z3_model(obj$s), obj$x)), 6)
})

test_that("context identity is reported", {
  a <- z3_context()
  b <- z3_context()
  x <- z3_int(a, "x")
  expect_true(z3_same_context(x, a))
  expect_false(z3_same_context(x, b))
  expect_true(z3_same_context(z3_context_of(x), a))
  expect_true(z3_same_context(z3_solver(a), x))
})

test_that("objects from different contexts cannot be combined", {
  a <- z3_context()
  b <- z3_context()
  x <- z3_int(a, "x")
  y <- z3_int(b, "y")
  expect_error(x + y, "different contexts")
  expect_error(z3_add(z3_solver(b), x > 0), "different contexts")
  expect_error(z3_eval(local({
    s <- z3_solver(b)
    z3_add(s, y > 0)
    z3_check(s)
    z3_model(s)
  }), x), "different contexts")
  expect_error(z3_const(a, "z", z3_int_sort(b)), "different contexts")
  expect_error(z3_array_sort(z3_int_sort(a), z3_int_sort(b)), "different contexts")
})

test_that("saved and restored objects fail cleanly", {
  ctx <- z3_context()
  x <- z3_int(ctx, "x")
  f <- tempfile(fileext = ".rds")
  on.exit(unlink(f))
  saveRDS(list(ctx = ctx, x = x), f)
  r <- readRDS(f)
  expect_output(print(r$x), "invalid")
  expect_output(print(r$ctx), "invalid")
  expect_error(r$x + 1, "no longer valid")
  expect_error(z3_int(r$ctx, "y"), "no longer valid")
})

test_that("wrong object kinds are rejected", {
  ctx <- z3_context()
  s <- z3_solver(ctx)
  expect_error(z3_int(s, "x"), "expected a Z3 context")
  expect_error(z3_int(ctx, NA_character_), "single non-missing string")
  expect_error(z3_int(ctx, c("a", "b")), "single non-missing string")
  expect_error(z3_context_of(1), "expected a z3r object")
  expect_error(z3_eval(s, 1), "expected a Z3 model")
})

test_that("every argument is checked for its context", {
  a <- z3_context()
  b <- z3_context()
  rb <- z3_const(b, "r", z3_rounding_sort(b))
  expect_error(z3_fp_val(a, 0.1, z3_fp32_sort(a), rounding = rb), "different contexts")
  # A literal takes its sort from another argument; that argument is
  # checked before the literal is built.
  fb <- z3_const(b, "f", z3_fp32_sort(b))
  xa <- z3_int(a, "x")
  expect_error(z3_fp_add(z3_rounding(a, "RNE"), 1, fb), "different contexts")
  expect_error(z3_sum(1, xa, z3_int(b, "y")), "different contexts")
  m <- z3_model(a)
  expect_error(z3_model_has(m, z3_int(b, "y")), "different contexts")
  if (z3_version_at_least("4.13.0")) {
    expect_error(z3_propagator_function(a, "f", list(), z3_int_sort(b)), "different contexts")
  }
})
