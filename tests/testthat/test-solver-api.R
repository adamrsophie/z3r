# Factoring a product of two large primes: hard for Z3's nonlinear solver.
hard_constraints <- function(ctx) {
  x <- z3_int(ctx, "x")
  y <- z3_int(ctx, "y")
  list(x * y == z3_int_val(ctx, "1000000016000000063"), x > 1, y > 1)
}

# Pigeonhole: n + 1 pigeons in n holes, as clauses (unsatisfiable).
pigeonhole <- function(ctx, n) {
  h <- lapply(seq_len(n + 1), function(i) z3_bools(ctx, paste0("p", i, "h", seq_len(n))))
  at_most_one <- unlist(lapply(seq_len(n), function(j) {
    unlist(lapply(seq_len(n), function(i) {
      lapply(seq_len(n + 1)[-seq_len(i)], function(k) !h[[i]][[j]] | !h[[k]][[j]])
    }), recursive = FALSE)
  }), recursive = FALSE)
  c(lapply(h, z3_or), at_most_one)
}

test_that("per-call timeouts are validated in R", {
  ctx <- z3_context()
  s <- z3_solver(ctx)
  z3_add(s, z3_int(ctx, "x") > 0)
  for (bad in list(-1, 0, 1.5, "a", NA, c(1, 2), 2^31)) {
    expect_error(z3_check(s, timeout = bad), "timeout must be a single positive whole number")
  }
  opt <- z3_optimize(ctx)
  expect_error(z3_check(opt, timeout = -5), "timeout must be a single positive whole number")
  # A rejected timeout changes nothing.
  expect_false(any(grepl("parameters set", capture.output(print(s)), fixed = TRUE)))
})

test_that("a per-call timeout applies to one check and is restored", {
  ctx <- z3_context()
  x <- z3_int(ctx, "x")
  s <- z3_solver(ctx)
  z3_add(s, x > 0, x < 10)
  expect_identical(z3_check(s, timeout = 60000), "sat")
  expect_true(z3_value(z3_eval(z3_model(s), x)) %in% 1:9)
  # Without a timeout of its own, the solver is left with none (Z3's
  # "unlimited" value).
  expect_output(print(s), "timeout 4294967295")
  z3_set_params(s, timeout = 123)
  expect_output(print(s), "timeout 123")
  expect_identical(z3_check(s, x > 5, timeout = 60000), "sat")
  expect_output(print(s), "timeout 123")
  # Assumptions still work with a timeout.
  expect_identical(z3_check(s, x > 20, timeout = 60000), "unsat")
})

test_that("a per-call timeout stops a hard check", {
  ctx <- z3_context()
  s <- z3_solver(ctx)
  z3_add(s, hard_constraints(ctx))
  status <- z3_check(s, timeout = 1)
  expect_output(print(s), "timeout 4294967295")
  skip_if(status != "unknown", "Z3 finished within the time limit")
  expect_match(z3_reason_unknown(s), "timeout|canceled")
})

test_that("other parameters survive a per-call timeout", {
  ctx <- z3_context()
  s <- z3_solver(ctx, random_seed = 7)
  z3_add(s, z3_int(ctx, "x") > 0)
  z3_check(s, timeout = 60000)
  expect_output(print(s), "random_seed 7")
})

test_that("the timeout is restored when the check fails", {
  skip_if_not(z3_version_at_least("4.12.3"))
  ctx <- z3_context()
  s <- z3_solver(ctx, timeout = 123)
  z3_add(s, pigeonhole(ctx, 3))
  z3_on_clause(s, function(hint, deps, lits) stop("clause callback failed"))
  expect_error(z3_check(s, timeout = 60000), "clause callback failed")
  expect_output(print(s), "timeout 123")
})

test_that("z3_interrupt() does nothing when no check runs", {
  ctx <- z3_context()
  s <- z3_solver(ctx)
  expect_error(z3_interrupt(ctx), "solver must be a z3_solver")
  expect_invisible(z3_interrupt(s))
  z3_add(s, z3_int(ctx, "x") > 0)
  expect_identical(z3_check(s), "sat")
  z3_interrupt(s)
  expect_identical(z3_check(s), "sat")
})

test_that("a callback can interrupt its own check", {
  skip_if_not(z3_version_at_least("4.12.3"))
  ctx <- z3_context()
  s <- z3_solver(ctx)
  z3_add(s, pigeonhole(ctx, 9))
  calls <- 0
  z3_on_clause(s, function(hint, deps, lits) {
    calls <<- calls + 1
    z3_interrupt(s)
  })
  # The timeout only guards against a hang; the interrupt comes first.
  expect_identical(z3_check(s, timeout = 60000), "unknown")
  expect_gt(calls, 0)
  expect_match(z3_reason_unknown(s), "interrupted|canceled")
  # The solver stays usable.
  z3_reset(s)
  z3_add(s, z3_int(ctx, "x") > 0)
  expect_identical(z3_check(s), "sat")
})

test_that("statistics as numbers and as text", {
  ctx <- z3_context()
  s <- z3_solver(ctx)
  z3_add(s, pigeonhole(ctx, 3))
  expect_identical(z3_check(s), "unsat")
  st <- z3_statistics(s)
  expect_gt(length(st), 0)
  expect_true(all(vapply(st, is.numeric, logical(1))))
  # Every entry is a whole number or a double (time, memory); none is
  # of a kind z3r does not read.
  expect_false(anyNA(unlist(st)))
  txt <- z3_statistics_text(s)
  expect_type(txt, "character")
  expect_length(txt, 1L)
  expect_match(txt, "^\\(:")
  # Z3 writes each key as an SMT-LIB keyword, with "-" in place of spaces.
  key <- gsub("[^[:alnum:]~!@$%^&*_+=<>.?/-]", "-", sub("^:", "", names(st)[[1]]))
  expect_match(txt, paste0(":", key), fixed = TRUE)
  fp <- z3_fixedpoint(ctx)
  expect_type(z3_statistics_text(fp), "character")
  opt <- z3_optimize(ctx)
  z3_add(opt, z3_int(ctx, "y") >= 0)
  z3_minimize(opt, z3_int(ctx, "y"))
  z3_check(opt)
  expect_type(z3_statistics_text(opt), "character")
  expect_error(z3_statistics_text(ctx), "x must be a z3_solver")
})

test_that("z3_help() lists parameter names and simplifier help", {
  ctx <- z3_context()
  s <- z3_solver(ctx)
  n <- z3_help(s, names_only = TRUE)
  expect_type(n, "character")
  expect_match(n, "^\\(.*\\)$")
  expect_match(n, "timeout")
  expect_false(grepl("unsigned int", n, fixed = TRUE))
  expect_match(z3_help(z3_optimize(ctx), names_only = TRUE), "priority")
  expect_match(z3_help(z3_tactic(ctx, "simplify"), names_only = TRUE), "som")
  expect_match(z3_help(ctx, names_only = TRUE), "som")
  expect_match(z3_help(z3_fixedpoint(ctx), names_only = TRUE), "engine")
  if (z3_version_at_least("4.12.2")) {
    expect_match(z3_help(z3_simplifier(ctx, "solve-eqs"), names_only = TRUE), "^\\(.*\\)$")
  }
  h <- z3_help(ctx)
  expect_type(h, "character")
  expect_match(h, "som")
  expect_gt(nchar(h), nchar(z3_help(ctx, names_only = TRUE)))
  expect_error(z3_help(s, names_only = NA), "names_only must be TRUE or FALSE")
  expect_error(z3_help(s, names_only = "yes"), "names_only must be TRUE or FALSE")
  expect_error(z3_help(1), "x must be a solver")
})

test_that("applying a tactic returns a printable result", {
  ctx <- z3_context()
  x <- z3_int(ctx, "x")
  g <- z3_goal(ctx)
  z3_add(g, x + 0 > 2, x < 10)
  res <- z3_apply_tactic(z3_tactic(ctx, "simplify"), g)
  expect_s3_class(res, "z3_apply_result")
  expect_length(res, 1L)
  expect_s3_class(res[[1]], "z3_goal")
  expect_match(format(res), "goals")
  expect_output(print(res), "<z3_apply_result: 1 subgoal>")
  expect_output(print(res), "goals")
  g2 <- z3_goal(ctx)
  z3_add(g2, x < 0 | x > 5)
  split <- z3_apply_tactic(z3_tactic(ctx, "split-clause"), g2)
  expect_length(split, 2L)
  expect_output(print(split), "2 subgoals")
})

test_that("optimizer checks take a per-call timeout", {
  ctx <- z3_context()
  x <- z3_int(ctx, "x")
  opt <- z3_optimize(ctx)
  z3_add(opt, x >= 0, x <= 50)
  obj <- z3_maximize(opt, x)
  expect_identical(z3_check(opt, timeout = 60000), "sat")
  expect_identical(z3_value(z3_optimum(obj)$value), 50)
  expect_output(print(opt), "timeout 4294967295")
  z3_set_params(opt, timeout = 456)
  z3_check(opt, timeout = 60000)
  expect_output(print(opt), "timeout 456")
})

test_that("objective values after a failed check are refused", {
  ctx <- z3_context()
  x <- z3_int(ctx, "x")
  opt <- z3_optimize(ctx)
  z3_add(opt, x >= 0, x <= 50)
  obj <- z3_maximize(opt, x)
  z3_on_model(opt, function(m) stop("model callback failed"))
  expect_error(z3_check(opt), "model callback failed")
  expect_error(z3_optimum(obj), "last z3_check\\(\\) failed")
})

test_that("an optimizer stopped early reports the best value so far", {
  ctx <- z3_context()
  x <- z3_int(ctx, "x")
  opt <- z3_optimize(ctx)
  z3_add(opt, hard_constraints(ctx))
  obj <- z3_maximize(opt, x)
  status <- z3_check(opt, timeout = 1)
  skip_if(status != "unknown", "Z3 finished within the time limit")
  o <- z3_optimum(obj)
  expect_identical(o$status, "unknown")
  expect_true(is.null(o$value) || inherits(o$value, "z3_expr"))
  expect_type(o$lower, "character")
  expect_type(o$upper, "character")
  expect_output(print(o), "unknown|best so far")
  # A model is still refused after "unknown".
  expect_error(z3_model(opt), "last result was \"unknown\"")
})
