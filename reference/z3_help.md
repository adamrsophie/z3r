# Help text and DIMACS export

`z3_help()` returns Z3's description of the parameters of a solver,
optimizer, fixedpoint engine, tactic or simplifier; for a context, it
describes the parameters of
[`z3_simplify()`](https://adamrsophie.github.io/z3r/reference/z3_simplify.md).
With `names_only = TRUE` it returns only the parameter names, as Z3
lists them. `z3_to_dimacs()` writes the assertions of a propositional
solver or goal in DIMACS CNF.

## Usage

``` r
z3_help(x, names_only = FALSE)

z3_to_dimacs(x, include_names = TRUE)

z3_import_model_converter(src, dst)
```

## Arguments

- x:

  A `z3_solver`, `z3_optimize`, `z3_fixedpoint`, `z3_tactic`,
  `z3_simplifier`, `z3_context` or (for DIMACS) `z3_goal`.

- names_only:

  If `TRUE`, return only the parameter names.

- include_names:

  Whether to include variable names as comments.

- src, dst:

  `z3_solver` objects.

## Value

A string.

## Details

`z3_import_model_converter(src, dst)` gives `dst` the model converter of
`src`, so that models of `dst` are translated the way `src` would
translate them (useful when `dst` solves a preprocessed copy of `src`'s
problem).

## Examples

``` r
ctx <- z3_context()
s <- z3_solver(ctx)
substr(z3_help(s), 1, 60)
#> [1] "algebraic_number_evaluator (bool) simplify/evaluate expressi"
z3_help(z3_tactic(ctx, "simplify"), names_only = TRUE)
#> [1] "(ite_extra_rules, flat, flat_and_or, sort_disjunctions, elim_and, elim_ite, local_ctx, local_ctx_limit, blast_distinct, blast_distinct_threshold, som, som_blowup, hoist_mul, hoist_ite, algebraic_number_evaluator, mul_to_power, expand_power, expand_tan, max_degree, sort_sums, gcd_rounding, arith_lhs, arith_ineq_lhs, elim_to_real, push_to_real, eq2ineq, elim_rem, split_concat_eq, bit2bool, blast_eq_value, elim_sign_ext, hi_div0, mul2concat, bv_sort_ac, bv_extract_prop, bv_not_simpl, bv_ite2id, bv_le_extra, bv_le2extract, expand_select_store, blast_select_store, expand_nested_stores, expand_select_ite, expand_store_eq, sort_store, max_memory, max_steps, push_ite_arith, push_ite_bv, pull_cheap_ite, bv_ineq_consistency_test_max, unfold_recursive_functions, cache_all, enable_der, rewrite_patterns, ignore_patterns_on_ground_qbody)"
substr(z3_help(ctx), 1, 60)
#> [1] "algebraic_number_evaluator (bool) simplify/evaluate expressi"
p <- z3_bools(ctx, c("a", "b"))
z3_add(s, p$a | p$b, !p$a)
cat(z3_to_dimacs(s))
#> p cnf 2 2
#> 1 2 0
#> -1 0
#> c 1 a
#> c 2 b
```
