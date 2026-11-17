test_that("SMT-LIB text cannot redeclare a datatype the context has", {
  ctx <- z3_context()
  z3_from_smtlib(ctx, "(declare-datatypes ((E 0)) (((a) (b)))) (declare-const x E) (assert (= x a))")
  expect_error(z3_from_smtlib(ctx, "(declare-datatypes ((E 0)) (((a) (b) (c))))"), "already declared")
  expect_error(z3_datatype(ctx, "E", a = list(), k = list()), "already declared")
  z3_enum_sort(ctx, "Color", c("red", "green"))
  expect_error(z3_add_smtlib(z3_solver(ctx), "(declare-datatype Color ((red) (green) (blue)))"),
               "already declared")
  # Comments and strings are not declarations.
  expect_length(z3_from_smtlib(ctx, "; (declare-datatype Color ((x)))\n(assert true)"), 1L)
})

test_that(":named assertions parsed into a solver are exported with their labels", {
  ctx <- z3_context()
  s <- z3_solver(ctx)
  ok <- tryCatch({
    z3_add_smtlib(s, "(set-option :produce-unsat-cores true)(declare-const p Bool)
      (assert (! p :named a1))(assert (! (not p) :named a2))")
    TRUE
  }, error = function(e) {
    # Older Z3 releases refuse the option on a solver that already exists.
    if (grepl("cannot be modified", conditionMessage(e))) FALSE else stop(e)
  })
  skip_if_not(ok, "this Z3 cannot enable unsat cores from SMT-LIB text")
  expect_identical(z3_check(s), "unsat")
  s2 <- z3_solver(ctx)
  z3_add(s2, z3_from_smtlib(ctx, z3_to_smtlib(s)))
  expect_identical(z3_check(s2), "unsat")
})

test_that("labels, model targets and model values are checked", {
  ctx <- z3_context()
  x <- z3_int(ctx, "x")
  y <- z3_int(ctx, "y")
  p <- z3_bool(ctx, "p")
  q <- z3_bool(ctx, "q")
  expect_error(z3_assert_and_track(z3_solver(ctx), x < 0, p & q), "Boolean constant")
  m <- z3_model(ctx)
  expect_error(z3_model_set(m, z3_bool_val(ctx, TRUE), FALSE), "only constants")
  expect_error(z3_model_set(m, x, z3_bound_var(ctx, 0, z3_int_sort(ctx))), "bound variables")
  z3_model_set(m, y, 3)
  z3_model_set(m, x, y)
  expect_equal(z3_value(m$x), 3)
  m2 <- z3_model(ctx)
  expect_error(z3_eval(m2, list(x, NA), completion = TRUE))
  expect_identical(length(m2), 0L)
})

test_that("names keep their meaning across constructors and exports", {
  ctx <- z3_context()
  n <- iconv(intToUtf8(233), "UTF-8", "latin1")
  Encoding(n) <- "latin1"
  expect_true(z3_identical(z3_int(ctx, n), z3_ints(ctx, n)[[1]]))
  b <- rawToChar(as.raw(233))
  Encoding(b) <- "bytes"
  expect_error(z3_int(ctx, b), "bytes")
  s <- z3_solver(ctx)
  z3_add(s, z3_bool(ctx, "w"), z3_int(ctx, "w") == 0)
  expect_error(z3_to_smtlib(s), "two different declarations")
  s <- z3_solver(ctx)
  z3_add(s, z3_int(ctx, "1.5") > 0)
  expect_error(z3_to_smtlib(s), "number or keyword")
  # Constructor and field names that abbreviate ctx or name stay names.
  d <- z3_datatype(ctx, "E", a = list(), b = list(), c = list())
  expect_true(all(c("a", "b", "c") %in% names(d)))
  tp <- z3_tuple_sort(ctx, "T", c = z3_int_sort(ctx), n = z3_int_sort(ctx))
  expect_true(all(c("c", "n") %in% names(tp)))
})

test_that("optimizers keep scopes, weights and imports honest", {
  ctx <- z3_context()
  p <- z3_bool(ctx, "p")
  o <- z3_optimize(ctx)
  z3_push(o)
  z3_add(o, FALSE)
  expect_error(z3_pop(o, 2), "cannot pop 2")
  expect_identical(z3_num_scopes(o), 1L)
  expect_identical(z3_check(o), "unsat")
  o <- z3_optimize(ctx)
  z3_add_soft(o, p, "3339/10000")
  expect_error(z3_to_smtlib(o), "three-decimal")
  z3_push(o)
  z3_pop(o)
  expect_error(z3_to_smtlib(o), "three-decimal")
  o <- z3_optimize(ctx)
  z3_push(o)
  z3_add_soft(o, p, "1/3")
  z3_pop(o)
  expect_type(z3_to_smtlib(o), "character")
  expect_error(z3_add_smtlib(z3_optimize(z3_context()), "(set-option :opt.priority box)"), "z3_set_params")
  o <- z3_optimize(ctx)
  z3_check(o)
  expect_error(z3_add_smtlib(o, "(maximize 1) (assert missing)"))
  expect_output(print(o), "not checked")
})

test_that("a translated optimizer still enforces tracked constraints", {
  skip_if_not(z3_version_at_least("4.15.5"))
  ctx <- z3_context()
  o <- z3_optimize(ctx)
  z3_assert_and_track(o, FALSE, "t")
  expect_identical(z3_check(z3_translate(o, z3_context())), "unsat")
})

test_that("user propagators are installed before searching and block export", {
  skip_if_not(z3_version_at_least("4.12.3"))
  ctx <- z3_context()
  p <- z3_bool(ctx, "p")
  forbid <- function(s) {
    z3_propagator(s, push = function(...) NULL, pop = function(...) NULL,
                  fixed = function(cb, term, value) z3_propagate_conflict(cb, term))
    z3_propagate_register(s, p)
  }
  s <- z3_solver(ctx, simple = TRUE)
  z3_add(s, p)
  z3_check(s)
  expect_error(forbid(s), "before the solver's first check")
  s <- z3_solver(ctx, simple = TRUE)
  z3_add(s, p)
  forbid(s)
  expect_error(z3_to_smtlib(s), "user propagator")
  expect_error(z3_to_dimacs(s), "user propagator")
})

test_that("simplifier inputs parsed in a popped scope do not block export", {
  skip_if_not(z3_version_at_least("4.12.2"))
  ctx <- z3_context()
  s <- z3_solver_add_simplifier(z3_solver(ctx), z3_simplifier(ctx, "simplify"))
  z3_push(s)
  z3_add_smtlib(s, "(assert false)")
  z3_pop(s)
  expect_match(z3_check_result(s)$fingerprint, "^[0-9a-f]{16}$")
})

test_that("numbers do not depend on the numeric locale", {
  ctx <- z3_context()
  old <- Sys.getlocale("LC_NUMERIC")
  on.exit(suppressWarnings(Sys.setlocale("LC_NUMERIC", old)))
  ok <- suppressWarnings(nzchar(Sys.setlocale("LC_NUMERIC", "de_DE.UTF-8")))
  skip_if_not(ok, "no de_DE.UTF-8 locale")
  expect_equal(z3_value(z3_real_val(ctx, "1/3"), exact = FALSE), 1 / 3)
  expect_identical(format(z3_real(ctx, "r") + 0.5), "(+ r (/ 1.0 2.0))")
})

test_that("other inputs that would change meaning are refused", {
  ctx <- z3_context()
  x <- z3_int(ctx, "x")
  y <- z3_int(ctx, "y")
  expect_error(z3_linear(list(row = 1L, col = 1L, value = list(y)), x, "==", 1), "not a number")
  expect_error(z3_linear(matrix(c(TRUE, FALSE), 1), list(x, y), "<=", 1), "numeric or character")
  expect_silent(z3_linear(matrix(1, 1, 2), list(x, y), "==", list(x)))
  skip_if_not_installed("bit64")
  expect_error(z3_fp_val(ctx, bit64::as.integer64("9007199254740993"), z3_fp128_sort(ctx)), "plain double")
})

test_that("check results fingerprint literal assumptions and objectives", {
  ctx <- z3_context()
  x <- z3_int(ctx, "x")
  s <- z3_solver(ctx)
  z3_add(s, x > 0)
  expect_false(identical(z3_check_result(s)$fingerprint, z3_check_result(s, FALSE)$fingerprint))
  o1 <- z3_optimize(ctx)
  z3_add(o1, x < 5)
  z3_maximize(o1, x)
  o2 <- z3_optimize(ctx)
  z3_add(o2, x < 5)
  z3_minimize(o2, x)
  expect_false(identical(z3_check_result(o1)$fingerprint, z3_check_result(o2)$fingerprint))
})

test_that("callback errors without a message are still errors", {
  ctx <- z3_context()
  x <- z3_int(ctx, "x")
  o <- z3_optimize(ctx)
  z3_add(o, x >= 0, x <= 3)
  z3_maximize(o, x)
  z3_on_model(o, function(model) {
    stop(structure(class = c("e", "error", "condition"), list(message = NULL, call = NULL)))
  })
  expect_error(z3_check(o), "without a message")
})

test_that("symcompR symbols translate only to Int, Real and Bool", {
  skip_if_not_installed("symcompR")
  ctx <- z3_context()
  bv <- z3_bv_sort(ctx, 8)
  expect_error(z3_from_symbolic(ctx, symcompR::Symbol("x") + symcompR::Symbol("y"),
                                sorts = list(x = bv, y = bv)), "Int, Real and Bool")
  xs <- lapply(sprintf("x%04d", 1:300), symcompR::Symbol, integer = TRUE)
  expect_s3_class(z3_from_symbolic(ctx, do.call(symcompR::Add, xs)), "z3_expr")
})
