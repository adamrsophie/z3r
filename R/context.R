#' Create a Z3 context
#'
#' A context owns every Z3 object (sorts, expressions, solvers, models)
#' created from it. Objects from different contexts cannot be combined.
#'
#' The context stays alive as long as any object created from it is alive,
#' even after the context variable itself has been removed, so objects can
#' be garbage collected in any order. Z3 contexts are not thread safe; use
#' each context from a single R process.
#'
#' Z3 objects are external pointers. They cannot be saved with [saveRDS()]
#' or [save()] and restored in another session; a restored object reports
#' that it is no longer valid. Use [z3_to_smtlib()] to store constraints.
#'
#' Configuration parameters fix properties of the context for its whole
#' life. The most useful are `proof = TRUE` (needed for [z3_proof()]),
#' `unsat_core`, `timeout` (milliseconds, for every solver of the context),
#' `auto_config`, `well_sorted_check` and `model_validate`. Values are R
#' scalars of the parameter's type (`TRUE`/`FALSE`, a whole number, or a
#' string); unknown names and values of the wrong type are errors.
#'
#' `concurrent_dec_ref = TRUE` makes Z3 queue reference count decrements
#' and apply them the next time the context creates an object or takes a
#' reference to one, so that other threads could release objects safely
#' (`Z3_enable_concurrent_dec_ref`, Z3 4.10.0 or later). z3r releases
#' objects only from R's main thread, so this only delays when Z3 frees
#' memory.
#'
#' @param ... Named configuration parameters.
#' @param concurrent_dec_ref `TRUE` to enable Z3's concurrent reference
#'   counting for this context (see Details); must be given by name.
#' @return An object of class `z3_context`.
#' @seealso [z3_version()]
#' @examples
#' ctx <- z3_context()
#' ctx
#' pctx <- z3_context(proof = TRUE)
#' @export
z3_context <- function(..., concurrent_dec_ref = FALSE) {
  if (!isTRUE(concurrent_dec_ref) && !isFALSE(concurrent_dec_ref)) {
    stop("concurrent_dec_ref must be TRUE or FALSE", call. = FALSE)
  }
  config <- list(...)
  if (length(config) && (is.null(names(config)) || any(names(config) == ""))) {
    stop("configuration parameters must be named", call. = FALSE)
  }
  unknown <- setdiff(names(config), context_params)
  if (length(unknown)) {
    stop(sprintf("unknown context parameter%s: %s; valid names are %s",
                 if (length(unknown) > 1L) "s" else "", paste(unknown, collapse = ", "),
                 paste(context_params, collapse = ", ")), call. = FALSE)
  }
  for (nm in names(config)) check_context_param(nm, config[[nm]])
  values <- vapply(config, param_string, character(1))
  cpp_mk_context(names(config) %||% character(), unname(values), concurrent_dec_ref)
}

# Configuration parameters Z3 accepts for a new context (Z3_set_param_value),
# by type. Z3 itself only prints a warning for an invalid value.
context_param_types <- c(
  auto_config = "bool", ctrl_c = "bool", debug_ref_count = "bool", dot_proof_file = "string",
  dump_models = "bool", encoding = "encoding", model = "bool", model_validate = "bool",
  proof = "bool", rlimit = "uint", smtlib2_compliant = "bool", stats = "bool", timeout = "uint",
  trace = "bool", trace_file_name = "string", type_check = "bool", unsat_core = "bool",
  well_sorted_check = "bool"
)
context_params <- names(context_param_types)

check_context_param <- function(name, value) {
  type <- context_param_types[[name]]
  ok <- length(value) == 1L && !is.na(value) && switch(type,
    bool = is.logical(value),
    uint = is.numeric(value) && value >= 0 && value == floor(value) && value <= 4294967295,
    string = is.character(value),
    encoding = is.character(value) && value %in% c("unicode", "bmp", "ascii")
  )
  if (!ok) {
    want <- switch(type,
      bool = "TRUE or FALSE",
      uint = "a whole number between 0 and 4294967295",
      string = "a single string",
      encoding = "one of \"unicode\", \"bmp\" or \"ascii\""
    )
    stop(sprintf("context parameter %s must be %s", name, want), call. = FALSE)
  }
}

`%||%` <- function(a, b) if (is.null(a)) b else a

#' @export
print.z3_context <- function(x, ...) {
  if (!cpp_is_valid(x)) {
    cat("<z3_context: invalid (restored from a saved session)>\n")
  } else {
    cat("<z3_context>\n")
  }
  invisible(x)
}

#' Z3 library version
#'
#' Reports the version of the Z3 shared library z3r is running against.
#'
#' @return A list with elements `runtime` (the version of the loaded Z3
#'   library, major.minor.build.revision), `headers` (the version of the Z3
#'   headers z3r was compiled with) and `full` (Z3's full version string).
#'   z3r refuses to load if the two versions differ.
#' @examples
#' z3_version()
#' z3_version_at_least("4.12.2")
#' @export
z3_version <- function() {
  cpp_version()
}

#' @rdname z3_version
#' @param version A version string such as `"4.12.2"`.
#' @return For `z3_version_at_least()`, `TRUE` when the Z3 headers z3r was
#'   compiled with are at least `version`, so that features needing that
#'   version are available.
#' @export
z3_version_at_least <- function(version) {
  h <- cpp_version()$headers
  if (identical(h, "unknown")) h <- "4.8.13"
  numeric_version(h) >= numeric_version(version)
}

#' Context of a Z3 object
#'
#' @param x Any z3r object: an expression, sort, solver, model, optimizer,
#'   goal or tactic.
#' @return The `z3_context` that owns `x`.
#' @examples
#' ctx <- z3_context()
#' x <- z3_int(ctx, "x")
#' z3_same_context(z3_context_of(x), ctx)
#' @export
z3_context_of <- function(x) {
  cpp_context_of(x)
}

#' @rdname z3_context_of
#' @param y Another z3r object.
#' @return For `z3_same_context()`, `TRUE` if both objects belong to the
#'   same Z3 context.
#' @export
z3_same_context <- function(x, y) {
  cpp_same_context(x, y)
}

#' Z3 trace messages
#'
#' `z3_trace(tag)` turns on Z3's internal trace messages for a tag, such as
#' `"arith"` or `"spacer"`, and `z3_trace(tag, FALSE)` turns them off again
#' (`Z3_enable_trace`, `Z3_disable_trace`). Only debug builds of Z3 trace;
#' they write the messages to the file `.z3-trace` in the working
#' directory. Release builds, which include the usual binary distributions
#' of Z3, ignore both calls.
#'
#' @param tag Trace tags, a character vector.
#' @param enable `TRUE` to turn the tags on, `FALSE` to turn them off.
#' @return `tag`, invisibly.
#' @examples
#' z3_trace("arith")
#' z3_trace("arith", enable = FALSE)
#' @export
z3_trace <- function(tag, enable = TRUE) {
  if (!is.character(tag)) stop("tag must be a character vector", call. = FALSE)
  if (!isTRUE(enable) && !isFALSE(enable)) stop("enable must be TRUE or FALSE", call. = FALSE)
  for (t in tag) cpp_trace(t, enable)
  invisible(tag)
}

.onLoad <- function(libname, pkgname) {
  v <- cpp_version()
  if (v$headers == "unknown") return(invisible())
  three <- function(x) paste(strsplit(x, ".", fixed = TRUE)[[1]][1:3], collapse = ".")
  # z3r is compiled for one Z3 release: an older library may lack functions
  # it calls (crashing R at the first call), and a newer one may number
  # operators and sorts differently, which silently changes inspection
  # results. Refuse to load with any other release.
  if (three(v$headers) != three(v$runtime)) {
    stop(sprintf(paste(
      "z3r was compiled against Z3 %s but the Z3 library loaded is %s;",
      "reinstall z3r against the library you use"
    ), three(v$headers), three(v$runtime)), call. = FALSE)
  }
}
