#' Algebraic datatypes
#'
#' Declare new sorts by their constructors, as in functional languages:
#' enumerations, records, lists, trees.
#'
#' `z3_datatype(ctx, name, ...)` takes one argument per constructor. Each is
#' a named list of fields, mapping field names to sorts; an empty list makes
#' a constructor without fields. A field may refer to the datatype being
#' declared by giving its name as a string, which makes the datatype
#' recursive. `z3_datatypes()` declares several datatypes at once, so that
#' they can refer to each other (mutual recursion).
#'
#' The result is a `z3_datatype` with:
#'
#' * `$sort`: the new sort;
#' * one element per constructor: a function building values (or, for a
#'   constructor without fields, the value itself);
#' * `$is_<constructor>`: recognizers, true when a value was built by that
#'   constructor;
#' * one element per field: accessor functions.
#'
#' `z3_enum_sort()`, `z3_tuple_sort()` and `z3_list_sort()` are shortcuts
#' for the common shapes. `z3_update_field()` returns a copy of a value with
#' one field changed. `z3_datatype_of()` rebuilds the object from a datatype
#' sort, for example one read from SMT-LIB2.
#'
#' Polymorphic datatypes (Z3 4.15.4 and later) take `parameters`, a list of
#' type variables made with `z3_type_variable()` that fields may use as
#' sorts. `z3_datatype_instance()` instantiates one at concrete sorts.
#'
#' @param ctx A [z3_context()].
#' @param name Name of the datatype.
#' @param ... Constructors, each a named list of field sorts. For
#'   `z3_tuple_sort()`, the named field sorts.
#' @param parameters Optional list of type variables.
#' @param specs For `z3_datatypes()`: a named list whose elements are lists
#'   of constructors as in `...`.
#' @param values Names of the enumeration values.
#' @param element Element sort of the list.
#' @param x A value of a datatype.
#' @param field A field accessor (an element of the `z3_datatype`).
#' @param value New value of the field.
#' @param sort A datatype sort, or sorts for the instance.
#' @return A `z3_datatype` (a named list of datatypes for
#'   `z3_datatypes()`), a sort for `z3_type_variable()` and a `z3_expr` for
#'   `z3_update_field()`.
#' @examples
#' ctx <- z3_context()
#' I <- z3_int_sort(ctx)
#' Tree <- z3_datatype(ctx, "Tree",
#'   leaf = list(),
#'   node = list(left = "Tree", value = I, right = "Tree"))
#' t <- z3_const(ctx, "t", Tree$sort)
#' s <- z3_solver(ctx)
#' z3_add(s, Tree$is_node(t), Tree$value(t) == 7, Tree$is_node(Tree$left(t)))
#' z3_check(s)
#' z3_eval(z3_model(s), t)
#'
#' Color <- z3_enum_sort(ctx, "Color", c("red", "green", "blue"))
#' c1 <- z3_const(ctx, "c1", Color$sort)
#' s <- z3_solver(ctx)
#' z3_add(s, c1 != Color$red, c1 != Color$green)
#' z3_check(s)
#' z3_eval(z3_model(s), c1)
#' @export
z3_datatype <- function(ctx, name, ..., parameters = NULL) {
  constructors <- list(...)
  if (!is.null(parameters)) {
    spec <- datatype_spec(constructors, name)
    sort <- cpp_mk_polymorphic_datatype(ctx, name, as_list(parameters), spec)
    return(new_datatype(sort))
  }
  z3_datatypes(ctx, stats::setNames(list(constructors), name))[[1]]
}

#' @rdname z3_datatype
#' @export
z3_datatypes <- function(ctx, specs) {
  if (!is.list(specs) || is.null(names(specs)) || any(names(specs) == "") ||
      anyDuplicated(names(specs))) {
    stop("specs must be a list of datatypes with distinct names", call. = FALSE)
  }
  built <- lapply(specs, datatype_spec, names(specs))
  sorts <- cpp_mk_datatypes(ctx, names(specs), unname(built))
  stats::setNames(lapply(sorts, new_datatype), names(specs))
}

# Constructor list for C++: name, recognizer, field names, sorts (NULL for a
# reference to a datatype of the same declaration) and reference indices.
datatype_spec <- function(constructors, datatype_names) {
  if (!length(constructors) || is.null(names(constructors)) ||
      any(names(constructors) == "") || anyDuplicated(names(constructors))) {
    stop("a datatype needs constructors given as named arguments", call. = FALSE)
  }
  Map(function(cname, fields) {
    if (!is.list(fields)) stop(sprintf("constructor '%s' needs a list of fields", cname), call. = FALSE)
    if (length(fields) && (is.null(names(fields)) || any(names(fields) == ""))) {
      stop(sprintf("fields of constructor '%s' must be named", cname), call. = FALSE)
    }
    sorts <- vector("list", length(fields))
    refs <- integer(length(fields))
    for (i in seq_along(fields)) {
      f <- fields[[i]]
      if (inherits(f, "z3_datatype")) f <- f$sort
      if (is.character(f)) {
        idx <- match(f, datatype_names)
        if (is.na(idx)) {
          stop(sprintf("field '%s' refers to unknown datatype '%s'", names(fields)[i], f), call. = FALSE)
        }
        refs[i] <- idx - 1L
      } else if (inherits(f, "z3_sort")) {
        sorts[i] <- list(f)
      } else {
        stop(sprintf("field '%s' must be a sort or the name of a datatype", names(fields)[i]),
             call. = FALSE)
      }
    }
    list(name = cname, recognizer = paste0("is_", cname), fields = names(fields) %||% character(),
         sorts = sorts, refs = refs)
  }, names(constructors), constructors, USE.NAMES = FALSE)
}

decl_function <- function(decl) {
  f <- function(...) cpp_apply_decl(decl, flatten_args(...))
  structure(f, class = c("z3_function", "function"), decl = decl)
}

new_datatype <- function(sort) {
  parts <- cpp_datatype_parts(sort)
  out <- list(sort = sort)
  cons_names <- character()
  # Constructors, recognizers and accessors share one list with `sort`, so
  # their names must all differ.
  add <- function(name, value) {
    if (!is.null(out[[name]])) {
      stop(sprintf("the name '%s' is used twice in datatype %s; constructor, recognizer (is_<constructor>) and accessor names must all differ, and 'sort' is reserved",
                   name, format(sort)), call. = FALSE)
    }
    out[[name]] <<- value
  }
  for (p in parts) {
    cname <- cpp_decl_info(p$constructor)$name
    cons_names <- c(cons_names, cname)
    add(cname, if (length(p$accessors) == 0L) {
      cpp_apply_decl(p$constructor, list())
    } else {
      decl_function(p$constructor)
    })
    add(paste0("is_", cname), decl_function(p$recognizer))
    for (a in p$accessors) add(cpp_decl_info(a)$name, decl_function(a))
  }
  structure(out, class = "z3_datatype", constructors = cons_names)
}

#' @export
print.z3_datatype <- function(x, ...) {
  cat("<z3_datatype ", format(x$sort), ">\n", sep = "")
  parts <- cpp_datatype_parts(x$sort)
  for (p in parts) {
    info <- cpp_decl_info(p$constructor)
    fields <- vapply(p$accessors, function(a) {
      i <- cpp_decl_info(a)
      paste0(i$name, ": ", format(i$range))
    }, character(1))
    cat("  ", info$name, if (length(fields)) paste0("(", paste(fields, collapse = ", "), ")"), "\n",
        sep = "")
  }
  invisible(x)
}

#' @rdname z3_datatype
#' @export
z3_datatype_of <- function(sort) new_datatype(sort)

#' @rdname z3_datatype
#' @export
z3_enum_sort <- function(ctx, name, values) {
  if (!is.character(values) || !length(values) || anyNA(values) || anyDuplicated(values)) {
    stop("values must be distinct non-missing names", call. = FALSE)
  }
  cons <- stats::setNames(rep(list(list()), length(values)), values)
  do.call(z3_datatype, c(list(ctx, name), cons))
}

#' @rdname z3_datatype
#' @export
z3_tuple_sort <- function(ctx, name, ...) {
  fields <- list(...)
  do.call(z3_datatype, c(list(ctx, name), stats::setNames(list(fields), paste0("mk_", name))))
}

#' @rdname z3_datatype
#' @export
z3_list_sort <- function(ctx, name, element) {
  z3_datatype(ctx, name, nil = list(), cons = list(head = element, tail = name))
}

#' @rdname z3_datatype
#' @export
z3_update_field <- function(x, field, value) cpp_datatype_update(z3_decl(field), x, value)

#' @rdname z3_datatype
#' @export
z3_type_variable <- function(ctx, name) cpp_mk_type_variable(ctx, name)

#' @rdname z3_datatype
#' @export
z3_datatype_instance <- function(ctx, name, sort) {
  new_datatype(cpp_datatype_instance(ctx, name, as_list(sort)))
}

#' @rdname z3_datatype
#' @export
z3_is_recursive_datatype <- function(sort) {
  if (inherits(sort, "z3_datatype")) sort <- sort$sort
  cpp_is_recursive_datatype(sort)
}
