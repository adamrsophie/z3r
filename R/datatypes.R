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
#' for the common shapes. `z3_enum_sort()` and `z3_list_sort()` use Z3's
#' dedicated constructors; since Z3 4.12.0 an enumeration cannot reuse the
#' name of a datatype already declared in the context. `z3_tuple_sort()`
#' names its constructor `mk_<name>`. A datatype with a single constructor
#' that has the datatype's own name and no recursive fields, as in
#' `z3_datatype(ctx, "Point", Point = list(x = I, y = I))`, is declared as a
#' Z3 tuple sort. `z3_update_field()` returns a copy of a value with
#' one field changed. `z3_datatype_of()` rebuilds the object from a datatype
#' sort, for example one read from SMT-LIB2. `z3_tuple_fields()` reads a
#' tuple sort (a datatype with one constructor and no recursive fields):
#' its constructor and its field accessors, in order.
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
#' @param element Element sort of the list (or a `z3_datatype`).
#' @param x A value of a datatype.
#' @param field A field accessor (an element of the `z3_datatype`).
#' @param value New value of the field.
#' @param sort A datatype sort (or a `z3_datatype`), or sorts for the
#'   instance.
#' @return A `z3_datatype` (a named list of datatypes for
#'   `z3_datatypes()`), a sort for `z3_type_variable()` and a `z3_expr` for
#'   `z3_update_field()`. `z3_tuple_fields()` returns a list with
#'   `constructor`, a `z3_function` building tuples, and `fields`, a named
#'   list of accessor functions.
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
#'
#' Point <- z3_datatype(ctx, "Point", Point = list(x = I, y = I))
#' tf <- z3_tuple_fields(Point)
#' names(tf$fields)
#' z3_simplify(tf$fields$y(tf$constructor(1, 2)))
#' @export
z3_datatype <- function(ctx, name, ..., parameters = NULL) {
  spec <- datatype_spec(list(...), name)
  if (!is.null(parameters)) {
    sort <- cpp_mk_polymorphic_datatype(ctx, name, as_list(parameters), spec)
    return(new_datatype(sort))
  }
  # A single constructor named like the datatype, without recursive fields,
  # is what Z3 calls a tuple sort.
  con <- spec[[1L]]
  built <- if (length(spec) == 1L && identical(con$name, name) &&
               !any(vapply(con$sorts, is.null, NA))) {
    cpp_mk_tuple_sort(ctx, name, con$fields, con$sorts)
  } else {
    cpp_mk_datatype(ctx, name, spec)
  }
  new_datatype(built$sort, built$parts)
}

#' @rdname z3_datatype
#' @export
z3_datatypes <- function(ctx, specs) {
  if (!is.list(specs) || is.null(names(specs)) || any(names(specs) == "") ||
      anyDuplicated(names(specs))) {
    stop("specs must be a list of datatypes with distinct names", call. = FALSE)
  }
  built <- Map(function(cons, self) datatype_spec(cons, names(specs), self), specs, names(specs))
  sorts <- cpp_mk_datatypes(ctx, names(specs), unname(built))
  stats::setNames(lapply(sorts, new_datatype), names(specs))
}

# Constructor list for C++: name, recognizer, field names, sorts (NULL for a
# reference to a datatype of the same declaration) and reference indices.
datatype_spec <- function(constructors, datatype_names, self = datatype_names) {
  if (!length(constructors) || is.null(names(constructors)) ||
      any(names(constructors) == "") || anyDuplicated(names(constructors))) {
    stop("a datatype needs constructors given as named arguments", call. = FALSE)
  }
  check_part_names(self, names(constructors),
                   unlist(lapply(constructors, function(f) if (is.list(f)) names(f))))
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
    list(name = enc2utf8(cname), recognizer = enc2utf8(paste0("is_", cname)),
         fields = enc2utf8(names(fields) %||% character()), sorts = sorts, refs = refs)
  }, names(constructors), constructors, USE.NAMES = FALSE)
}

# new_datatype() keeps the constructors, recognizers (is_<constructor>) and
# accessors of a datatype by name in one list with `sort`, so their names
# must all differ. Checked before Z3 declares the sort: Z3 keeps a declared
# name, which would block a corrected second attempt.
check_part_names <- function(datatype, constructors, accessors = NULL) {
  keys <- c("sort", constructors, paste0("is_", constructors), accessors)
  dup <- keys[duplicated(enc2utf8(keys))]
  if (length(dup)) dup_part_name(dup[[1L]], datatype)
}

dup_part_name <- function(name, datatype) {
  stop(sprintf("the name '%s' is used twice in datatype %s; constructor, recognizer (is_<constructor>) and accessor names must all differ, and 'sort' is reserved",
               name, datatype), call. = FALSE)
}

decl_function <- function(decl) {
  f <- function(...) cpp_apply_decl(decl, flatten_args(...))
  structure(f, class = c("z3_function", "function"), decl = decl)
}

# `parts` (constructor, recognizer and accessors of each constructor) may
# come from the call that declared the sort.
new_datatype <- function(sort, parts = cpp_datatype_parts(sort)) {
  out <- list(sort = sort)
  cons_names <- character()
  # Constructors, recognizers and accessors share one list with `sort`, so
  # their names must all differ.
  add <- function(name, value) {
    if (!is.null(out[[name]])) dup_part_name(name, format(sort))
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
  if (!is.character(values) || !length(values) || anyNA(values) || any(values == "") ||
      anyDuplicated(values)) {
    stop("values must be distinct non-missing names", call. = FALSE)
  }
  check_part_names(name, values)
  built <- cpp_mk_enumeration_sort(ctx, name, enc2utf8(values))
  new_datatype(built$sort, built$parts)
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
  if (inherits(element, "z3_datatype")) element <- element$sort
  built <- cpp_mk_list_sort(ctx, name, element)
  new_datatype(built$sort, built$parts)
}

#' @rdname z3_datatype
#' @export
z3_tuple_fields <- function(sort) {
  if (inherits(sort, "z3_datatype")) sort <- sort$sort
  parts <- cpp_tuple_parts(sort)
  fields <- lapply(parts$accessors, decl_function)
  names(fields) <- vapply(parts$accessors, function(a) cpp_decl_info(a)$name, "")
  list(constructor = decl_function(parts$constructor), fields = fields)
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
