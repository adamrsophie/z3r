# Solve and return the R values of the named constants.
solve_values <- function(ctx, ..., vars) {
  s <- z3_solver(ctx)
  z3_add(s, ...)
  status <- z3_check(s)
  if (status != "sat") return(status)
  m <- z3_model(s)
  lapply(vars, function(v) z3_value(z3_eval(m, v, completion = TRUE)))
}

check_status <- function(ctx, ...) {
  s <- z3_solver(ctx)
  z3_add(s, ...)
  z3_check(s)
}

# Path of the z3 command-line tool, or "" when it is not installed.
z3_binary <- function() Sys.which("z3")[[1]]
