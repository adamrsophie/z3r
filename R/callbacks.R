# Called from C++ for every R callback. Returns list(value, error message,
# interrupted).
callback_invoke <- function(fn, ...) {
  tryCatch(list(fn(...), NULL, FALSE),
           error = function(e) list(NULL, conditionMessage(e), FALSE),
           interrupt = function(e) list(NULL, "interrupted by the user", TRUE))
}

#' User propagators
#'
#' A user propagator lets R code take part in Z3's search: Z3 tells it when
#' registered terms get fixed values or become equal, and it can respond by
#' propagating consequences or reporting conflicts. This implements custom
#' theories and constraints that are awkward to state as formulas. Needs Z3
#' 4.12.3 or later.
#'
#' `z3_propagator(solver, ...)` installs the callbacks, given as named
#' functions. `push(cb)` and `pop(cb, n)` are required: Z3 opens and closes
#' backtracking scopes, and the propagator must undo its own state on
#' `pop`. The optional callbacks are `fixed(cb, term, value)` (a registered
#' term got a value), `eq(cb, a, b)` and `diseq(cb, a, b)` (registered terms
#' became equal or different), `final(cb)` (a full assignment exists; the
#' last chance to object), `created(cb, term)` (a term using a function from
#' `z3_propagator_function()` appeared), `decide(cb, term, index, phase)`
#' (Z3 is about to split on a term) and `binding(cb, quantifier,
#' instance)` (Z3 4.15.3; return `FALSE` to block a quantifier instance).
#'
#' Terms are registered before the check with `z3_propagate_register()`, or
#' inside a callback with `z3_propagate_register_cb()`. Inside a callback,
#' `cb` is a handle valid only until the callback returns:
#'
#' * `z3_propagate_consequence(cb, fixed, consequence, lhs, rhs)` states
#'   that `consequence` follows from the values of the `fixed` terms and the
#'   equalities `lhs[[i]] == rhs[[i]]`. With `consequence = FALSE` it is a
#'   conflict, and Z3 backtracks.
#' * `z3_propagate_conflict(cb, fixed)` is the same with `FALSE`.
#' * `z3_next_split(cb, term, index, phase)` suggests the next decision.
#'
#' An error in a callback interrupts the check and is raised as an R error
#' (an interrupt with Ctrl-C is raised as an interrupt). While a check runs,
#' its solver cannot be used from its own callbacks: `z3_add()`,
#' `z3_push()`, `z3_check()` and every other function on it fail; act
#' through the callback handle instead.
#'
#' Z3 cannot copy R callbacks. [z3_translate()] and
#' [z3_solver_add_simplifier()] refuse solvers that have them, and when Z3
#' copies a solver internally during a check (as in parallel mode), z3r
#' refuses a `"sat"` result, which the copy may have found without the
#' propagator's constraints. [z3_reset()] removes a solver's callbacks;
#' install them again afterwards.
#'
#' `z3_on_clause(solver, fn)` registers `fn(proof_hint, deps, literals)`,
#' called for every clause the solver asserts, infers or deletes (Z3
#' 4.12.3). `z3_on_model(optimizer, fn)` registers `fn(model)`, called with
#' each improved model during optimization. `z3_set_proof_checker(ctx, fn)`
#' (Z3 5.3.2) registers `fn(proof, assertions)`, called by the SMT-LIB2
#' interpreter ([z3_eval_smtlib()]) after unsatisfiable checks when proofs
#' are enabled; it returns a report string, or `NULL` for an empty report,
#' and an R error in it is raised when [z3_eval_smtlib()] returns. The
#' checker stays registered while the context object exists; pass
#' `fn = NULL` to remove it. It cannot replace itself or call
#' [z3_eval_smtlib()] on its own context.
#'
#' Callback functions are owned by the solver, optimizer or context they
#' are registered with, so closures that refer back to that object do not
#' keep it alive.
#'
#' @param solver A `z3_solver`.
#' @param optimizer A `z3_optimize`.
#' @param ctx A [z3_context()].
#' @param push,pop,eq,diseq,final,created,decide,binding Callback
#'   functions (see Details).
#' @param fixed For `z3_propagator()`, the callback for fixed values (see
#'   Details); for `z3_propagate_consequence()` and
#'   `z3_propagate_conflict()`, a list of fixed terms justifying the
#'   consequence.
#' @param cb The callback handle passed to a callback.
#' @param term,e An expression (Bool or bit-vector for registration).
#' @param consequence A Boolean expression.
#' @param lhs,rhs Lists of terms whose equalities justify a consequence.
#' @param index Bit index for bit-vector splits.
#' @param phase `TRUE`, `FALSE` or `NA` (let Z3 choose).
#' @param name,domain,range Name and signature of a propagator function.
#' @param fn A callback function.
#' @return `z3_propagator()`, `z3_on_clause()`, `z3_on_model()` and
#'   `z3_propagate_register()` return their first argument invisibly;
#'   `z3_propagator_function()` returns a `z3_function`; the in-callback
#'   functions return `TRUE` when Z3 accepted the request.
#' @examples
#' ctx <- z3_context()
#' if (z3_version_at_least("4.12.3")) {
#'   # A propagator that forbids more than two of four Booleans being true.
#'   p <- z3_bools(ctx, paste0("p", 1:4))
#'   s <- z3_solver(ctx)
#'   trail <- list()
#'   true_terms <- list()
#'   z3_propagator(s,
#'     push = function(cb) trail[[length(trail) + 1]] <<- length(true_terms),
#'     pop = function(cb, n) {
#'       keep <- trail[[length(trail) - n + 1]]
#'       true_terms <<- true_terms[seq_len(keep)]
#'       trail <<- trail[seq_len(length(trail) - n)]
#'     },
#'     fixed = function(cb, term, value) {
#'       if (isTRUE(z3_value(value))) {
#'         true_terms[[length(true_terms) + 1]] <<- term
#'         if (length(true_terms) > 2) z3_propagate_conflict(cb, true_terms)
#'       }
#'     })
#'   for (x in p) z3_propagate_register(s, x)
#'   z3_add(s, z3_or(p[1:2]), z3_or(p[3:4]))
#'   z3_check(s)
#'   sum(unlist(z3_model_values(z3_model(s))))
#' }
#' @export
z3_propagator <- function(solver, push, pop, fixed = NULL, eq = NULL, diseq = NULL,
                          final = NULL, created = NULL, decide = NULL, binding = NULL) {
  fns <- Filter(Negate(is.null), list(push = push, pop = pop, fixed = fixed, eq = eq, diseq = diseq,
                                      final = final, created = created, decide = decide,
                                      binding = binding))
  for (nm in names(fns)) {
    if (!is.function(fns[[nm]])) stop(sprintf("%s must be a function", nm), call. = FALSE)
  }
  cpp_propagate_init(solver, c(fns, list(.invoke = callback_invoke)))
  invisible(solver)
}

#' @rdname z3_propagator
#' @export
z3_propagate_register <- function(solver, e) {
  cpp_propagate_register(solver, e)
  invisible(solver)
}

#' @rdname z3_propagator
#' @export
z3_propagator_function <- function(ctx, name, domain, range) {
  decl_function(cpp_propagate_declare(ctx, name, as_list_sorts(domain), range))
}

#' @rdname z3_propagator
#' @export
z3_propagate_consequence <- function(cb, fixed = list(), consequence, lhs = list(), rhs = list()) {
  cpp_cb_consequence(cb, as_list(fixed), as_list(lhs), as_list(rhs), consequence)
}

#' @rdname z3_propagator
#' @export
z3_propagate_conflict <- function(cb, fixed = list(), lhs = list(), rhs = list()) {
  cpp_cb_consequence(cb, as_list(fixed), as_list(lhs), as_list(rhs), FALSE)
}

#' @rdname z3_propagator
#' @export
z3_propagate_register_cb <- function(cb, e) invisible(cpp_cb_register(cb, e))

#' @rdname z3_propagator
#' @export
z3_next_split <- function(cb, term, index = 0, phase = NA) {
  ph <- if (is.na(phase)) 0L else if (isTRUE(phase)) 1L else -1L
  cpp_cb_next_split(cb, term, as_count(index, "index"), ph)
}

#' @rdname z3_propagator
#' @export
z3_on_clause <- function(solver, fn) {
  if (!is.function(fn)) stop("fn must be a function", call. = FALSE)
  cpp_on_clause(solver, list(clause = fn, .invoke = callback_invoke))
  invisible(solver)
}

#' @rdname z3_propagator
#' @export
z3_on_model <- function(optimizer, fn) {
  if (!is.function(fn)) stop("fn must be a function", call. = FALSE)
  cpp_on_model(optimizer, list(model = fn, .invoke = callback_invoke))
  invisible(optimizer)
}

#' @rdname z3_propagator
#' @export
z3_set_proof_checker <- function(ctx, fn) {
  if (is.null(fn)) return(invisible(cpp_set_proof_check(ctx, NULL)))
  if (!is.function(fn)) stop("fn must be a function or NULL", call. = FALSE)
  cpp_set_proof_check(ctx, list(check = fn, .invoke = callback_invoke))
  invisible(ctx)
}

#' @export
print.z3_callback <- function(x, ...) {
  cat("<z3_callback>", if (!cpp_is_valid(x)) "(expired)", "\n")
  invisible(x)
}
