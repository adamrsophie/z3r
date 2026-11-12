# Fixedpoint callbacks

`z3_on_lemma(fp, lemma, predecessor, unfold)` registers R functions
called by the Spacer engine during
[`z3_query()`](https://adamrsophie.github.io/z3r/reference/z3_fixedpoint.md):
`lemma(lemma, level)` for every lemma Spacer learns (an implication from
a predicate application to a property; `level` is `Inf` for inductive
invariants), and `predecessor()` and `unfold()` when Spacer computes a
predecessor or unfolds the search to a new level. Giving `lemma` also
turns on Spacer's `spacer.p3.share_lemmas` and
`spacer.p3.share_invariants`, without which Spacer reports no lemmas.

## Usage

``` r
z3_on_lemma(fp, lemma = NULL, predecessor = NULL, unfold = NULL)

z3_fixedpoint_external(fp, app = NULL, assign = NULL)
```

## Arguments

- fp:

  A `z3_fixedpoint`.

- lemma, predecessor, unfold, app, assign:

  Callback functions, or `NULL`.

## Value

`fp`, invisibly.

## Details

`z3_fixedpoint_external(fp, app, assign)` supplies user-defined
relations for the Datalog engine (Z3's external relations):
`app(decl, args)` is called to reduce applications of relational
operators and returns a term of the operator's range sort, or `NULL` to
keep the application; `assign(decl, args, outs)` is called for
destructive updates. It can be set once per engine.

The callbacks follow the rules of
[`z3_propagator()`](https://adamrsophie.github.io/z3r/reference/z3_propagator.md)
callbacks: errors stop the query and are raised afterwards, and the
engine cannot be used from its own callbacks.

## Examples

``` r
ctx <- z3_context()
I <- z3_int_sort(ctx)
inv <- z3_function(ctx, "inv", I, z3_bool_sort(ctx))
x <- z3_int(ctx, "x")
fp <- z3_fixedpoint(ctx)
z3_register_relation(fp, inv)
z3_add_rule(fp, inv(0))
z3_add_rule(fp, z3_forall(x, z3_implies(inv(x), inv(x + 2))))
lemmas <- list()
z3_on_lemma(fp, lemma = function(lemma, level) lemmas[[length(lemmas) + 1]] <<- lemma)
z3_query(fp, z3_exists(x, inv(x) & x == 7))
#> [1] "unsat"
length(lemmas) > 0
#> [1] TRUE
```
