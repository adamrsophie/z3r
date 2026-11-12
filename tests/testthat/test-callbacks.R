test_that("a user propagator enforces a custom constraint", {
  skip_if_not(z3_version_at_least("4.13.0"))
  ctx <- z3_context()
  p <- z3_bools(ctx, paste0("p", 1:4))
  s <- z3_solver(ctx)
  trail <- list()
  true_terms <- list()
  fixed_calls <- 0
  z3_propagator(s,
    push = function(cb) trail[[length(trail) + 1]] <<- length(true_terms),
    pop = function(cb, n) {
      keep <- trail[[length(trail) - n + 1]]
      true_terms <<- true_terms[seq_len(keep)]
      trail <<- trail[seq_len(length(trail) - n)]
    },
    fixed = function(cb, term, value) {
      fixed_calls <<- fixed_calls + 1
      if (isTRUE(z3_value(value))) {
        true_terms[[length(true_terms) + 1]] <<- term
        if (length(true_terms) > 2) z3_propagate_conflict(cb, true_terms)
      }
    })
  for (x in p) z3_propagate_register(s, x)
  z3_add(s, z3_or(p[1:2]), z3_or(p[3:4]))
  expect_identical(z3_check(s), "sat")
  expect_gt(fixed_calls, 0)
  expect_lte(sum(unlist(z3_model_values(z3_model(s)))), 2)
  # With three forced, the propagator makes the problem unsatisfiable.
  z3_add(s, p$p1, p$p2, p$p3)
  expect_identical(z3_check(s), "unsat")
})

test_that("final callbacks and consequences", {
  skip_if_not(z3_version_at_least("4.13.0"))
  ctx <- z3_context()
  a <- z3_bool(ctx, "a")
  b <- z3_bool(ctx, "b")
  s <- z3_solver(ctx)
  finals <- 0
  z3_propagator(s, push = function(cb) NULL, pop = function(cb, n) NULL,
    fixed = function(cb, term, value) {
      # a implies b, enforced by the propagator
      if (z3_identical(term, a) && isTRUE(z3_value(value))) z3_propagate_consequence(cb, list(a), b)
    },
    final = function(cb) finals <<- finals + 1)
  z3_propagate_register(s, a)
  z3_propagate_register(s, b)
  z3_add(s, a)
  expect_identical(z3_check(s), "sat")
  expect_true(z3_value(z3_eval(z3_model(s), b)))
  z3_add(s, !b)
  expect_identical(z3_check(s), "unsat")
})

test_that("errors in callbacks are raised after the check", {
  skip_if_not(z3_version_at_least("4.13.0"))
  ctx <- z3_context()
  p <- z3_bool(ctx, "p")
  s <- z3_solver(ctx)
  z3_propagator(s, push = function(cb) NULL, pop = function(cb, n) NULL,
                fixed = function(cb, t, v) stop("boom"))
  z3_propagate_register(s, p)
  z3_add(s, p)
  expect_error(z3_check(s), "error in the R callback 'fixed': boom")
  expect_error(z3_propagator(s, push = function(cb) NULL, pop = function(cb, n) NULL), "already has")
  expect_error(z3_propagator(z3_solver(ctx), push = 1, pop = function(cb, n) NULL), "must be a function")
  expect_error(z3_propagate_register(z3_solver(ctx), p), "install a user propagator")
  # Callback handles expire when the callback returns.
  kept <- NULL
  s2 <- z3_solver(ctx)
  z3_propagator(s2, push = function(cb) kept <<- cb, pop = function(cb, n) NULL)
  z3_add(s2, p | !p)
  z3_check(s2)
  if (!is.null(kept)) expect_error(z3_propagate_conflict(kept, list(p)), "no longer valid")
})

test_that("clause and model callbacks", {
  ctx <- z3_context()
  x <- z3_int(ctx, "x")
  o <- z3_optimize(ctx)
  models <- 0
  z3_on_model(o, function(m) {
    stopifnot(inherits(m, "z3_model"))
    models <<- models + 1
  })
  z3_add(o, x >= 0, x <= 50)
  z3_maximize(o, x)
  expect_identical(z3_check(o), "sat")
  expect_gt(models, 0)
  skip_if_not(z3_version_at_least("4.13.0"))
  p <- z3_bools(ctx, paste0("w", 1:6))
  s <- z3_solver(ctx)
  clauses <- 0
  z3_on_clause(s, function(hint, deps, lits) clauses <<- clauses + 1)
  z3_add(s, z3_atleast(p, k = 4), z3_atmost(p, k = 3))
  expect_identical(z3_check(s), "unsat")
  expect_gt(clauses, 0)
})

test_that("proof checker callback", {
  skip_if_not(z3_version_at_least("5.3.2"))
  ctx <- z3_context(proof = TRUE)
  seen <- 0
  z3_set_proof_checker(ctx, function(proof, assertions) {
    seen <<- seen + 1
    "checked"
  })
  out <- z3_eval_smtlib(ctx, paste("(set-option :produce-proofs true) (declare-const p Bool)",
                                   "(assert p) (assert (not p)) (check-sat)"))
  expect_match(out, "unsat")
  expect_gt(seen, 0)
  z3_set_proof_checker(ctx, NULL)
})
