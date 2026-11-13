#' Convert between symcompR and Z3 expressions
#'
#' `z3_from_symbolic()` translates a symbolic expression from the
#' [symcompR](https://github.com/adamrsophie/symcompR) package into a Z3
#' expression, and `z3_to_symbolic()` translates back. symcompR is optional;
#' these functions need it installed.
#'
#' Only translations that preserve the mathematical meaning exactly are
#' made; anything else is an error naming the unsupported part.
#'
#' **Supported.** Integers and rationals (exact); symbols; `+`, `-`, `*`,
#' `/`, powers with integer exponents (and rational exponents, which become
#' Z3's real power); `Eq`, `Ne`, `Lt`, `Le`, `Gt`, `Ge`; `And`, `Or`,
#' `Not`, `Implies`, `Xor`, `Equivalent`, `ITE`; `Abs`, `floor`,
#' `ceiling`, `Mod` (with Python's sign convention, which is also
#' symcompR's: the result takes the sign of the divisor), `Max`, `Min`, and
#' `Piecewise` with a final `TRUE` branch; `true` and `false`.
#'
#' **Not supported.** Floats (they are inexact), constants such as `pi`,
#' `E` and `oo`, complex numbers, transcendental functions, derivatives,
#' integrals, sums and other unevaluated operators.
#'
#' **Symbols and sorts.** A symbol becomes a Z3 constant of the same name.
#' Its sort comes from `sorts` (a named list of `z3_sort`s) when given;
#' otherwise from its assumptions: `integer = TRUE` gives Int and
#' `real = TRUE` gives Real. A symbol without assumptions is Bool where a
#' Boolean is expected (as an argument of `And`, `Or`, ...), and an error
#' elsewhere, because SymPy-style symbols without assumptions are complex.
#'
#' In the other direction, Int constants become symbols with `integer =
#' TRUE`, Real constants symbols with `real = TRUE`, and Bool constants
#' plain symbols. Z3's `div` and `mod` are translated with `floor` and
#' `Abs` so that the result keeps Z3's (Euclidean) meaning.
#'
#' @param ctx A [z3_context()].
#' @param expr A symcompR expression (an object of class `sym`).
#' @param sorts Optional named list of sorts for symbols.
#' @param x A `z3_expr`.
#' @return A `z3_expr`, or for `z3_to_symbolic()` a symcompR expression.
#' @examples
#' if (requireNamespace("symcompR", quietly = TRUE)) {
#'   x <- symcompR::Symbol("x", integer = TRUE)
#'   y <- symcompR::Symbol("y", integer = TRUE)
#'   ctx <- z3_context()
#'   e <- z3_from_symbolic(ctx, symcompR::Eq(x^2 + y^2, 25))
#'   s <- z3_solver(ctx)
#'   z3_add(s, e, z3_int(ctx, "x") > 3, z3_int(ctx, "y") > 0)
#'   z3_check(s)
#'   z3_model(s)
#'   z3_to_symbolic(z3_int(ctx, "x") * 2 + 1)
#' }
#' @export
z3_from_symbolic <- function(ctx, expr, sorts = list()) {
  need_symcompr()
  if (!inherits(expr, "sym")) expr <- symcompR::as_sym(expr)
  from_sym(ctx, expr, sorts, expected = NULL)
}

need_symcompr <- function() {
  if (!requireNamespace("symcompR", quietly = TRUE)) {
    stop("the symcompR package is needed for this function", call. = FALSE)
  }
}

sym_unsupported <- function(e) {
  stop(sprintf("the symcompR expression %s (%s) has no exact Z3 translation",
               format(e), symcompR::sym_type(e)), call. = FALSE)
}

from_sym <- function(ctx, e, sorts, expected) {
  type <- symcompR::sym_type(e)
  args <- symcompR::sym_args(e)
  rec <- function(a, exp = NULL) from_sym(ctx, a, sorts, exp)
  rec_all <- function(exp = NULL) lapply(args, rec, exp)
  switch(type,
    Integer = z3_int_val(ctx, as.character(e$value)),
    Rational = z3_real_val(ctx, as.character(e$value)),
    Float = stop("symcompR Floats are inexact and are not translated; use Rational values",
                 call. = FALSE),
    BooleanAtom = z3_bool_val(ctx, identical(symcompR::sym_name(e), "true")),
    Symbol = sym_symbol(ctx, e, sorts, expected),
    Add = z3_sum(rec_all("arith")),
    Mul = z3_prod(rec_all("arith")),
    Pow = sym_pow(ctx, args, rec),
    Equality = rec(args[[1]]) == rec(args[[2]]),
    Unequality = rec(args[[1]]) != rec(args[[2]]),
    StrictLessThan = rec(args[[1]], "arith") < rec(args[[2]], "arith"),
    LessThan = rec(args[[1]], "arith") <= rec(args[[2]], "arith"),
    StrictGreaterThan = rec(args[[1]], "arith") > rec(args[[2]], "arith"),
    GreaterThan = rec(args[[1]], "arith") >= rec(args[[2]], "arith"),
    And = z3_and(rec_all("bool")),
    Or = z3_or(rec_all("bool")),
    Not = z3_not(rec(args[[1]], "bool")),
    Implies = z3_implies(rec(args[[1]], "bool"), rec(args[[2]], "bool")),
    Xor = Reduce(z3_xor, rec_all("bool")),
    Equivalent = z3_and(lapply(rec_all("bool")[-1], function(b) z3_iff(rec(args[[1]], "bool"), b))),
    ITE = z3_ite(rec(args[[1]], "bool"), rec(args[[2]], expected), rec(args[[3]], expected)),
    Piecewise = sym_piecewise(ctx, args, rec, expected),
    Function = sym_function(ctx, e, args, rec),
    sym_unsupported(e)
  )
}

sym_symbol <- function(ctx, e, sorts, expected) {
  name <- symcompR::sym_name(e)
  if (!is.null(sorts[[name]])) return(z3_const(ctx, name, sorts[[name]]))
  if (isTRUE(symcompR::is_integer(e))) return(z3_int(ctx, name))
  if (isTRUE(symcompR::is_real(e))) return(z3_real(ctx, name))
  if (identical(expected, "bool")) return(z3_bool(ctx, name))
  stop(sprintf(paste(
    "cannot tell the sort of symbol '%s': give it the assumption integer = TRUE or",
    "real = TRUE, or pass sorts = list(%s = <sort>)"
  ), name, name), call. = FALSE)
}

sym_pow <- function(ctx, args, rec) {
  base <- rec(args[[1]], "arith")
  ex <- args[[2]]
  if (symcompR::sym_type(ex) == "Integer") {
    k <- as.numeric(ex$value)
    if (abs(k) > 2^31) stop("exponent too large to translate", call. = FALSE)
    if (k >= 0) return(base^k)
    return(1 / base^(-k))
  }
  if (symcompR::sym_type(ex) == "Rational") return(base^z3_real_val(ctx, as.character(ex$value)))
  base^rec(ex, "arith")
}

sym_piecewise <- function(ctx, args, rec, expected) {
  pairs <- lapply(args, symcompR::sym_args)
  n <- length(pairs)
  last <- pairs[[n]]
  if (!(symcompR::sym_type(last[[2]]) == "BooleanAtom" && symcompR::sym_name(last[[2]]) == "true")) {
    stop("only Piecewise expressions whose last condition is TRUE can be translated", call. = FALSE)
  }
  out <- rec(last[[1]], expected)
  for (i in rev(seq_len(n - 1L))) {
    out <- z3_ite(rec(pairs[[i]][[2]], "bool"), rec(pairs[[i]][[1]], expected), out)
  }
  out
}

sym_function <- function(ctx, e, args, rec) {
  name <- symcompR::sym_name(e)
  a <- lapply(args, rec, "arith")
  switch(name,
    Abs = z3_abs(a[[1]]),
    floor = z3_to_int(a[[1]]),
    ceiling = -z3_to_int(-a[[1]]),
    # Python and SymPy: a - b * floor(a / b); exact for Int and Real.
    Mod = a[[1]] - a[[2]] * z3_to_int(a[[1]] / a[[2]]),
    Max = Reduce(function(p, q) z3_ite(p >= q, p, q), a),
    Min = Reduce(function(p, q) z3_ite(p <= q, p, q), a),
    sym_unsupported(e)
  )
}

#' @rdname z3_from_symbolic
#' @export
z3_to_symbolic <- function(x) {
  need_symcompr()
  to_sym(x)
}

to_sym <- function(x) {
  S <- function(f) getExportedValue("symcompR", f)
  kind <- z3_kind(x)
  sort <- z3_sort_kind(x)
  if (kind == "numeral") {
    if (sort == "int") return(S("Integer")(cpp_value_info(x, 0L)$exact))
    if (sort == "real") {
      parts <- strsplit(cpp_value_info(x, 0L)$exact, "/", fixed = TRUE)[[1]]
      if (length(parts) == 1L) return(S("Integer")(parts))
      return(S("Rational")(parts[1], parts[2]))
    }
    stop(sprintf("numerals of sort %s have no symcompR counterpart", format(z3_sort(x))), call. = FALSE)
  }
  if (kind != "app") stop(sprintf("cannot translate %s to symcompR", format(x)), call. = FALSE)
  info <- z3_decl_info(z3_decl(x))
  args <- lapply(z3_args(x), to_sym)
  if (!info$interpreted) {
    if (length(args)) stop("uninterpreted functions have no symcompR counterpart", call. = FALSE)
    # Z3 prints a numbered symbol as k!<n>, the same as a constant named
    # "k!<n>": the two would become one symcompR symbol.
    if (grepl("^k![0-9]+$", info$name)) {
      stop(sprintf("the constant %s may be a numbered Z3 symbol, which has no unambiguous symcompR name",
                   info$name), call. = FALSE)
    }
    return(switch(sort,
      int = S("Symbol")(info$name, integer = TRUE),
      real = S("Symbol")(info$name, real = TRUE),
      bool = S("Symbol")(info$name),
      stop(sprintf("constants of sort %s have no symcompR counterpart", format(z3_sort(x))), call. = FALSE)
    ))
  }
  fold <- function(f) Reduce(f, args)
  switch(info$kind,
    true = S("true"),
    false = S("false"),
    add = fold(`+`),
    sub = fold(`-`),
    mul = fold(`*`),
    uminus = -args[[1]],
    div = args[[1]] / args[[2]],
    idiv = sym_euclid_div(args[[1]], args[[2]], S),
    mod = args[[1]] - S("Abs")(args[[2]]) * floor(args[[1]] / S("Abs")(args[[2]])),
    power = args[[1]]^args[[2]],
    to_real = args[[1]],
    to_int = floor(args[[1]]),
    abs = S("Abs")(args[[1]]),
    le = S("Le")(args[[1]], args[[2]]),
    lt = S("Lt")(args[[1]], args[[2]]),
    ge = S("Ge")(args[[1]], args[[2]]),
    gt = S("Gt")(args[[1]], args[[2]]),
    eq = if (sort_is_bool(z3_args(x)[[1]])) S("Equivalent")(args[[1]], args[[2]]) else S("Eq")(args[[1]], args[[2]]),
    distinct = do.call(S("And"), unlist(lapply(seq_along(args)[-length(args)], function(i) {
      lapply((i + 1):length(args), function(j) S("Ne")(args[[i]], args[[j]]))
    }), recursive = FALSE)),
    and = do.call(S("And"), args),
    or = do.call(S("Or"), args),
    not = S("Not")(args[[1]]),
    implies = S("Implies")(args[[1]], args[[2]]),
    xor = S("Xor")(args[[1]], args[[2]]),
    iff = S("Equivalent")(args[[1]], args[[2]]),
    ite = if (sort_is_bool(x)) S("ITE")(args[[1]], args[[2]], args[[3]]) else
      S("Piecewise")(list(args[[2]], args[[1]]), list(args[[3]], TRUE)),
    stop(sprintf("the Z3 operation %s has no symcompR translation", info$name), call. = FALSE)
  )
}

sort_is_bool <- function(x) z3_sort_kind(x) == "bool"

# Euclidean division: sign(b) * floor(a / |b|).
sym_euclid_div <- function(a, b, S) {
  S("Piecewise")(list(floor(a / b), S("Gt")(b, 0)), list(-floor(a / -b), TRUE))
}
