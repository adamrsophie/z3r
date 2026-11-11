# Internal helpers.

# A single non-negative (or positive) whole number as an R integer.
as_count <- function(x, what, positive = FALSE) {
  lower <- if (positive) 1 else 0
  ok <- is.numeric(x) && length(x) == 1L && !is.na(x) && x == floor(x) &&
    x >= lower && x <= .Machine$integer.max
  if (!ok) {
    stop(sprintf("%s must be a single %s whole number", what,
                 if (positive) "positive" else "non-negative"), call. = FALSE)
  }
  as.integer(x)
}

# Splices plain lists among the arguments, so that both z3_add(s, a, b) and
# z3_add(s, list(a, b)) work. z3r objects are external pointers, not lists.
flatten_args <- function(...) {
  args <- list(...)
  out <- list()
  for (a in args) {
    if (is.list(a)) out <- c(out, a) else out[[length(out) + 1L]] <- a
  }
  out
}

is_z3_expr <- function(x) inherits(x, "z3_expr")
