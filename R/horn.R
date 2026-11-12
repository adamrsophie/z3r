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
#' predicate at a search level (`-1` is the invariant level), and
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
  cpp_fixedpoint_cover(fp, as.integer(level), z3_decl(relation), NULL, FALSE)
}

#' @rdname z3_fixedpoint
#' @export
z3_add_cover <- function(fp, level, relation, property) {
  cpp_fixedpoint_cover(fp, as.integer(level), z3_decl(relation), property, TRUE)
  invisible(fp)
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
#' A sort with `size` elements, used by the Datalog engine for relations
#' over finite domains. `z3_finite_domain_size()` reads the size back.
#'
#' @param ctx A [z3_context()].
#' @param name Sort name.
#' @param size Number of elements.
#' @param sort A finite domain sort.
#' @return A `z3_sort`, or a number.
#' @examples
#' ctx <- z3_context()
#' S <- z3_finite_domain_sort(ctx, "S", 10)
#' z3_finite_domain_size(S)
#' @export
z3_finite_domain_sort <- function(ctx, name, size) cpp_finite_domain_sort(ctx, name, size)

#' @rdname z3_finite_domain_sort
#' @export
z3_finite_domain_size <- function(sort) cpp_finite_domain_size(sort)
