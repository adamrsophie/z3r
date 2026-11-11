test_that("repeated creation and destruction is safe", {
  for (i in 1:200) {
    ctx <- z3_context()
    x <- z3_int(ctx, "x")
    s <- z3_solver(ctx)
    z3_add(s, x > i, x < i + 2)
    stopifnot(z3_check(s) == "sat")
    m <- z3_model(s)
    stopifnot(z3_value(z3_eval(m, x)) == i + 1)
    if (i %% 50 == 0) gc()
  }
  rm(ctx, x, s, m)
  gc()
  expect_true(TRUE)
})

test_that("garbage collection with live expressions, models and solvers", {
  ctx <- z3_context()
  xs <- z3_ints(ctx, paste0("x", 1:50))
  s <- z3_solver(ctx)
  for (i in seq_along(xs)) {
    z3_add(s, xs[[i]] == i)
    if (i %% 10 == 0) gc()
  }
  exprs <- lapply(xs, function(x) x * 2 + 1)
  rm(ctx)
  gc()
  expect_identical(z3_check(s), "sat")
  m <- z3_model(s)
  rm(s)
  gc()
  vals <- unname(vapply(exprs, function(e) z3_value(z3_eval(m, e)), 0))
  expect_identical(vals, 2 * (1:50) + 1)
})

test_that("objects outlive the R variables of their context in any order", {
  objs <- local({
    ctx <- z3_context()
    x <- z3_int(ctx, "x")
    opt <- z3_optimize(ctx)
    z3_add(opt, x >= 0, x <= 9)
    h <- z3_maximize(opt, x)
    g <- z3_goal(ctx)
    z3_add(g, x > 1)
    t <- z3_tactic(ctx, "simplify")
    f <- z3_function(ctx, "f", z3_int_sort(ctx), z3_int_sort(ctx))
    list(x = x, opt = opt, h = h, g = g, t = t, f = f)
  })
  gc()
  expect_identical(z3_check(objs$opt), "sat")
  expect_identical(z3_value(z3_optimum(objs$h)$value), 9)
  expect_length(z3_apply_tactic(objs$t, objs$g), 1L)
  expect_identical(z3_sort_kind(objs$f(objs$x)), "int")
  for (nm in sample(names(objs))) {
    objs[[nm]] <- NULL
    gc()
  }
  expect_length(objs, 0L)
})

test_that("errors inside native calls do not corrupt state", {
  ctx <- z3_context()
  x <- z3_int(ctx, "x")
  for (i in 1:100) {
    expect_error(x + TRUE)
    expect_error(z3_tactic(ctx, "nonexistent"))
    expect_error(z3_from_smtlib(ctx, "(assert (> undefined 1))"))
  }
  s <- z3_solver(ctx)
  z3_add(s, x == 1)
  expect_identical(z3_check(s), "sat")
})

test_that("many contexts at once", {
  ctxs <- lapply(1:20, function(i) z3_context())
  vars <- Map(function(ctx, i) z3_int(ctx, "x") == i, ctxs, seq_along(ctxs))
  sols <- lapply(seq_along(ctxs), function(i) {
    s <- z3_solver(ctxs[[i]])
    z3_add(s, vars[[i]])
    s
  })
  rm(ctxs, vars)
  gc()
  res <- vapply(sols, z3_check, "")
  expect_true(all(res == "sat"))
})
