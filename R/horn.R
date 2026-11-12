#' Horn clauses and Datalog: the fixedpoint engine
#'
#' Z3's fixedpoint engine decides reachability questions over recursive
#' relations: Datalog queries over finite relations, and constrained Horn
#' clauses (CHC) over arithmetic and other theories, solved by the Spacer
#' engine. Many program verification problems ("can this program reach an
#' error state?") are CHC problems.
#'
#' 1. Create an engine with `z3_fixedpoint(ctx)`.
#' 2. Declare relations as Boolean-valued functions ([z3_function()] with
#'    range Bool) and register them with `z3_register_relation()`.
#' 3. Add rules with `z3_add_rule()`: an implication `body => head`,
#'    usually universally quantified with [z3_forall()], or a fact. Rules
#'    may be named.
#' 4. Ask `z3_query(fp, q)` whether `q` is derivable. `"sat"` means yes;
#'    `z3_answer()` then gives a derivation (or the set of reachable
#'    tuples). `"unsat"` means no; `z3_answer()` gives an inductive
#'    invariant proving it.
#'
#' Spacer functions: `z3_ground_answer()` (a ground derivation),
#' `z3_answer_trace()` (rules used along the counterexample),
#' `z3_answer_rule_names()`, `z3_add_invariant()` (a known lemma) and
#' `z3_reachable()` (the reachable states found for a predicate).
#' `z3_cover()` and `z3_add_cover()` read and add properties of a
#' predicate at a search level (`-1` is the invariant level;
#' `z3_add_cover()` accepts levels up to 1024, or up to `z3_num_levels()`
#' when that is larger), and
#' `z3_query(fp, q, level)` starts the search at a given level.
#'
#' `z3_set_params(fp, engine = "datalog")` selects the Datalog engine;
#' parameters are listed by `z3_param_help(fp)`. `z3_add_fact()` adds a
#' tuple to a relation over finite domain sorts ([z3_finite_domain_sort()]).
#' `z3_fixedpoint_rules()` and `z3_fixedpoint_assertions()` list the
#' content; [z3_to_smtlib()] prints it, and [z3_add_smtlib()] parses rules
#' in Z3's fixedpoint format.
#'
#' @param ctx A [z3_context()].
#' @param fp A `z3_fixedpoint`.
#' @param relation A relation (a `z3_function` with Bool range).
#' @param rule A Boolean formula.
#' @param name Optional rule name.
#' @param update If `TRUE`, replace the rule of the same name.
#' @param args Indices of the elements of a fact.
#' @param query A Boolean formula, or a list of relations for
#'   `z3_query()`.
#' @param level Search level. For `z3_query()`, `NULL` (the default)
#'   searches from level 0 and a number starts the Spacer search at that
#'   level.
#' @param property A Boolean formula over the relation's arguments, given as
#'   bound variables (see [z3_bound_var()]).
#' @param kinds Relation representation names for
#'   `z3_set_representation()`.
#' @return `z3_fixedpoint()` returns an engine; `z3_query()` returns
#'   `"sat"`, `"unsat"` or `"unknown"`; the answer functions return
#'   expressions; the others return `fp` invisibly.
#' @examples
#' ctx <- z3_context()
#' I <- z3_int_sort(ctx)
#' B <- z3_bool_sort(ctx)
#' # Program: x := 0; while (*) x := x + 2. Can x ever be 7?
#' inv <- z3_function(ctx, "inv", I, B)
#' x <- z3_int(ctx, "x")
#' fp <- z3_fixedpoint(ctx)
#' z3_register_relation(fp, inv)
#' z3_add_rule(fp, inv(0))
#' z3_add_rule(fp, z3_forall(x, z3_implies(inv(x), inv(x + 2))))
#' z3_query(fp, z3_exists(x, inv(x) & x == 7))
#' z3_answer(fp)
#' z3_query(fp, z3_exists(x, inv(x) & x == 8))
#' @export
z3_fixedpoint <- function(ctx) cpp_mk_fixedpoint(ctx)

#' @export
print.z3_fixedpoint <- function(x, ...) {
  status <- cpp_fixedpoint_last_status(x)
  cat(sprintf("<z3_fixedpoint: %d rule%s, %s>\n", length(cpp_fixedpoint_contents(x, "rules")),
              if (length(cpp_fixedpoint_contents(x, "rules")) == 1L) "" else "s",
              if (is.na(status)) "not queried" else paste("last query:", status)))
  invisible(x)
}

#' @rdname z3_fixedpoint
#' @export
z3_register_relation <- function(fp, relation) {
  cpp_fixedpoint_register(fp, z3_decl(relation))
  invisible(fp)
}

#' @rdname z3_fixedpoint
#' @export
z3_add_rule <- function(fp, rule, name = "", update = FALSE) {
  cpp_fixedpoint_add_rule(fp, rule, name, isTRUE(update))
  invisible(fp)
}

#' @rdname z3_fixedpoint
#' @export
z3_add_fact <- function(fp, relation, args) {
  cpp_fixedpoint_add_fact(fp, z3_decl(relation), as.numeric(args))
  invisible(fp)
}

#' @export
z3_add.z3_fixedpoint <- function(x, ...) {
  for (a in flatten_args(...)) cpp_fixedpoint_assert(x, a)
  invisible(x)
}

#' @rdname z3_fixedpoint
#' @export
z3_query <- function(fp, query, level = NULL) {
  if (inherits(query, "z3_function") || inherits(query, "z3_func_decl")) query <- list(query)
  if (is.list(query)) query <- lapply(query, z3_decl)
  cpp_fixedpoint_query(fp, query, if (is.null(level)) -1L else as_count(level, "level"))
}

#' @rdname z3_fixedpoint
#' @export
z3_answer <- function(fp) cpp_fixedpoint_answer(fp, "answer")

#' @rdname z3_fixedpoint
#' @export
z3_ground_answer <- function(fp) cpp_fixedpoint_answer(fp, "ground")

#' @rdname z3_fixedpoint
#' @export
z3_answer_trace <- function(fp) cpp_fixedpoint_answer(fp, "trace")

#' @rdname z3_fixedpoint
#' @export
z3_answer_rule_names <- function(fp) cpp_fixedpoint_answer(fp, "names")

#' @export
z3_reason_unknown.z3_fixedpoint <- function(x) cpp_fixedpoint_reason_unknown(x)

#' @rdname z3_fixedpoint
#' @export
z3_cover <- function(fp, level, relation) {
  cpp_fixedpoint_cover(fp, as_level(level), z3_decl(relation), NULL, FALSE)
}

#' @rdname z3_fixedpoint
#' @export
z3_add_cover <- function(fp, level, relation, property) {
  cpp_fixedpoint_cover(fp, as_level(level), z3_decl(relation), property, TRUE)
  invisible(fp)
}

# A Spacer level: -1 (the invariant level) or a whole number from 0.
as_level <- function(level) {
  if (!is.numeric(level) || length(level) != 1L || is.na(level) || level != trunc(level) ||
      level < -1 || level > .Machine$integer.max) {
    stop("level must be -1 or a whole number of at least 0", call. = FALSE)
  }
  as.integer(level)
}

#' @rdname z3_fixedpoint
#' @export
z3_num_levels <- function(fp, relation) cpp_fixedpoint_num_levels(fp, z3_decl(relation))

#' @rdname z3_fixedpoint
#' @export
z3_add_invariant <- function(fp, relation, property) {
  cpp_fixedpoint_spacer(fp, "invariant", z3_decl(relation), property)
  invisible(fp)
}

#' @rdname z3_fixedpoint
#' @export
z3_reachable <- function(fp, relation) cpp_fixedpoint_spacer(fp, "reachable", z3_decl(relation), NULL)

#' @rdname z3_fixedpoint
#' @export
z3_fixedpoint_rules <- function(fp) cpp_fixedpoint_contents(fp, "rules")

#' @rdname z3_fixedpoint
#' @export
z3_fixedpoint_assertions <- function(fp) cpp_fixedpoint_contents(fp, "assertions")

#' @export
z3_assertions.z3_fixedpoint <- function(x) cpp_fixedpoint_contents(x, "assertions")

#' @rdname z3_fixedpoint
#' @export
z3_set_representation <- function(fp, relation, kinds) {
  cpp_fixedpoint_representation(fp, z3_decl(relation), as.character(kinds))
  invisible(fp)
}

#' @export
z3_set_params.z3_fixedpoint <- function(x, ...) {
  cpp_fixedpoint_set_params(x, params_list(...))
  invisible(x)
}

#' @export
z3_param_help.z3_fixedpoint <- function(x) cpp_fixedpoint_param_help(x)

#' @export
z3_statistics.z3_fixedpoint <- function(x) cpp_fixedpoint_statistics(x)

#' Finite domain sorts
#'
#' A sort with `size` elements, numbered from 0, used by the Datalog engine
#' for relations over finite domains. R whole numbers combined with such
#' expressions become elements, as in `r(1, 2)` or `z3_val(ctx, 3, S)`.
#' `z3_finite_domain_size()` reads the size back.
#'
#' @param ctx A [z3_context()].
#' @param name Sort name.
#' @param size Number of elements: a whole number up to 2^53, or a string
#'   of digits for larger sizes (below 2^64).
#' @param sort A finite domain sort.
#' @return A `z3_sort`; for `z3_finite_domain_size()` a number, or a string
#'   when the size exceeds 2^53.
#' @examples
#' ctx <- z3_context()
#' S <- z3_finite_domain_sort(ctx, "S", 10)
#' z3_finite_domain_size(S)
#' @export
z3_finite_domain_sort <- function(ctx, name, size) cpp_finite_domain_sort(ctx, name, whole_string(size, "size"))

#' @rdname z3_finite_domain_sort
#' @export
z3_finite_domain_size <- function(sort) cpp_finite_domain_size(sort)

#' Relation sorts
#'
#' The Datalog engine represents a whole relation as a term of a relation
#' sort ([z3_sort_kind()] `"relation"`), whose columns are the sorts of the
#' relation's arguments. Such terms reach R as arguments of the callbacks of
#' [z3_fixedpoint_external()]. `z3_relation_columns()` lists the column
#' sorts (`Z3_get_relation_arity`, `Z3_get_relation_column`); the arity is
#' the length of the list.
#'
#' @param sort A relation sort, for example `z3_sort(args[[1]])` inside an
#'   external relation callback.
#' @return A list of `z3_sort` objects, one per column.
#' @export
z3_relation_columns <- function(sort) cpp_relation_columns(sort)

#' Fixedpoint callbacks
#'
#' `z3_on_lemma(fp, lemma, predecessor, unfold)` registers R functions
#' called by the Spacer engine during [z3_query()]: `lemma(lemma, level)`
#' for every lemma Spacer learns (an implication from a predicate
#' application to a property; `level` is `Inf` for inductive invariants),
#' and `predecessor()` and `unfold()` when Spacer computes a predecessor or
#' unfolds the search to a new level. Giving `lemma` also turns on Spacer's
#' `spacer.p3.share_lemmas` and `spacer.p3.share_invariants`, without which
#' Spacer reports no lemmas.
#'
#' `z3_fixedpoint_external(fp, app, assign)` supplies user-defined
#' relations for the Datalog engine (Z3's external relations): `app(decl,
#' args)` is called to reduce applications of relational operators and
#' returns a term of the operator's range sort, or `NULL` to keep the
#' application; `assign(decl, args, outs)` is called for destructive
#' updates. It can be set once per engine.
#'
#' The callbacks follow the rules of [z3_propagator()] callbacks: errors
#' stop the query and are raised afterwards, and the engine cannot be used
#' from its own callbacks.
#'
#' @param fp A `z3_fixedpoint`.
#' @param lemma,predecessor,unfold,app,assign Callback functions, or `NULL`.
#' @return `fp`, invisibly.
#' @examples
#' ctx <- z3_context()
#' I <- z3_int_sort(ctx)
#' inv <- z3_function(ctx, "inv", I, z3_bool_sort(ctx))
#' x <- z3_int(ctx, "x")
#' fp <- z3_fixedpoint(ctx)
#' z3_register_relation(fp, inv)
#' z3_add_rule(fp, inv(0))
#' z3_add_rule(fp, z3_forall(x, z3_implies(inv(x), inv(x + 2))))
#' lemmas <- list()
#' z3_on_lemma(fp, lemma = function(lemma, level) lemmas[[length(lemmas) + 1]] <<- lemma)
#' z3_query(fp, z3_exists(x, inv(x) & x == 7))
#' length(lemmas) > 0
#' @export
z3_on_lemma <- function(fp, lemma = NULL, predecessor = NULL, unfold = NULL) {
  fns <- callback_list(lemma = lemma, predecessor = predecessor, unfold = unfold)
  if (!is.null(lemma)) {
    z3_set_params(fp, spacer.p3.share_lemmas = TRUE, spacer.p3.share_invariants = TRUE)
  }
  cpp_fixedpoint_on_lemma(fp, fns)
  invisible(fp)
}

#' @rdname z3_on_lemma
#' @export
z3_fixedpoint_external <- function(fp, app = NULL, assign = NULL) {
  cpp_fixedpoint_external(fp, callback_list(app = app, assign = assign))
  invisible(fp)
}

callback_list <- function(...) {
  fns <- Filter(Negate(is.null), list(...))
  if (!length(fns)) stop("give at least one callback function", call. = FALSE)
  for (nm in names(fns)) {
    if (!is.function(fns[[nm]])) stop(sprintf("%s must be a function", nm), call. = FALSE)
  }
  c(fns, list(.invoke = callback_invoke))
}
