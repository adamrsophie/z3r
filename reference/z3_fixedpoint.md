# Horn clauses and Datalog: the fixedpoint engine

Z3's fixedpoint engine decides reachability questions over recursive
relations: Datalog queries over finite relations, and constrained Horn
clauses (CHC) over arithmetic and other theories, solved by the Spacer
engine. Many program verification problems ("can this program reach an
error state?") are CHC problems.

## Usage

``` r
z3_fixedpoint(ctx)

z3_register_relation(fp, relation)

z3_add_rule(fp, rule, name = "", update = FALSE)

z3_add_fact(fp, relation, args)

z3_query(fp, query, level = NULL)

z3_answer(fp)

z3_ground_answer(fp)

z3_answer_trace(fp)

z3_answer_rule_names(fp)

z3_cover(fp, level, relation)

z3_add_cover(fp, level, relation, property)

z3_num_levels(fp, relation)

z3_add_invariant(fp, relation, property)

z3_reachable(fp, relation)

z3_fixedpoint_rules(fp)

z3_fixedpoint_assertions(fp)

z3_set_representation(fp, relation, kinds)
```

## Arguments

- ctx:

  A
  [`z3_context()`](https://adamrsophie.github.io/z3r/reference/z3_context.md).

- fp:

  A `z3_fixedpoint`.

- relation:

  A relation (a `z3_function` with Bool range).

- rule:

  A Boolean formula.

- name:

  Optional rule name.

- update:

  If `TRUE`, replace the rule of the same name.

- args:

  Indices of the elements of a fact.

- query:

  A Boolean formula, or a list of relations for `z3_query()`.

- level:

  Search level. For `z3_query()`, `NULL` (the default) searches from
  level 0 and a number starts the Spacer search at that level.

- property:

  A Boolean formula over the relation's arguments, given as bound
  variables (see
  [`z3_bound_var()`](https://adamrsophie.github.io/z3r/reference/z3_inspect.md)).

- kinds:

  Relation representation names for `z3_set_representation()`.

## Value

`z3_fixedpoint()` returns an engine; `z3_query()` returns `"sat"`,
`"unsat"` or `"unknown"`; the answer functions return expressions; the
others return `fp` invisibly.

## Details

1.  Create an engine with `z3_fixedpoint(ctx)`.

2.  Declare relations as Boolean-valued functions
    ([`z3_function()`](https://adamrsophie.github.io/z3r/reference/z3_function.md)
    with range Bool) and register them with `z3_register_relation()`.

3.  Add rules with `z3_add_rule()`: an implication `body => head`,
    usually universally quantified with
    [`z3_forall()`](https://adamrsophie.github.io/z3r/reference/z3_forall.md),
    or a fact. Rules may be named.

4.  Ask `z3_query(fp, q)` whether `q` is derivable. `"sat"` means yes;
    `z3_answer()` then gives a derivation (or the set of reachable
    tuples). `"unsat"` means no; `z3_answer()` gives an inductive
    invariant proving it.

Spacer functions: `z3_ground_answer()` (a ground derivation),
`z3_answer_trace()` (rules used along the counterexample),
`z3_answer_rule_names()`, `z3_add_invariant()` (a known lemma) and
`z3_reachable()` (the reachable states found for a predicate).
`z3_cover()` and `z3_add_cover()` read and add properties of a predicate
at a search level (`-1` is the invariant level; `z3_add_cover()` accepts
levels up to 1024, or up to `z3_num_levels()` when that is larger), and
`z3_query(fp, q, level)` starts the search at a given level.

`z3_set_params(fp, engine = "datalog")` selects the Datalog engine;
parameters are listed by `z3_param_help(fp)`. `z3_add_fact()` adds a
tuple to a relation over finite domain sorts
([`z3_finite_domain_sort()`](https://adamrsophie.github.io/z3r/reference/z3_finite_domain_sort.md)).
`z3_fixedpoint_rules()` and `z3_fixedpoint_assertions()` list the
content;
[`z3_to_smtlib()`](https://adamrsophie.github.io/z3r/reference/z3_to_smtlib.md)
prints it, and
[`z3_add_smtlib()`](https://adamrsophie.github.io/z3r/reference/z3_to_smtlib.md)
parses rules in Z3's fixedpoint format.

## Examples

``` r
ctx <- z3_context()
I <- z3_int_sort(ctx)
B <- z3_bool_sort(ctx)
# Program: x := 0; while (*) x := x + 2. Can x ever be 7?
inv <- z3_function(ctx, "inv", I, B)
x <- z3_int(ctx, "x")
fp <- z3_fixedpoint(ctx)
z3_register_relation(fp, inv)
z3_add_rule(fp, inv(0))
z3_add_rule(fp, z3_forall(x, z3_implies(inv(x), inv(x + 2))))
z3_query(fp, z3_exists(x, inv(x) & x == 7))
#> [1] "unsat"
z3_answer(fp)
#> <z3_expr Bool> (forall ((A Int))
#>   (! (= (inv A)
#>         (and (not (= A 7))
#>              (not (= A 5))
#>              (not (= A 3))
#>              (not (= A 1))
#>              (not (<= A (- 1)))))
#>      :weight 0))
z3_query(fp, z3_exists(x, inv(x) & x == 8))
#> [1] "sat"
```
