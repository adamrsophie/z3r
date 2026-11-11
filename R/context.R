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
#' @return An object of class `z3_context`.
#' @seealso [z3_version()]
#' @examples
#' ctx <- z3_context()
#' ctx
#' @export
z3_context <- function() {
  cpp_mk_context()
}

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
#'   z3r warns when it is loaded if the two versions differ.
#' @examples
#' z3_version()
#' @export
z3_version <- function() {
  cpp_version()
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

.onLoad <- function(libname, pkgname) {
  v <- cpp_version()
  same <- function(a, b) identical(strsplit(a, ".", fixed = TRUE)[[1]][1:3],
                                   strsplit(b, ".", fixed = TRUE)[[1]][1:3])
  if (v$headers != "unknown" && !same(v$headers, v$runtime)) {
    warning(sprintf(paste(
      "z3r was compiled against Z3 %s but is running with Z3 %s;",
      "reinstall z3r so that headers and library match"
    ), v$headers, v$runtime), call. = FALSE)
  }
}
