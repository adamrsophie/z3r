test_that("z3_trace() turns tags on and off", {
  # Release builds of Z3 ignore trace tags; debug builds would trace, so the
  # tags are turned off again at once.
  expect_invisible(z3_trace(c("arith", "spacer")))
  expect_identical(z3_trace(c("arith", "spacer"), enable = FALSE), c("arith", "spacer"))
  expect_error(z3_trace(1), "character vector")
  expect_error(z3_trace(NA_character_), "non-missing string")
  expect_error(z3_trace("arith", enable = NA), "TRUE or FALSE")
})

test_that("contexts with concurrent reference counting", {
  expect_error(z3_context(concurrent_dec_ref = NA), "TRUE or FALSE")
  if (!z3_version_at_least("4.10.0")) {
    expect_error(z3_context(concurrent_dec_ref = TRUE), "4.10.0")
    skip("concurrent_dec_ref needs Z3 4.10.0")
  }
  ctx <- z3_context(concurrent_dec_ref = TRUE, proof = FALSE)
  for (i in 1:20) local({
    y <- z3_int(ctx, paste0("y", i))
    check_status(ctx, y > i)
  })
  gc()
  x <- z3_int(ctx, "x")
  expect_identical(check_status(ctx, x > 1, x < 3), "sat")
})

test_that("relation sorts reach R through external relations", {
  ctx <- z3_context()
  N <- z3_finite_domain_sort(ctx, "N", 8)
  B <- z3_bool_sort(ctx)
  edge <- z3_function(ctx, "edge", list(N, N), B)
  path <- z3_function(ctx, "path", list(N, N), B)
  a <- z3_const(ctx, "a", N)
  b <- z3_const(ctx, "b", N)
  dl <- z3_fixedpoint(ctx)
  z3_set_params(dl, engine = "datalog")
  rel <- NULL
  z3_fixedpoint_external(dl, app = function(decl, args) {
    for (x in args) {
      if (is.null(rel) && z3_sort_kind(x) == "relation") rel <<- z3_sort(x)
    }
    NULL
  })
  z3_register_relation(dl, edge)
  z3_register_relation(dl, path)
  z3_set_representation(dl, edge, "external_relation")
  z3_add_fact(dl, edge, c(1, 2))
  z3_add_rule(dl, z3_forall(list(a, b), z3_implies(edge(a, b), path(a, b))))
  z3_query(dl, path)
  expect_false(is.null(rel))
  cols <- z3_relation_columns(rel)
  expect_length(cols, 2)
  for (s in cols) {
    expect_identical(z3_sort_kind(s), "finite_domain")
    expect_equal(z3_finite_domain_size(s), 8)
  }
  expect_error(z3_relation_columns(N), "expected a relation sort")
  expect_error(z3_relation_columns(a), "z3_sort")
})
