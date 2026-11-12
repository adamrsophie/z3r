#' Inspect the structure of an expression
#'
#' Z3 expressions are trees. `z3_kind()` classifies a node:
#'
#' * `"numeral"`: a number (Int, Real, bit-vector or floating point value);
#' * `"app"`: an application of a function declaration to arguments,
#'   including constants (no arguments) and built-in operators;
#' * `"var"`: a bound variable inside a quantifier body, identified by its de
#'   Bruijn index;
#' * `"quantifier"`: `forall`, `exists` or `lambda`.
#'
#' `z3_decl()` returns the function declaration at the head of an
#' application (numerals included), `z3_args()` its arguments, and
#' `z3_decl_info()` describes a declaration: its name, kind (such as
#' `"add"`, `"le"`, `"and"`, `"bvadd"` or `"uninterpreted"`), domain, range
#' and indexed parameters (for example the bounds of an `extract`). Kinds
#' that only some Z3 versions define are reported as `"other"`.
#'
#' `z3_quantifier_info()` returns the type, bound variable names and sorts,
#' body (with bound variables as `"var"` nodes), weight and patterns of a
#' quantifier. `z3_var_index()` gives a bound variable's de Bruijn index;
#' `z3_bound_var()` creates one, for building bodies for
#' [z3_substitute_vars()] and [z3_substitute_funs()].
#'
#' @param x A `z3_expr`.
#' @param decl A `z3_func_decl` or `z3_function`.
#' @param ctx A [z3_context()] (unused when `sort` is given).
#' @param index De Bruijn index of a bound variable.
#' @param sort Sort of the bound variable.
#' @return See Details.
#' @examples
#' ctx <- z3_context()
#' x <- z3_int(ctx, "x")
#' e <- x + 2 * x^2
#' z3_kind(e)
#' z3_decl_info(z3_decl(e))$kind
#' z3_args(e)
#' z3_kind(z3_int_val(ctx, 3))
#' q <- z3_forall(x, x * 0 == 0)
#' z3_quantifier_info(q)$names
#' @name z3_inspect
NULL

#' @rdname z3_inspect
#' @export
z3_kind <- function(x) cpp_ast_kind(x)

#' @rdname z3_inspect
#' @export
z3_decl <- function(x) {
  if (inherits(x, "z3_function")) return(attr(x, "decl"))
  if (inherits(x, "z3_func_decl")) return(x)
  cpp_app_parts(x)$decl
}

#' @rdname z3_inspect
#' @export
z3_args <- function(x) cpp_app_parts(x)$args

#' @rdname z3_inspect
#' @export
z3_num_args <- function(x) length(cpp_app_parts(x)$args)

#' @rdname z3_inspect
#' @export
z3_decl_info <- function(decl) {
  decl <- z3_decl(decl)
  c(cpp_decl_info(decl)[c("name", "domain", "range")], cpp_decl_details(decl))
}

#' @rdname z3_inspect
#' @export
z3_is_const <- function(x) is_z3_expr(x) && cpp_is_const(x)

#' @rdname z3_inspect
#' @export
z3_is_numeral <- function(x) is_z3_expr(x) && cpp_ast_kind(x) == "numeral"

#' @rdname z3_inspect
#' @export
z3_is_app <- function(x) is_z3_expr(x) && cpp_ast_kind(x) %in% c("app", "numeral")

#' @rdname z3_inspect
#' @export
z3_is_quantifier <- function(x) is_z3_expr(x) && cpp_ast_kind(x) == "quantifier"

#' @rdname z3_inspect
#' @export
z3_quantifier_info <- function(x) cpp_quantifier_parts(x)

#' @rdname z3_inspect
#' @export
z3_var_index <- function(x) cpp_var_index(x)

#' @rdname z3_inspect
#' @export
z3_bound_var <- function(ctx, index, sort) cpp_mk_bound(as_count(index, "index"), sort)

#' @rdname z3_inspect
#' @param ... Arguments for `z3_apply()`: expressions, R values, or lists
#'   of them.
#' @export
z3_apply <- function(decl, ...) cpp_apply_decl(z3_decl(decl), flatten_args(...))

#' Substitution
#'
#' * `z3_substitute(x, from, to)` replaces every occurrence of the
#'   expressions in `from` by the corresponding expressions in `to`, all at
#'   once.
#' * `z3_substitute_vars(x, to)` replaces bound variable `i` (a de Bruijn
#'   index, see [z3_bound_var()]) by `to[[i + 1]]`.
#' * `z3_substitute_funs(x, from, to)` replaces applications of the
#'   functions in `from` by the bodies in `to`, in which bound variable `i`
#'   stands for argument `i + 1`. Needs Z3 4.9.0 or later.
#'
#' @param x A `z3_expr`.
#' @param from Expressions (or functions for `z3_substitute_funs()`) to
#'   replace: a single object or a list.
#' @param to Replacements: a single object or a list of the same length.
#' @return A `z3_expr`.
#' @examples
#' ctx <- z3_context()
#' x <- z3_int(ctx, "x")
#' y <- z3_int(ctx, "y")
#' z3_substitute(x + y, x, 3)
#' z3_simplify(z3_substitute(x * y, list(x, y), list(2, 5)))
#' f <- z3_function(ctx, "f", z3_int_sort(ctx), z3_int_sort(ctx))
#' v0 <- z3_bound_var(ctx, 0, z3_int_sort(ctx))
#' z3_substitute_funs(f(x) + f(1), f, v0 * v0)
#' @export
z3_substitute <- function(x, from, to) {
  cpp_substitute(x, as_list(from), as_list(to))
}

#' @rdname z3_substitute
#' @export
z3_substitute_vars <- function(x, to) cpp_substitute_vars(x, as_list(to))

#' @rdname z3_substitute
#' @export
z3_substitute_funs <- function(x, from, to) {
  from <- lapply(as_list(from), z3_decl)
  cpp_substitute_funs(x, from, as_list(to))
}

as_list <- function(x) {
  if (is.list(x) && !inherits(x, "z3_function")) x else list(x)
}

#' Simplify with parameters
#'
#' `z3_simplify()` with named parameters calls Z3's configurable simplifier;
#' `z3_simplify_help()` lists its parameters (such as `som` for sum of
#' monomials, or `arith_lhs`).
#'
#' @param ctx A [z3_context()].
#' @return A data frame of parameters.
#' @examples
#' ctx <- z3_context()
#' x <- z3_int(ctx, "x")
#' z3_simplify((x + 1)^2, som = TRUE)
#' head(z3_simplify_help(ctx))
#' @export
z3_simplify_help <- function(ctx) cpp_simplify_help(ctx)

#' Translate objects between contexts
#'
#' Copies an expression, solver, optimizer or model into another context.
#' This is the only way to combine objects from different contexts.
#' Optimizers need Z3 4.15.5 or later.
#'
#' @param x A `z3_expr`, `z3_solver`, `z3_optimize`, `z3_model` or `z3_goal`.
#' @param ctx The target [z3_context()].
#' @return An object of the same class in `ctx`.
#' @examples
#' a <- z3_context()
#' b <- z3_context()
#' x <- z3_int(a, "x")
#' y <- z3_translate(x + 1, b)
#' z3_same_context(y, b)
#' @export
z3_translate <- function(x, ctx) {
  if (is_z3_expr(x)) return(cpp_translate(x, ctx))
  if (inherits(x, "z3_solver")) return(cpp_solver_translate(x, ctx))
  if (inherits(x, "z3_optimize")) return(cpp_optimize_translate(x, ctx))
  if (inherits(x, "z3_model")) return(cpp_model_translate(x, ctx))
  if (inherits(x, "z3_goal")) return(cpp_goal_translate(x, ctx))
  stop("x must be an expression, solver, optimizer or model", call. = FALSE)
}

#' Expression identity and properties
#'
#' `z3_ast_id()` is a number unique to the term within its context (equal
#' terms share it); `z3_ast_hash()` is Z3's hash. `z3_is_well_sorted()`
#' checks sorts recursively. `z3_is_ground()` (no bound variables) and
#' `z3_depth()` (height of the tree) need Z3 4.14.0 or later.
#'
#' @param x A `z3_expr` (for `z3_ast_id()` also a sort or function).
#' @return A number or `TRUE`/`FALSE`.
#' @examples
#' ctx <- z3_context()
#' x <- z3_int(ctx, "x")
#' z3_ast_id(x + 1) == z3_ast_id(x + 1)
#' z3_is_well_sorted(x + 1)
#' @export
z3_ast_id <- function(x) {
  if (inherits(x, "z3_function")) x <- z3_decl(x)
  if (inherits(x, c("z3_sort", "z3_func_decl"))) return(cpp_decl_identity(x, NULL))
  cpp_ast_id(x)$id
}

#' @rdname z3_ast_id
#' @export
z3_ast_hash <- function(x) cpp_ast_id(x)$hash

#' @rdname z3_ast_id
#' @export
z3_is_well_sorted <- function(x) cpp_is_well_sorted(x)

#' @rdname z3_ast_id
#' @export
z3_is_ground <- function(x) cpp_is_ground(x)

#' @rdname z3_ast_id
#' @export
z3_depth <- function(x) cpp_depth(x)

#' Printing mode
#'
#' Sets how expressions of a context are printed: `"smtlib2"` (the default,
#' SMT-LIB2 compliant), `"low_level"` (Z3's internal form with sharing), or
#' `"smtlib_full"`.
#'
#' @param ctx A [z3_context()].
#' @param mode One of the modes above.
#' @return `ctx`, invisibly.
#' @examples
#' ctx <- z3_context()
#' z3_set_print_mode(ctx, "low_level")
#' z3_int(ctx, "x") + 1
#' @export
z3_set_print_mode <- function(ctx, mode = c("smtlib2", "low_level", "smtlib_full")) {
  cpp_set_print_mode(ctx, match.arg(mode))
  invisible(ctx)
}

#' Global Z3 parameters and logging
#'
#' Global parameters apply to the whole R process: every context created
#' afterwards, and some settings of existing contexts. Prefer per-solver
#' parameters ([z3_set_params()]) or context configuration
#' ([z3_context()]) when they exist. Values are given as R scalars and
#' passed to Z3 as strings.
#'
#' * `z3_global_param_set(name, value)` sets one parameter, for example
#'   `"smt.random_seed"` or `"verbose"`.
#' * `z3_global_param_get(name)` returns the current value as a string, or
#'   `NA` when the name is unknown.
#' * `z3_global_param_reset()` restores all defaults.
#' * `z3_global_param_help()` lists the global parameters (Z3 4.9.0 or
#'   later).
#' * `z3_log_open(file)`/`z3_log_close()` record every Z3 API call into a
#'   file for bug reports. `z3_warnings(enabled)` turns Z3's warning
#'   messages on or off.
#'
#' @param name Parameter name.
#' @param value A single logical, number or string.
#' @param ctx A [z3_context()] used to read the descriptions.
#' @param file Path of the log file.
#' @param enabled `TRUE` or `FALSE`.
#' @return `z3_global_param_get()` returns a string; the others return
#'   their input invisibly.
#' @examples
#' old <- z3_global_param_get("model.completion")
#' z3_global_param_set("model.completion", TRUE)
#' z3_global_param_get("model.completion")
#' z3_global_param_set("model.completion", old)
#' @export
z3_global_param_set <- function(name, value) {
  if (!is.character(name) || length(name) != 1L || is.na(name)) {
    stop("name must be a single string", call. = FALSE)
  }
  if (is.na(cpp_global_param_get(name))) {
    stop(sprintf("unknown global parameter '%s'", name), call. = FALSE)
  }
  cpp_global_param_set(name, param_string(value))
  invisible(value)
}

#' @rdname z3_global_param_set
#' @export
z3_global_param_get <- function(name) cpp_global_param_get(name)

#' @rdname z3_global_param_set
#' @export
z3_global_param_reset <- function() invisible(cpp_global_param_reset())

#' @rdname z3_global_param_set
#' @export
z3_global_param_help <- function(ctx) cpp_global_param_help(ctx)

#' @rdname z3_global_param_set
#' @export
z3_log_open <- function(file) {
  if (!cpp_open_log(path.expand(file))) stop("could not open the log file", call. = FALSE)
  invisible(file)
}

#' @rdname z3_global_param_set
#' @export
z3_log_close <- function() invisible(cpp_close_log())

#' @rdname z3_global_param_set
#' @export
z3_warnings <- function(enabled) {
  cpp_toggle_warnings(isTRUE(enabled))
  invisible(enabled)
}

# Scalar to the string form Z3's configuration parser expects.
param_string <- function(value) {
  if (length(value) != 1L || is.na(value)) {
    stop("value must be a single non-missing value", call. = FALSE)
  }
  if (is.logical(value)) return(if (value) "true" else "false")
  if (is.numeric(value)) return(format(value, scientific = FALSE, trim = TRUE, digits = 17))
  if (is.character(value)) return(value)
  stop("value must be logical, numeric or character", call. = FALSE)
}

#' Rebuild an application with new arguments
#'
#' `z3_update_args(x, args)` returns the application `x` with its arguments
#' replaced by `args` (same number and sorts).
#'
#' @param x An application.
#' @param args A list of new arguments.
#' @return A `z3_expr`.
#' @examples
#' ctx <- z3_context()
#' x <- z3_int(ctx, "x")
#' z3_update_args(x + 1, list(x * 2, 5))
#' @export
z3_update_args <- function(x, args) cpp_update_term(x, as_list(args))

#' Implied equalities
#'
#' Groups terms into classes that are equal in every model of the solver's
#' assertions. Terms with the same class number are provably equal.
#'
#' @param solver A `z3_solver`.
#' @param terms A list of expressions.
#' @return A list with the check `status` and an integer `class` per term.
#' @examples
#' ctx <- z3_context()
#' x <- z3_int(ctx, "x")
#' y <- z3_int(ctx, "y")
#' z <- z3_int(ctx, "z")
#' s <- z3_solver(ctx)
#' z3_add(s, x == y + 1, y + 1 == z)
#' z3_implied_equalities(s, list(x, y, z))
#' @export
z3_implied_equalities <- function(solver, terms) cpp_implied_equalities(solver, as_list(terms))

#' SMT-LIB2 parsers with shared declarations
#'
#' A parser (Z3 4.9.0 and later) keeps the sorts and functions declared in
#' earlier fragments, and accepts sorts and functions declared in R, so a
#' problem can be read in pieces. `z3_parser_add()` makes a sort or function
#' known to it; `z3_parse()` returns the assertions of a fragment.
#'
#' @param ctx A [z3_context()].
#' @param parser A `z3_parser`.
#' @param x A `z3_sort`, `z3_function` or `z3_func_decl`.
#' @param text SMT-LIB2 text.
#' @return `z3_parser()` returns a parser; `z3_parse()` a list of
#'   expressions; `z3_parser_add()` the parser, invisibly.
#' @examples
#' ctx <- z3_context()
#' if (z3_version_at_least("4.9.0")) {
#'   p <- z3_parser(ctx)
#'   f <- z3_function(ctx, "f", z3_int_sort(ctx), z3_int_sort(ctx))
#'   z3_parser_add(p, f)
#'   z3_parse(p, "(declare-const a Int) (assert (> (f a) 3))")
#'   z3_parse(p, "(assert (< a 0))")
#' }
#' @export
z3_parser <- function(ctx) cpp_mk_parser(ctx)

#' @rdname z3_parser
#' @export
z3_parser_add <- function(parser, x) {
  if (inherits(x, "z3_function")) x <- z3_decl(x)
  cpp_parser_add(parser, x)
  invisible(parser)
}

#' @rdname z3_parser
#' @export
z3_parse <- function(parser, text) cpp_parser_parse(parser, paste(text, collapse = "\n"))

#' @export
print.z3_parser <- function(x, ...) {
  cat("<z3_parser>\n")
  invisible(x)
}

#' Memory, logging and context parameters
#'
#' `z3_memory_usage()` estimates the memory, in bytes, that Z3 has allocated
#' in this R process. `z3_log_append()` writes a comment into the API log
#' opened with [z3_log_open()]. `z3_update_param()` changes a configuration
#' parameter of an existing context.
#'
#' @param text Text for the log.
#' @param ctx A [z3_context()].
#' @param name,value Parameter name and value.
#' @return A number, or the input invisibly.
#' @examples
#' z3_memory_usage()
#' ctx <- z3_context()
#' z3_update_param(ctx, "timeout", 5000)
#' @export
z3_memory_usage <- function() cpp_memory_usage()

#' @rdname z3_memory_usage
#' @export
z3_log_append <- function(text) invisible(cpp_append_log(as.character(text)))

#' @rdname z3_memory_usage
#' @export
z3_update_param <- function(ctx, name, value) {
  if (!name %in% context_params) {
    stop(sprintf("unknown context parameter '%s'", name), call. = FALSE)
  }
  cpp_update_param(ctx, name, param_string(value))
  invisible(ctx)
}
