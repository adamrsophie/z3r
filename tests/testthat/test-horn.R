test_that("constrained Horn clauses: reachability and invariants", {
  ctx <- z3_context()
  I <- z3_int_sort(ctx)
  B <- z3_bool_sort(ctx)
  inv <- z3_function(ctx, "inv", I, B)
  x <- z3_int(ctx, "x")
  fp <- z3_fixedpoint(ctx)
  z3_register_relation(fp, inv)
  z3_add_rule(fp, inv(0), name = "init")
  z3_add_rule(fp, z3_forall(x, z3_implies(inv(x), inv(x + 2))), name = "step")
  expect_output(print(fp), "2 rules, not queried")
  expect_identical(z3_query(fp, z3_exists(x, inv(x) & x == 7)), "unsat")
  invariant <- z3_answer(fp)
  expect_true(z3_is_quantifier(invariant))
  expect_identical(z3_query(fp, z3_exists(x, inv(x) & x == 8)), "sat")
  expect_s3_class(z3_answer(fp), "z3_expr")
  expect_length(z3_fixedpoint_rules(fp), 2L)
  z3_add_rule(fp, inv(1))
  expect_error(z3_answer(fp), "changed")
  expect_identical(z3_query(fp, z3_exists(x, inv(x) & x == 7)), "sat")
  expect_true("engine" %in% z3_param_help(fp)$name)
})

test_that("relation queries and Datalog", {
  ctx <- z3_context()
  N <- z3_finite_domain_sort(ctx, "N", 8)
  B <- z3_bool_sort(ctx)
  edge <- z3_function(ctx, "edge", list(N, N), B)
  path <- z3_function(ctx, "path", list(N, N), B)
  a <- z3_const(ctx, "a", N)
  b <- z3_const(ctx, "b", N)
  c <- z3_const(ctx, "c", N)
  fp <- z3_fixedpoint(ctx)
  z3_set_params(fp, engine = "datalog")
  z3_register_relation(fp, edge)
  z3_register_relation(fp, path)
  z3_add_fact(fp, edge, c(1, 2))
  z3_add_fact(fp, edge, c(2, 3))
  z3_add_rule(fp, z3_forall(list(a, b), z3_implies(edge(a, b), path(a, b))))
  z3_add_rule(fp, z3_forall(list(a, b, c), z3_implies(edge(a, b) & path(b, c), path(a, c))))
  expect_identical(z3_query(fp, path), "sat")
  ans <- format(z3_answer(fp))
  # The answer lists the derived tuples: 1 -> 2, 2 -> 3 and 1 -> 3.
  expect_match(ans, "\\(= \\(:var 1\\) 3\\)")
})

test_that("queries can start at a given level", {
  ctx <- z3_context()
  I <- z3_int_sort(ctx)
  inv <- z3_function(ctx, "inv", I, z3_bool_sort(ctx))
  x <- z3_int(ctx, "x")
  fp <- z3_fixedpoint(ctx)
  z3_register_relation(fp, inv)
  z3_add_rule(fp, inv(0))
  z3_add_rule(fp, z3_forall(x, z3_implies(inv(x), inv(x + 2))))
  expect_identical(z3_query(fp, z3_exists(x, inv(x) & x == 7), level = 0), "unsat")
  expect_identical(z3_query(fp, z3_exists(x, inv(x) & x == 4), level = 0), "sat")
  expect_error(z3_query(fp, inv, level = 0), "query formula")
  expect_error(z3_query(fp, inv(4), level = -1), "non-negative")
})

test_that("model-based projection, extrapolation and qe_lite", {
  ctx <- z3_context()
  x <- z3_int(ctx, "x")
  y <- z3_int(ctx, "y")
  f <- x > 2 & y == x + 1
  sol <- z3_solver(ctx)
  z3_add(sol, f)
  expect_identical(z3_check(sol), "sat")
  m <- z3_model(sol)
  holds <- function(e) isTRUE(z3_value(z3_eval(m, e, completion = TRUE)))
  valid <- function(e) {
    s <- z3_solver(ctx)
    z3_add(s, !e)
    z3_check(s) == "unsat"
  }
  ex <- z3_exists(x, f)

  p <- z3_model_project(m, x, f)
  expect_s3_class(p, "z3_expr")
  expect_false(grepl("\\bx\\b", format(p)))
  expect_true(holds(p))
  expect_true(valid(z3_implies(p, ex)))

  sk <- z3_model_project(m, list(x), f, method = "skolem")
  expect_named(sk, c("formula", "vars", "terms"))
  expect_true(holds(sk$formula))

  if (z3_version_at_least("4.13.4")) {
    w <- z3_model_project(m, x, f, method = "witness")
    expect_true(holds(w$formula))
    expect_length(w$vars, length(w$terms))
    if (length(w$vars)) {
      expect_true(z3_identical(w$vars[[1]], x))
      expect_true(holds(z3_substitute(f, w$vars, w$terms)))
    }
  }

  g <- f | y < 0
  e <- z3_model_extrapolate(m, g)
  expect_true(holds(e))
  expect_true(valid(z3_implies(e, g)))

  q <- z3_qe_lite(x, y == x + 1 & x > 2)
  expect_named(q, c("formula", "remaining"))
  expect_length(q$remaining, 0L)
  expect_false(grepl("\\bx\\b", format(q$formula)))
  expect_true(valid(q$formula == z3_exists(x, y == x + 1 & x > 2)))

  expect_error(z3_model_project(m, x + 1, f), "constants")
  expect_error(z3_model_project(m, x, f & y < 0), "true in the model")
  expect_error(z3_model_extrapolate(m, y < 0), "true in the model")
  expect_error(z3_model_project(z3_model(ctx), x, f), "true in the model")
  expect_error(z3_qe_lite(x, x + 1), "Boolean")
})
