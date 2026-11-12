test_that("a user propagator enforces a custom constraint", {
  skip_if_not(z3_version_at_least("4.12.3"))
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
  skip_if_not(z3_version_at_least("4.12.3"))
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
  expect_gt(finals, 0)
  z3_add(s, !b)
  expect_identical(z3_check(s), "unsat")
})

test_that("errors in callbacks are raised after the check", {
  skip_if_not(z3_version_at_least("4.12.3"))
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
  z3_propagator(s2, push = function(cb) NULL, pop = function(cb, n) NULL,
                fixed = function(cb, t, v) kept <<- cb)
  z3_propagate_register(s2, p)
  z3_add(s2, p)
  z3_check(s2)
  expect_s3_class(kept, "z3_callback")
  expect_error(z3_propagate_conflict(kept, list(p)), "has expired")
})

test_that("callbacks cannot reenter or change their own solver", {
  skip_if_not(z3_version_at_least("4.12.3"))
  ctx <- z3_context()
  p <- z3_bool(ctx, "p")
  for (action in list(function(s) z3_push(s), function(s) z3_add(s, p), function(s) z3_check(s),
                      function(s) z3_reset(s), function(s) z3_pop(s))) {
    s <- z3_solver(ctx)
    z3_propagator(s, push = function(cb) NULL, pop = function(cb, n) NULL,
                  fixed = function(cb, t, v) action(s))
    z3_propagate_register(s, p)
    z3_add(s, p)
    expect_error(z3_check(s), "running a check")
    # The solver is usable again afterwards.
    expect_identical(z3_num_scopes(s), 0L)
  }
})

test_that("resetting a solver removes its callbacks", {
  skip_if_not(z3_version_at_least("4.12.3"))
  ctx <- z3_context()
  p <- z3_bools(ctx, c("a", "b"))
  install <- function(s) {
    z3_propagator(s, push = function(cb) NULL, pop = function(cb, n) NULL,
                  fixed = function(cb, t, v) if (isTRUE(z3_value(v))) z3_propagate_conflict(cb, list(t)))
    for (x in p) z3_propagate_register(s, x)
  }
  s <- z3_solver(ctx)
  install(s)
  z3_add(s, z3_or(p))
  expect_identical(z3_check(s), "unsat")
  z3_reset(s)
  z3_add(s, z3_or(p))
  expect_identical(z3_check(s), "sat")
  install(s)
  z3_add(s, z3_or(p))
  expect_identical(z3_check(s), "unsat")
})

test_that("solvers with callbacks are not copied without them", {
  skip_if_not(z3_version_at_least("4.12.3"))
  ctx <- z3_context()
  p <- z3_bool(ctx, "p")
  s <- z3_solver(ctx)
  z3_propagator(s, push = function(cb) NULL, pop = function(cb, n) NULL)
  expect_error(z3_translate(s, z3_context()), "cannot copy a solver with R callbacks")
  o <- z3_optimize(ctx)
  z3_on_model(o, function(m) NULL)
  if (z3_version_at_least("4.15.5")) expect_error(z3_translate(o, z3_context()), "cannot copy")
})

test_that("errors in pop callbacks are raised by z3_pop()", {
  skip_if_not(z3_version_at_least("4.12.3"))
  ctx <- z3_context()
  p <- z3_bool(ctx, "p")
  s <- z3_solver(ctx)
  z3_propagator(s, push = function(cb) NULL, pop = function(cb, n) stop("pop failed"),
                fixed = function(cb, t, v) NULL)
  z3_propagate_register(s, p)
  z3_push(s)
  z3_add(s, p)
  z3_check(s)
  res <- tryCatch({ z3_pop(s); "no error" }, error = function(e) conditionMessage(e))
  # Z3 calls pop on its own schedule; if it ran the callback here, its error is raised here.
  expect_true(res == "no error" || grepl("pop failed", res))
})

test_that("callback closures do not keep their solver alive", {
  skip_if_not(z3_version_at_least("4.12.3"))
  ctx <- z3_context()
  collected <- 0
  make <- function() {
    e <- new.env()
    reg.finalizer(e, function(e) collected <<- collected + 1)
    s <- z3_solver(ctx)
    # The closures refer to their environment, which holds s and e.
    z3_propagator(s, push = function(cb) e, pop = function(cb, n) s)
    z3_on_clause(s, function(hint, deps, lits) s)
    invisible(NULL)
  }
  for (i in 1:20) make()
  gc()
  gc()
  expect_identical(collected, 20)
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
  skip_if_not(z3_version_at_least("4.12.3"))
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
  # NULL is an empty report; an R error is raised after the interpreter.
  z3_set_proof_checker(ctx, function(proof, assertions) NULL)
  expect_match(z3_eval_smtlib(ctx, "(assert p) (assert (not p)) (check-sat)"), "unsat")
  z3_set_proof_checker(ctx, function(proof, assertions) stop("bad proof"))
  expect_error(z3_eval_smtlib(ctx, "(assert p) (assert (not p)) (check-sat)"), "bad proof")
  # The checker cannot replace itself or reenter the interpreter.
  z3_set_proof_checker(ctx, function(proof, assertions) {
    z3_set_proof_checker(ctx, NULL)
    "checked"
  })
  expect_error(z3_eval_smtlib(ctx, "(assert p) (assert (not p)) (check-sat)"), "while it runs")
  z3_set_proof_checker(ctx, function(proof, assertions) z3_eval_smtlib(ctx, "(check-sat)"))
  expect_error(z3_eval_smtlib(ctx, "(assert p) (assert (not p)) (check-sat)"), "inside a proof checker")
  z3_set_proof_checker(ctx, NULL)
})

test_that("a proof checker does not keep its context alive", {
  skip_if_not(z3_version_at_least("5.3.2"))
  collected <- 0
  make <- function() {
    ctx <- z3_context(proof = TRUE)
    e <- new.env()
    reg.finalizer(e, function(e) collected <<- collected + 1)
    z3_set_proof_checker(ctx, function(proof, assertions) { e; ctx; "ok" })
    invisible(NULL)
  }
  for (i in 1:10) make()
  gc()
  gc()
  expect_identical(collected, 10)
})

test_that("callbacks run by assertions and registration are guarded too", {
  skip_if_not(z3_version_at_least("4.12.3"))
  ctx <- z3_context()
  p <- z3_bool(ctx, "p")
  s <- z3_solver(ctx, simple = TRUE)
  pops <- 0
  z3_propagator(s, push = function(cb) NULL,
                pop = function(cb, n) {
                  pops <<- pops + 1
                  z3_reset(s)
                },
                fixed = function(cb, term, value) NULL)
  z3_propagate_register(s, p)
  z3_check(s)
  # Asserting after the check backtracks the search, which runs pop.
  expect_error(z3_add(s, p), "running a check")
  expect_gt(pops, 0)
  expect_identical(z3_check(s), "sat")
})
