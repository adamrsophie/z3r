# Package index

## Contexts and versions

- [`z3_context()`](https://adamrsophie.github.io/z3r/reference/z3_context.md)
  : Create a Z3 context
- [`z3_version()`](https://adamrsophie.github.io/z3r/reference/z3_version.md)
  [`z3_version_at_least()`](https://adamrsophie.github.io/z3r/reference/z3_version.md)
  : Z3 library version
- [`z3_context_of()`](https://adamrsophie.github.io/z3r/reference/z3_context_of.md)
  [`z3_same_context()`](https://adamrsophie.github.io/z3r/reference/z3_context_of.md)
  : Context of a Z3 object
- [`z3_translate()`](https://adamrsophie.github.io/z3r/reference/z3_translate.md)
  : Translate objects between contexts
- [`z3_global_param_set()`](https://adamrsophie.github.io/z3r/reference/z3_global_param_set.md)
  [`z3_global_param_get()`](https://adamrsophie.github.io/z3r/reference/z3_global_param_set.md)
  [`z3_global_param_reset()`](https://adamrsophie.github.io/z3r/reference/z3_global_param_set.md)
  [`z3_global_param_help()`](https://adamrsophie.github.io/z3r/reference/z3_global_param_set.md)
  [`z3_log_open()`](https://adamrsophie.github.io/z3r/reference/z3_global_param_set.md)
  [`z3_log_close()`](https://adamrsophie.github.io/z3r/reference/z3_global_param_set.md)
  [`z3_warnings()`](https://adamrsophie.github.io/z3r/reference/z3_global_param_set.md)
  : Global Z3 parameters and logging
- [`z3_set_print_mode()`](https://adamrsophie.github.io/z3r/reference/z3_set_print_mode.md)
  : Printing mode
- [`z3_memory_usage()`](https://adamrsophie.github.io/z3r/reference/z3_memory_usage.md)
  [`z3_log_append()`](https://adamrsophie.github.io/z3r/reference/z3_memory_usage.md)
  [`z3_update_param()`](https://adamrsophie.github.io/z3r/reference/z3_memory_usage.md)
  : Memory, logging and context parameters
- [`z3_trace()`](https://adamrsophie.github.io/z3r/reference/z3_trace.md)
  : Z3 trace messages

## Sorts

- [`z3_int_sort()`](https://adamrsophie.github.io/z3r/reference/z3_sorts.md)
  [`z3_real_sort()`](https://adamrsophie.github.io/z3r/reference/z3_sorts.md)
  [`z3_bool_sort()`](https://adamrsophie.github.io/z3r/reference/z3_sorts.md)
  [`z3_bv_sort()`](https://adamrsophie.github.io/z3r/reference/z3_sorts.md)
  [`z3_string_sort()`](https://adamrsophie.github.io/z3r/reference/z3_sorts.md)
  [`z3_array_sort()`](https://adamrsophie.github.io/z3r/reference/z3_sorts.md)
  [`z3_uninterpreted_sort()`](https://adamrsophie.github.io/z3r/reference/z3_sorts.md)
  : Z3 sorts
- [`z3_sort()`](https://adamrsophie.github.io/z3r/reference/z3_sort.md)
  [`z3_sort_kind()`](https://adamrsophie.github.io/z3r/reference/z3_sort.md)
  [`z3_sort_name()`](https://adamrsophie.github.io/z3r/reference/z3_sort.md)
  : Sort of an expression
- [`z3_finite_domain_sort()`](https://adamrsophie.github.io/z3r/reference/z3_finite_domain_sort.md)
  [`z3_finite_domain_size()`](https://adamrsophie.github.io/z3r/reference/z3_finite_domain_sort.md)
  : Finite domain sorts

## Constants and values

- [`z3_const()`](https://adamrsophie.github.io/z3r/reference/z3_const.md)
  [`z3_int()`](https://adamrsophie.github.io/z3r/reference/z3_const.md)
  [`z3_real()`](https://adamrsophie.github.io/z3r/reference/z3_const.md)
  [`z3_bool()`](https://adamrsophie.github.io/z3r/reference/z3_const.md)
  [`z3_bv()`](https://adamrsophie.github.io/z3r/reference/z3_const.md)
  [`z3_string()`](https://adamrsophie.github.io/z3r/reference/z3_const.md)
  [`z3_ints()`](https://adamrsophie.github.io/z3r/reference/z3_const.md)
  [`z3_reals()`](https://adamrsophie.github.io/z3r/reference/z3_const.md)
  [`z3_bools()`](https://adamrsophie.github.io/z3r/reference/z3_const.md)
  [`z3_fresh_const()`](https://adamrsophie.github.io/z3r/reference/z3_const.md)
  : Symbolic constants
- [`z3_int_val()`](https://adamrsophie.github.io/z3r/reference/z3_int_val.md)
  [`z3_real_val()`](https://adamrsophie.github.io/z3r/reference/z3_int_val.md)
  [`z3_bool_val()`](https://adamrsophie.github.io/z3r/reference/z3_int_val.md)
  [`z3_bv_val()`](https://adamrsophie.github.io/z3r/reference/z3_int_val.md)
  [`z3_string_val()`](https://adamrsophie.github.io/z3r/reference/z3_int_val.md)
  [`z3_val()`](https://adamrsophie.github.io/z3r/reference/z3_int_val.md)
  : Exact numeric, Boolean and string values
- [`z3_value()`](https://adamrsophie.github.io/z3r/reference/z3_value.md)
  : Convert a Z3 value to an R value
- [`z3_decimal()`](https://adamrsophie.github.io/z3r/reference/z3_decimal.md)
  [`z3_binary_string()`](https://adamrsophie.github.io/z3r/reference/z3_decimal.md)
  : Decimal digits of a number
- [`z3_string_escaped()`](https://adamrsophie.github.io/z3r/reference/z3_string_escaped.md)
  [`z3_string_bytes()`](https://adamrsophie.github.io/z3r/reference/z3_string_escaped.md)
  : String values as escaped text or bytes
- [`z3_is_expr()`](https://adamrsophie.github.io/z3r/reference/z3_is_expr.md)
  : Test for z3r objects

## Operators and functions

- [`Ops(`*`<z3_expr>`*`)`](https://adamrsophie.github.io/z3r/reference/z3_operators.md)
  : Operators on Z3 expressions
- [`z3_and()`](https://adamrsophie.github.io/z3r/reference/z3_and.md)
  [`z3_or()`](https://adamrsophie.github.io/z3r/reference/z3_and.md)
  [`z3_not()`](https://adamrsophie.github.io/z3r/reference/z3_and.md)
  [`z3_implies()`](https://adamrsophie.github.io/z3r/reference/z3_and.md)
  [`z3_xor()`](https://adamrsophie.github.io/z3r/reference/z3_and.md)
  [`z3_iff()`](https://adamrsophie.github.io/z3r/reference/z3_and.md)
  [`z3_ite()`](https://adamrsophie.github.io/z3r/reference/z3_and.md)
  [`z3_distinct()`](https://adamrsophie.github.io/z3r/reference/z3_and.md)
  : Logical connectives
- [`z3_sum()`](https://adamrsophie.github.io/z3r/reference/z3_sum.md)
  [`z3_prod()`](https://adamrsophie.github.io/z3r/reference/z3_sum.md)
  [`z3_div()`](https://adamrsophie.github.io/z3r/reference/z3_sum.md)
  [`z3_mod()`](https://adamrsophie.github.io/z3r/reference/z3_sum.md)
  [`z3_rem()`](https://adamrsophie.github.io/z3r/reference/z3_sum.md)
  [`z3_abs()`](https://adamrsophie.github.io/z3r/reference/z3_sum.md)
  [`z3_to_real()`](https://adamrsophie.github.io/z3r/reference/z3_sum.md)
  [`z3_to_int()`](https://adamrsophie.github.io/z3r/reference/z3_sum.md)
  [`z3_is_int()`](https://adamrsophie.github.io/z3r/reference/z3_sum.md)
  : Arithmetic functions
- [`z3_divides()`](https://adamrsophie.github.io/z3r/reference/z3_divides.md)
  : Divisibility
- [`z3_atmost()`](https://adamrsophie.github.io/z3r/reference/z3_atmost.md)
  [`z3_atleast()`](https://adamrsophie.github.io/z3r/reference/z3_atmost.md)
  [`z3_pble()`](https://adamrsophie.github.io/z3r/reference/z3_atmost.md)
  [`z3_pbge()`](https://adamrsophie.github.io/z3r/reference/z3_atmost.md)
  [`z3_pbeq()`](https://adamrsophie.github.io/z3r/reference/z3_atmost.md)
  : Cardinality and pseudo-Boolean constraints

## Solving

- [`z3_solver()`](https://adamrsophie.github.io/z3r/reference/z3_solver.md)
  : Create a solver
- [`z3_add()`](https://adamrsophie.github.io/z3r/reference/z3_add.md) :
  Add constraints
- [`z3_check()`](https://adamrsophie.github.io/z3r/reference/z3_check.md)
  : Check satisfiability
- [`z3_interrupt()`](https://adamrsophie.github.io/z3r/reference/z3_interrupt.md)
  : Interrupt a running check
- [`z3_model()`](https://adamrsophie.github.io/z3r/reference/z3_model.md)
  : Model of the last satisfiable check
- [`z3_eval()`](https://adamrsophie.github.io/z3r/reference/z3_eval.md)
  : Evaluate an expression in a model
- [`as.list(`*`<z3_model>`*`)`](https://adamrsophie.github.io/z3r/reference/as.list.z3_model.md)
  [`z3_model_values()`](https://adamrsophie.github.io/z3r/reference/as.list.z3_model.md)
  [`z3_model_functions()`](https://adamrsophie.github.io/z3r/reference/as.list.z3_model.md)
  : Contents of a model
- [`z3_model_set()`](https://adamrsophie.github.io/z3r/reference/z3_model_set.md)
  [`z3_model_set_function()`](https://adamrsophie.github.io/z3r/reference/z3_model_set.md)
  [`z3_model_set_default()`](https://adamrsophie.github.io/z3r/reference/z3_model_set.md)
  [`z3_model_has()`](https://adamrsophie.github.io/z3r/reference/z3_model_set.md)
  [`z3_model_universes()`](https://adamrsophie.github.io/z3r/reference/z3_model_set.md)
  [`z3_as_array_function()`](https://adamrsophie.github.io/z3r/reference/z3_model_set.md)
  : Build and query models
- [`z3_push()`](https://adamrsophie.github.io/z3r/reference/z3_push.md)
  [`z3_pop()`](https://adamrsophie.github.io/z3r/reference/z3_push.md)
  [`z3_reset()`](https://adamrsophie.github.io/z3r/reference/z3_push.md)
  [`z3_num_scopes()`](https://adamrsophie.github.io/z3r/reference/z3_push.md)
  : Incremental solving: scopes
- [`z3_with_scope()`](https://adamrsophie.github.io/z3r/reference/z3_with_scope.md)
  [`z3_all_models()`](https://adamrsophie.github.io/z3r/reference/z3_with_scope.md)
  : Scoped solving and model enumeration
- [`z3_assertions()`](https://adamrsophie.github.io/z3r/reference/z3_assertions.md)
  : Assertions of a solver, optimizer or goal
- [`z3_assert_and_track()`](https://adamrsophie.github.io/z3r/reference/z3_assert_and_track.md)
  : Track assertions for unsatisfiable cores
- [`z3_unsat_core()`](https://adamrsophie.github.io/z3r/reference/z3_unsat_core.md)
  : Unsatisfiable core
- [`z3_reason_unknown()`](https://adamrsophie.github.io/z3r/reference/z3_reason_unknown.md)
  : Reason for an unknown result
- [`z3_set_params()`](https://adamrsophie.github.io/z3r/reference/z3_set_params.md)
  [`z3_param_help()`](https://adamrsophie.github.io/z3r/reference/z3_set_params.md)
  : Solver and optimizer parameters
- [`z3_statistics()`](https://adamrsophie.github.io/z3r/reference/z3_statistics.md)
  [`z3_statistics_text()`](https://adamrsophie.github.io/z3r/reference/z3_statistics.md)
  : Solver statistics
- [`z3_help()`](https://adamrsophie.github.io/z3r/reference/z3_help.md)
  [`z3_to_dimacs()`](https://adamrsophie.github.io/z3r/reference/z3_help.md)
  [`z3_import_model_converter()`](https://adamrsophie.github.io/z3r/reference/z3_help.md)
  : Help text and DIMACS export
- [`z3_proof()`](https://adamrsophie.github.io/z3r/reference/z3_proof.md)
  [`z3_consequences()`](https://adamrsophie.github.io/z3r/reference/z3_proof.md)
  [`z3_cube()`](https://adamrsophie.github.io/z3r/reference/z3_proof.md)
  [`z3_units()`](https://adamrsophie.github.io/z3r/reference/z3_proof.md)
  [`z3_non_units()`](https://adamrsophie.github.io/z3r/reference/z3_proof.md)
  [`z3_trail()`](https://adamrsophie.github.io/z3r/reference/z3_proof.md)
  : Proofs, consequences and cubes
- [`z3_congruence_root()`](https://adamrsophie.github.io/z3r/reference/z3_congruence_root.md)
  [`z3_congruence_next()`](https://adamrsophie.github.io/z3r/reference/z3_congruence_root.md)
  [`z3_congruence_explain()`](https://adamrsophie.github.io/z3r/reference/z3_congruence_root.md)
  : Congruence classes
- [`z3_solve_for()`](https://adamrsophie.github.io/z3r/reference/z3_solve_for.md)
  : Solve for variables
- [`z3_set_initial_value()`](https://adamrsophie.github.io/z3r/reference/z3_set_initial_value.md)
  : Initial values (phase hints)
- [`z3_implied_equalities()`](https://adamrsophie.github.io/z3r/reference/z3_implied_equalities.md)
  : Implied equalities

## Optimization

- [`z3_optimize()`](https://adamrsophie.github.io/z3r/reference/z3_optimize.md)
  : Create an optimizer
- [`z3_minimize()`](https://adamrsophie.github.io/z3r/reference/z3_minimize.md)
  [`z3_maximize()`](https://adamrsophie.github.io/z3r/reference/z3_minimize.md)
  : Objectives
- [`z3_add_soft()`](https://adamrsophie.github.io/z3r/reference/z3_add_soft.md)
  : Soft constraints
- [`z3_optimum()`](https://adamrsophie.github.io/z3r/reference/z3_optimum.md)
  : Optimal value of an objective

## Bit-vectors and floating point

- [`z3_bvult()`](https://adamrsophie.github.io/z3r/reference/z3_bitvectors.md)
  [`z3_bvule()`](https://adamrsophie.github.io/z3r/reference/z3_bitvectors.md)
  [`z3_bvugt()`](https://adamrsophie.github.io/z3r/reference/z3_bitvectors.md)
  [`z3_bvuge()`](https://adamrsophie.github.io/z3r/reference/z3_bitvectors.md)
  [`z3_bvslt()`](https://adamrsophie.github.io/z3r/reference/z3_bitvectors.md)
  [`z3_bvsle()`](https://adamrsophie.github.io/z3r/reference/z3_bitvectors.md)
  [`z3_bvsgt()`](https://adamrsophie.github.io/z3r/reference/z3_bitvectors.md)
  [`z3_bvsge()`](https://adamrsophie.github.io/z3r/reference/z3_bitvectors.md)
  [`z3_bvudiv()`](https://adamrsophie.github.io/z3r/reference/z3_bitvectors.md)
  [`z3_bvsdiv()`](https://adamrsophie.github.io/z3r/reference/z3_bitvectors.md)
  [`z3_bvurem()`](https://adamrsophie.github.io/z3r/reference/z3_bitvectors.md)
  [`z3_bvsrem()`](https://adamrsophie.github.io/z3r/reference/z3_bitvectors.md)
  [`z3_bvsmod()`](https://adamrsophie.github.io/z3r/reference/z3_bitvectors.md)
  [`z3_bvand()`](https://adamrsophie.github.io/z3r/reference/z3_bitvectors.md)
  [`z3_bvor()`](https://adamrsophie.github.io/z3r/reference/z3_bitvectors.md)
  [`z3_bvxor()`](https://adamrsophie.github.io/z3r/reference/z3_bitvectors.md)
  [`z3_bvnand()`](https://adamrsophie.github.io/z3r/reference/z3_bitvectors.md)
  [`z3_bvnor()`](https://adamrsophie.github.io/z3r/reference/z3_bitvectors.md)
  [`z3_bvxnor()`](https://adamrsophie.github.io/z3r/reference/z3_bitvectors.md)
  [`z3_bvshl()`](https://adamrsophie.github.io/z3r/reference/z3_bitvectors.md)
  [`z3_bvlshr()`](https://adamrsophie.github.io/z3r/reference/z3_bitvectors.md)
  [`z3_bvashr()`](https://adamrsophie.github.io/z3r/reference/z3_bitvectors.md)
  [`z3_bvnot()`](https://adamrsophie.github.io/z3r/reference/z3_bitvectors.md)
  [`z3_bvneg()`](https://adamrsophie.github.io/z3r/reference/z3_bitvectors.md)
  [`z3_concat()`](https://adamrsophie.github.io/z3r/reference/z3_bitvectors.md)
  [`z3_extract()`](https://adamrsophie.github.io/z3r/reference/z3_bitvectors.md)
  [`z3_zero_ext()`](https://adamrsophie.github.io/z3r/reference/z3_bitvectors.md)
  [`z3_sign_ext()`](https://adamrsophie.github.io/z3r/reference/z3_bitvectors.md)
  [`z3_rotate_left()`](https://adamrsophie.github.io/z3r/reference/z3_bitvectors.md)
  [`z3_rotate_right()`](https://adamrsophie.github.io/z3r/reference/z3_bitvectors.md)
  [`z3_repeat()`](https://adamrsophie.github.io/z3r/reference/z3_bitvectors.md)
  [`z3_bv2int()`](https://adamrsophie.github.io/z3r/reference/z3_bitvectors.md)
  [`z3_int2bv()`](https://adamrsophie.github.io/z3r/reference/z3_bitvectors.md)
  [`z3_bvadd_no_overflow()`](https://adamrsophie.github.io/z3r/reference/z3_bitvectors.md)
  [`z3_bvmul_no_overflow()`](https://adamrsophie.github.io/z3r/reference/z3_bitvectors.md)
  [`z3_bvsub_no_underflow()`](https://adamrsophie.github.io/z3r/reference/z3_bitvectors.md)
  [`z3_bvadd_no_underflow()`](https://adamrsophie.github.io/z3r/reference/z3_bitvectors.md)
  [`z3_bvsub_no_overflow()`](https://adamrsophie.github.io/z3r/reference/z3_bitvectors.md)
  [`z3_bvmul_no_underflow()`](https://adamrsophie.github.io/z3r/reference/z3_bitvectors.md)
  [`z3_bvsdiv_no_overflow()`](https://adamrsophie.github.io/z3r/reference/z3_bitvectors.md)
  [`z3_bv_width()`](https://adamrsophie.github.io/z3r/reference/z3_bitvectors.md)
  [`z3_bvneg_no_overflow()`](https://adamrsophie.github.io/z3r/reference/z3_bitvectors.md)
  [`z3_bit2bool()`](https://adamrsophie.github.io/z3r/reference/z3_bitvectors.md)
  [`z3_bvredand()`](https://adamrsophie.github.io/z3r/reference/z3_bitvectors.md)
  [`z3_bvredor()`](https://adamrsophie.github.io/z3r/reference/z3_bitvectors.md)
  [`z3_rotate_left_by()`](https://adamrsophie.github.io/z3r/reference/z3_bitvectors.md)
  [`z3_rotate_right_by()`](https://adamrsophie.github.io/z3r/reference/z3_bitvectors.md)
  [`z3_ubv_to_str()`](https://adamrsophie.github.io/z3r/reference/z3_bitvectors.md)
  [`z3_sbv_to_str()`](https://adamrsophie.github.io/z3r/reference/z3_bitvectors.md)
  [`z3_bv_from_bits()`](https://adamrsophie.github.io/z3r/reference/z3_bitvectors.md)
  : Bit-vector operations
- [`z3_fp_sort()`](https://adamrsophie.github.io/z3r/reference/z3_fp.md)
  [`z3_fp16_sort()`](https://adamrsophie.github.io/z3r/reference/z3_fp.md)
  [`z3_fp32_sort()`](https://adamrsophie.github.io/z3r/reference/z3_fp.md)
  [`z3_fp64_sort()`](https://adamrsophie.github.io/z3r/reference/z3_fp.md)
  [`z3_fp128_sort()`](https://adamrsophie.github.io/z3r/reference/z3_fp.md)
  [`z3_rounding_sort()`](https://adamrsophie.github.io/z3r/reference/z3_fp.md)
  [`z3_fp_sort_info()`](https://adamrsophie.github.io/z3r/reference/z3_fp.md)
  [`z3_fp()`](https://adamrsophie.github.io/z3r/reference/z3_fp.md)
  [`z3_fp_val()`](https://adamrsophie.github.io/z3r/reference/z3_fp.md)
  [`z3_rounding()`](https://adamrsophie.github.io/z3r/reference/z3_fp.md)
  [`z3_fp_nan()`](https://adamrsophie.github.io/z3r/reference/z3_fp.md)
  [`z3_fp_inf()`](https://adamrsophie.github.io/z3r/reference/z3_fp.md)
  [`z3_fp_zero()`](https://adamrsophie.github.io/z3r/reference/z3_fp.md)
  [`z3_fp_from_bits()`](https://adamrsophie.github.io/z3r/reference/z3_fp.md)
  [`z3_fp_add()`](https://adamrsophie.github.io/z3r/reference/z3_fp.md)
  [`z3_fp_sub()`](https://adamrsophie.github.io/z3r/reference/z3_fp.md)
  [`z3_fp_mul()`](https://adamrsophie.github.io/z3r/reference/z3_fp.md)
  [`z3_fp_div()`](https://adamrsophie.github.io/z3r/reference/z3_fp.md)
  [`z3_fp_fma()`](https://adamrsophie.github.io/z3r/reference/z3_fp.md)
  [`z3_fp_sqrt()`](https://adamrsophie.github.io/z3r/reference/z3_fp.md)
  [`z3_fp_round_to_integral()`](https://adamrsophie.github.io/z3r/reference/z3_fp.md)
  [`z3_fp_rem()`](https://adamrsophie.github.io/z3r/reference/z3_fp.md)
  [`z3_fp_min()`](https://adamrsophie.github.io/z3r/reference/z3_fp.md)
  [`z3_fp_max()`](https://adamrsophie.github.io/z3r/reference/z3_fp.md)
  [`z3_fp_abs()`](https://adamrsophie.github.io/z3r/reference/z3_fp.md)
  [`z3_fp_neg()`](https://adamrsophie.github.io/z3r/reference/z3_fp.md)
  [`z3_fp_eq()`](https://adamrsophie.github.io/z3r/reference/z3_fp.md)
  [`z3_fp_lt()`](https://adamrsophie.github.io/z3r/reference/z3_fp.md)
  [`z3_fp_leq()`](https://adamrsophie.github.io/z3r/reference/z3_fp.md)
  [`z3_fp_gt()`](https://adamrsophie.github.io/z3r/reference/z3_fp.md)
  [`z3_fp_geq()`](https://adamrsophie.github.io/z3r/reference/z3_fp.md)
  [`z3_fp_is_nan()`](https://adamrsophie.github.io/z3r/reference/z3_fp.md)
  [`z3_fp_is_infinite()`](https://adamrsophie.github.io/z3r/reference/z3_fp.md)
  [`z3_fp_is_zero()`](https://adamrsophie.github.io/z3r/reference/z3_fp.md)
  [`z3_fp_is_normal()`](https://adamrsophie.github.io/z3r/reference/z3_fp.md)
  [`z3_fp_is_subnormal()`](https://adamrsophie.github.io/z3r/reference/z3_fp.md)
  [`z3_fp_is_negative()`](https://adamrsophie.github.io/z3r/reference/z3_fp.md)
  [`z3_fp_is_positive()`](https://adamrsophie.github.io/z3r/reference/z3_fp.md)
  [`z3_fp_to_real()`](https://adamrsophie.github.io/z3r/reference/z3_fp.md)
  [`z3_fp_to_ieee_bv()`](https://adamrsophie.github.io/z3r/reference/z3_fp.md)
  [`z3_fp_to_ubv()`](https://adamrsophie.github.io/z3r/reference/z3_fp.md)
  [`z3_fp_to_sbv()`](https://adamrsophie.github.io/z3r/reference/z3_fp.md)
  [`z3_fp_to_fp()`](https://adamrsophie.github.io/z3r/reference/z3_fp.md)
  [`z3_fp_from_bv()`](https://adamrsophie.github.io/z3r/reference/z3_fp.md)
  [`z3_fp_from_signed()`](https://adamrsophie.github.io/z3r/reference/z3_fp.md)
  [`z3_fp_from_unsigned()`](https://adamrsophie.github.io/z3r/reference/z3_fp.md)
  [`z3_fp_numeral_parts()`](https://adamrsophie.github.io/z3r/reference/z3_fp.md)
  [`z3_fp_from_parts()`](https://adamrsophie.github.io/z3r/reference/z3_fp.md)
  : IEEE 754 floating point

## Arrays, sets and sequences

- [`z3_array()`](https://adamrsophie.github.io/z3r/reference/z3_array.md)
  [`z3_select()`](https://adamrsophie.github.io/z3r/reference/z3_array.md)
  [`z3_store()`](https://adamrsophie.github.io/z3r/reference/z3_array.md)
  [`z3_const_array()`](https://adamrsophie.github.io/z3r/reference/z3_array.md)
  [`z3_array_map()`](https://adamrsophie.github.io/z3r/reference/z3_array.md)
  [`z3_array_default()`](https://adamrsophie.github.io/z3r/reference/z3_array.md)
  [`z3_array_ext()`](https://adamrsophie.github.io/z3r/reference/z3_array.md)
  [`z3_as_array()`](https://adamrsophie.github.io/z3r/reference/z3_array.md)
  [`z3_lambda()`](https://adamrsophie.github.io/z3r/reference/z3_array.md)
  [`z3_array_sort_info()`](https://adamrsophie.github.io/z3r/reference/z3_array.md)
  : Arrays
- [`z3_set_sort()`](https://adamrsophie.github.io/z3r/reference/z3_set_sort.md)
  [`z3_empty_set()`](https://adamrsophie.github.io/z3r/reference/z3_set_sort.md)
  [`z3_full_set()`](https://adamrsophie.github.io/z3r/reference/z3_set_sort.md)
  [`z3_set_add()`](https://adamrsophie.github.io/z3r/reference/z3_set_sort.md)
  [`z3_set_del()`](https://adamrsophie.github.io/z3r/reference/z3_set_sort.md)
  [`z3_set_union()`](https://adamrsophie.github.io/z3r/reference/z3_set_sort.md)
  [`z3_set_intersect()`](https://adamrsophie.github.io/z3r/reference/z3_set_sort.md)
  [`z3_set_difference()`](https://adamrsophie.github.io/z3r/reference/z3_set_sort.md)
  [`z3_set_complement()`](https://adamrsophie.github.io/z3r/reference/z3_set_sort.md)
  [`z3_set_member()`](https://adamrsophie.github.io/z3r/reference/z3_set_sort.md)
  [`z3_set_subset()`](https://adamrsophie.github.io/z3r/reference/z3_set_sort.md)
  : Sets
- [`z3_length()`](https://adamrsophie.github.io/z3r/reference/z3_strings.md)
  [`z3_contains()`](https://adamrsophie.github.io/z3r/reference/z3_strings.md)
  [`z3_prefixof()`](https://adamrsophie.github.io/z3r/reference/z3_strings.md)
  [`z3_suffixof()`](https://adamrsophie.github.io/z3r/reference/z3_strings.md)
  [`z3_substr()`](https://adamrsophie.github.io/z3r/reference/z3_strings.md)
  [`z3_str_at()`](https://adamrsophie.github.io/z3r/reference/z3_strings.md)
  [`z3_indexof()`](https://adamrsophie.github.io/z3r/reference/z3_strings.md)
  [`z3_replace()`](https://adamrsophie.github.io/z3r/reference/z3_strings.md)
  [`z3_str_to_int()`](https://adamrsophie.github.io/z3r/reference/z3_strings.md)
  [`z3_int_to_str()`](https://adamrsophie.github.io/z3r/reference/z3_strings.md)
  [`z3_str_lt()`](https://adamrsophie.github.io/z3r/reference/z3_strings.md)
  [`z3_str_le()`](https://adamrsophie.github.io/z3r/reference/z3_strings.md)
  : String operations
- [`z3_seq_sort()`](https://adamrsophie.github.io/z3r/reference/z3_sequences.md)
  [`z3_re_sort()`](https://adamrsophie.github.io/z3r/reference/z3_sequences.md)
  [`z3_char_sort()`](https://adamrsophie.github.io/z3r/reference/z3_sequences.md)
  [`z3_sort_basis()`](https://adamrsophie.github.io/z3r/reference/z3_sequences.md)
  [`z3_seq_empty()`](https://adamrsophie.github.io/z3r/reference/z3_sequences.md)
  [`z3_seq_unit()`](https://adamrsophie.github.io/z3r/reference/z3_sequences.md)
  [`z3_seq_nth()`](https://adamrsophie.github.io/z3r/reference/z3_sequences.md)
  [`z3_last_indexof()`](https://adamrsophie.github.io/z3r/reference/z3_sequences.md)
  [`z3_replace_all()`](https://adamrsophie.github.io/z3r/reference/z3_sequences.md)
  [`z3_replace_re()`](https://adamrsophie.github.io/z3r/reference/z3_sequences.md)
  [`z3_replace_re_all()`](https://adamrsophie.github.io/z3r/reference/z3_sequences.md)
  [`z3_seq_power()`](https://adamrsophie.github.io/z3r/reference/z3_sequences.md)
  [`z3_seq_map()`](https://adamrsophie.github.io/z3r/reference/z3_sequences.md)
  [`z3_seq_mapi()`](https://adamrsophie.github.io/z3r/reference/z3_sequences.md)
  [`z3_seq_foldl()`](https://adamrsophie.github.io/z3r/reference/z3_sequences.md)
  [`z3_seq_foldli()`](https://adamrsophie.github.io/z3r/reference/z3_sequences.md)
  [`z3_re()`](https://adamrsophie.github.io/z3r/reference/z3_sequences.md)
  [`z3_in_re()`](https://adamrsophie.github.io/z3r/reference/z3_sequences.md)
  [`z3_re_star()`](https://adamrsophie.github.io/z3r/reference/z3_sequences.md)
  [`z3_re_plus()`](https://adamrsophie.github.io/z3r/reference/z3_sequences.md)
  [`z3_re_option()`](https://adamrsophie.github.io/z3r/reference/z3_sequences.md)
  [`z3_re_complement()`](https://adamrsophie.github.io/z3r/reference/z3_sequences.md)
  [`z3_re_union()`](https://adamrsophie.github.io/z3r/reference/z3_sequences.md)
  [`z3_re_concat()`](https://adamrsophie.github.io/z3r/reference/z3_sequences.md)
  [`z3_re_intersect()`](https://adamrsophie.github.io/z3r/reference/z3_sequences.md)
  [`z3_re_diff()`](https://adamrsophie.github.io/z3r/reference/z3_sequences.md)
  [`z3_re_loop()`](https://adamrsophie.github.io/z3r/reference/z3_sequences.md)
  [`z3_re_power()`](https://adamrsophie.github.io/z3r/reference/z3_sequences.md)
  [`z3_re_range()`](https://adamrsophie.github.io/z3r/reference/z3_sequences.md)
  [`z3_re_empty()`](https://adamrsophie.github.io/z3r/reference/z3_sequences.md)
  [`z3_re_full()`](https://adamrsophie.github.io/z3r/reference/z3_sequences.md)
  [`z3_re_allchar()`](https://adamrsophie.github.io/z3r/reference/z3_sequences.md)
  [`z3_char_val()`](https://adamrsophie.github.io/z3r/reference/z3_sequences.md)
  [`z3_char_to_int()`](https://adamrsophie.github.io/z3r/reference/z3_sequences.md)
  [`z3_char_to_bv()`](https://adamrsophie.github.io/z3r/reference/z3_sequences.md)
  [`z3_char_from_bv()`](https://adamrsophie.github.io/z3r/reference/z3_sequences.md)
  [`z3_char_is_digit()`](https://adamrsophie.github.io/z3r/reference/z3_sequences.md)
  [`z3_char_le()`](https://adamrsophie.github.io/z3r/reference/z3_sequences.md)
  [`z3_str_from_code()`](https://adamrsophie.github.io/z3r/reference/z3_sequences.md)
  [`z3_str_to_code()`](https://adamrsophie.github.io/z3r/reference/z3_sequences.md)
  : Sequences, regular expressions and characters

## Datatypes, functions and quantifiers

- [`z3_datatype()`](https://adamrsophie.github.io/z3r/reference/z3_datatype.md)
  [`z3_datatypes()`](https://adamrsophie.github.io/z3r/reference/z3_datatype.md)
  [`z3_datatype_of()`](https://adamrsophie.github.io/z3r/reference/z3_datatype.md)
  [`z3_enum_sort()`](https://adamrsophie.github.io/z3r/reference/z3_datatype.md)
  [`z3_tuple_sort()`](https://adamrsophie.github.io/z3r/reference/z3_datatype.md)
  [`z3_list_sort()`](https://adamrsophie.github.io/z3r/reference/z3_datatype.md)
  [`z3_tuple_fields()`](https://adamrsophie.github.io/z3r/reference/z3_datatype.md)
  [`z3_update_field()`](https://adamrsophie.github.io/z3r/reference/z3_datatype.md)
  [`z3_type_variable()`](https://adamrsophie.github.io/z3r/reference/z3_datatype.md)
  [`z3_datatype_instance()`](https://adamrsophie.github.io/z3r/reference/z3_datatype.md)
  [`z3_is_recursive_datatype()`](https://adamrsophie.github.io/z3r/reference/z3_datatype.md)
  : Algebraic datatypes
- [`z3_function()`](https://adamrsophie.github.io/z3r/reference/z3_function.md)
  [`z3_fresh_function()`](https://adamrsophie.github.io/z3r/reference/z3_function.md)
  : Uninterpreted functions
- [`z3_rec_function()`](https://adamrsophie.github.io/z3r/reference/z3_rec_function.md)
  [`z3_rec_define()`](https://adamrsophie.github.io/z3r/reference/z3_rec_function.md)
  : Recursive functions
- [`z3_forall()`](https://adamrsophie.github.io/z3r/reference/z3_forall.md)
  [`z3_exists()`](https://adamrsophie.github.io/z3r/reference/z3_forall.md)
  [`z3_quantifier_db()`](https://adamrsophie.github.io/z3r/reference/z3_forall.md)
  [`z3_pattern()`](https://adamrsophie.github.io/z3r/reference/z3_forall.md)
  : Quantifiers
- [`z3_linear_order()`](https://adamrsophie.github.io/z3r/reference/z3_linear_order.md)
  [`z3_partial_order()`](https://adamrsophie.github.io/z3r/reference/z3_linear_order.md)
  [`z3_piecewise_linear_order()`](https://adamrsophie.github.io/z3r/reference/z3_linear_order.md)
  [`z3_tree_order()`](https://adamrsophie.github.io/z3r/reference/z3_linear_order.md)
  [`z3_transitive_closure()`](https://adamrsophie.github.io/z3r/reference/z3_linear_order.md)
  : Special relations

## Theories of Z3 5

- [`z3_complex_sort()`](https://adamrsophie.github.io/z3r/reference/z3_v5_theories.md)
  [`z3_complex()`](https://adamrsophie.github.io/z3r/reference/z3_v5_theories.md)
  [`z3_real_part()`](https://adamrsophie.github.io/z3r/reference/z3_v5_theories.md)
  [`z3_imag_part()`](https://adamrsophie.github.io/z3r/reference/z3_v5_theories.md)
  [`z3_ff_sort()`](https://adamrsophie.github.io/z3r/reference/z3_v5_theories.md)
  [`z3_ff_size()`](https://adamrsophie.github.io/z3r/reference/z3_v5_theories.md)
  [`z3_ff_add()`](https://adamrsophie.github.io/z3r/reference/z3_v5_theories.md)
  [`z3_ff_mul()`](https://adamrsophie.github.io/z3r/reference/z3_v5_theories.md)
  [`z3_ff_neg()`](https://adamrsophie.github.io/z3r/reference/z3_v5_theories.md)
  [`z3_ff_bitsum()`](https://adamrsophie.github.io/z3r/reference/z3_v5_theories.md)
  [`z3_finite_set_sort()`](https://adamrsophie.github.io/z3r/reference/z3_v5_theories.md)
  [`z3_fs_element_sort()`](https://adamrsophie.github.io/z3r/reference/z3_v5_theories.md)
  [`z3_fs_empty()`](https://adamrsophie.github.io/z3r/reference/z3_v5_theories.md)
  [`z3_fs_singleton()`](https://adamrsophie.github.io/z3r/reference/z3_v5_theories.md)
  [`z3_fs_union()`](https://adamrsophie.github.io/z3r/reference/z3_v5_theories.md)
  [`z3_fs_intersect()`](https://adamrsophie.github.io/z3r/reference/z3_v5_theories.md)
  [`z3_fs_difference()`](https://adamrsophie.github.io/z3r/reference/z3_v5_theories.md)
  [`z3_fs_member()`](https://adamrsophie.github.io/z3r/reference/z3_v5_theories.md)
  [`z3_fs_subset()`](https://adamrsophie.github.io/z3r/reference/z3_v5_theories.md)
  [`z3_fs_size()`](https://adamrsophie.github.io/z3r/reference/z3_v5_theories.md)
  [`z3_fs_range()`](https://adamrsophie.github.io/z3r/reference/z3_v5_theories.md)
  [`z3_fs_map()`](https://adamrsophie.github.io/z3r/reference/z3_v5_theories.md)
  [`z3_fs_filter()`](https://adamrsophie.github.io/z3r/reference/z3_v5_theories.md)
  : Complex numbers, finite fields and finite sets (Z3 5.x)

## Tactics and simplifiers

- [`z3_tactic()`](https://adamrsophie.github.io/z3r/reference/z3_tactic.md)
  [`z3_tactic_then()`](https://adamrsophie.github.io/z3r/reference/z3_tactic.md)
  [`z3_tactic_or_else()`](https://adamrsophie.github.io/z3r/reference/z3_tactic.md)
  [`z3_tactic_repeat()`](https://adamrsophie.github.io/z3r/reference/z3_tactic.md)
  [`z3_tactic_try_for()`](https://adamrsophie.github.io/z3r/reference/z3_tactic.md)
  [`z3_tactics()`](https://adamrsophie.github.io/z3r/reference/z3_tactic.md)
  [`z3_solver_from_tactic()`](https://adamrsophie.github.io/z3r/reference/z3_tactic.md)
  [`z3_goal()`](https://adamrsophie.github.io/z3r/reference/z3_tactic.md)
  [`z3_goal_formulas()`](https://adamrsophie.github.io/z3r/reference/z3_tactic.md)
  [`z3_goal_status()`](https://adamrsophie.github.io/z3r/reference/z3_tactic.md)
  [`z3_apply_tactic()`](https://adamrsophie.github.io/z3r/reference/z3_tactic.md)
  : Tactics and goals
- [`z3_probe()`](https://adamrsophie.github.io/z3r/reference/z3_probe.md)
  [`z3_probes()`](https://adamrsophie.github.io/z3r/reference/z3_probe.md)
  [`z3_probe_apply()`](https://adamrsophie.github.io/z3r/reference/z3_probe.md)
  [`z3_tactic_when()`](https://adamrsophie.github.io/z3r/reference/z3_probe.md)
  [`z3_tactic_cond()`](https://adamrsophie.github.io/z3r/reference/z3_probe.md)
  [`z3_tactic_fail_if()`](https://adamrsophie.github.io/z3r/reference/z3_probe.md)
  [`z3_tactic_skip()`](https://adamrsophie.github.io/z3r/reference/z3_probe.md)
  [`z3_tactic_fail()`](https://adamrsophie.github.io/z3r/reference/z3_probe.md)
  [`z3_tactic_fail_if_not_decided()`](https://adamrsophie.github.io/z3r/reference/z3_probe.md)
  [`z3_tactic_par_or()`](https://adamrsophie.github.io/z3r/reference/z3_probe.md)
  [`z3_tactic_par_then()`](https://adamrsophie.github.io/z3r/reference/z3_probe.md)
  [`z3_tactic_with()`](https://adamrsophie.github.io/z3r/reference/z3_probe.md)
  [`z3_tactic_params()`](https://adamrsophie.github.io/z3r/reference/z3_probe.md)
  : Probes and conditional tactics
- [`z3_goal_info()`](https://adamrsophie.github.io/z3r/reference/z3_goal_info.md)
  [`z3_goal_reset()`](https://adamrsophie.github.io/z3r/reference/z3_goal_info.md)
  [`z3_goal_convert_model()`](https://adamrsophie.github.io/z3r/reference/z3_goal_info.md)
  : Goal properties
- [`z3_simplifier()`](https://adamrsophie.github.io/z3r/reference/z3_simplifier.md)
  [`z3_simplifiers()`](https://adamrsophie.github.io/z3r/reference/z3_simplifier.md)
  [`z3_simplifier_then()`](https://adamrsophie.github.io/z3r/reference/z3_simplifier.md)
  [`z3_simplifier_with()`](https://adamrsophie.github.io/z3r/reference/z3_simplifier.md)
  [`z3_solver_add_simplifier()`](https://adamrsophie.github.io/z3r/reference/z3_simplifier.md)
  : Simplifiers

## Horn clauses

- [`z3_fixedpoint()`](https://adamrsophie.github.io/z3r/reference/z3_fixedpoint.md)
  [`z3_register_relation()`](https://adamrsophie.github.io/z3r/reference/z3_fixedpoint.md)
  [`z3_add_rule()`](https://adamrsophie.github.io/z3r/reference/z3_fixedpoint.md)
  [`z3_add_fact()`](https://adamrsophie.github.io/z3r/reference/z3_fixedpoint.md)
  [`z3_query()`](https://adamrsophie.github.io/z3r/reference/z3_fixedpoint.md)
  [`z3_answer()`](https://adamrsophie.github.io/z3r/reference/z3_fixedpoint.md)
  [`z3_ground_answer()`](https://adamrsophie.github.io/z3r/reference/z3_fixedpoint.md)
  [`z3_answer_trace()`](https://adamrsophie.github.io/z3r/reference/z3_fixedpoint.md)
  [`z3_answer_rule_names()`](https://adamrsophie.github.io/z3r/reference/z3_fixedpoint.md)
  [`z3_cover()`](https://adamrsophie.github.io/z3r/reference/z3_fixedpoint.md)
  [`z3_add_cover()`](https://adamrsophie.github.io/z3r/reference/z3_fixedpoint.md)
  [`z3_num_levels()`](https://adamrsophie.github.io/z3r/reference/z3_fixedpoint.md)
  [`z3_add_invariant()`](https://adamrsophie.github.io/z3r/reference/z3_fixedpoint.md)
  [`z3_reachable()`](https://adamrsophie.github.io/z3r/reference/z3_fixedpoint.md)
  [`z3_fixedpoint_rules()`](https://adamrsophie.github.io/z3r/reference/z3_fixedpoint.md)
  [`z3_fixedpoint_assertions()`](https://adamrsophie.github.io/z3r/reference/z3_fixedpoint.md)
  [`z3_set_representation()`](https://adamrsophie.github.io/z3r/reference/z3_fixedpoint.md)
  : Horn clauses and Datalog: the fixedpoint engine
- [`z3_on_lemma()`](https://adamrsophie.github.io/z3r/reference/z3_on_lemma.md)
  [`z3_fixedpoint_external()`](https://adamrsophie.github.io/z3r/reference/z3_on_lemma.md)
  : Fixedpoint callbacks
- [`z3_relation_columns()`](https://adamrsophie.github.io/z3r/reference/z3_relation_columns.md)
  : Relation sorts
- [`z3_model_project()`](https://adamrsophie.github.io/z3r/reference/z3_model_project.md)
  [`z3_model_extrapolate()`](https://adamrsophie.github.io/z3r/reference/z3_model_project.md)
  [`z3_qe_lite()`](https://adamrsophie.github.io/z3r/reference/z3_model_project.md)
  : Model-based projection and light quantifier elimination

## Algebraic numbers

- [`z3_algebraic()`](https://adamrsophie.github.io/z3r/reference/z3_algebraic.md)
  [`z3_algebraic_root()`](https://adamrsophie.github.io/z3r/reference/z3_algebraic.md)
  [`z3_algebraic_power()`](https://adamrsophie.github.io/z3r/reference/z3_algebraic.md)
  [`z3_algebraic_sign()`](https://adamrsophie.github.io/z3r/reference/z3_algebraic.md)
  [`z3_algebraic_poly()`](https://adamrsophie.github.io/z3r/reference/z3_algebraic.md)
  [`z3_algebraic_roots()`](https://adamrsophie.github.io/z3r/reference/z3_algebraic.md)
  [`z3_algebraic_eval()`](https://adamrsophie.github.io/z3r/reference/z3_algebraic.md)
  [`z3_subresultants()`](https://adamrsophie.github.io/z3r/reference/z3_algebraic.md)
  [`z3_algebraic_bounds()`](https://adamrsophie.github.io/z3r/reference/z3_algebraic.md)
  : Algebraic number arithmetic
- [`z3_rcf()`](https://adamrsophie.github.io/z3r/reference/z3_rcf.md)
  [`z3_rcf_pi()`](https://adamrsophie.github.io/z3r/reference/z3_rcf.md)
  [`z3_rcf_e()`](https://adamrsophie.github.io/z3r/reference/z3_rcf.md)
  [`z3_rcf_infinitesimal()`](https://adamrsophie.github.io/z3r/reference/z3_rcf.md)
  [`z3_rcf_roots()`](https://adamrsophie.github.io/z3r/reference/z3_rcf.md)
  [`z3_rcf_decimal()`](https://adamrsophie.github.io/z3r/reference/z3_rcf.md)
  [`z3_rcf_fraction()`](https://adamrsophie.github.io/z3r/reference/z3_rcf.md)
  [`z3_rcf_info()`](https://adamrsophie.github.io/z3r/reference/z3_rcf.md)
  [`format(`*`<z3_rcf>`*`)`](https://adamrsophie.github.io/z3r/reference/z3_rcf.md)
  : Real closed fields

## Expressions as data

- [`z3_kind()`](https://adamrsophie.github.io/z3r/reference/z3_inspect.md)
  [`z3_decl()`](https://adamrsophie.github.io/z3r/reference/z3_inspect.md)
  [`z3_args()`](https://adamrsophie.github.io/z3r/reference/z3_inspect.md)
  [`z3_num_args()`](https://adamrsophie.github.io/z3r/reference/z3_inspect.md)
  [`z3_decl_info()`](https://adamrsophie.github.io/z3r/reference/z3_inspect.md)
  [`z3_is_const()`](https://adamrsophie.github.io/z3r/reference/z3_inspect.md)
  [`z3_is_numeral()`](https://adamrsophie.github.io/z3r/reference/z3_inspect.md)
  [`z3_is_app()`](https://adamrsophie.github.io/z3r/reference/z3_inspect.md)
  [`z3_is_quantifier()`](https://adamrsophie.github.io/z3r/reference/z3_inspect.md)
  [`z3_quantifier_info()`](https://adamrsophie.github.io/z3r/reference/z3_inspect.md)
  [`z3_var_index()`](https://adamrsophie.github.io/z3r/reference/z3_inspect.md)
  [`z3_bound_var()`](https://adamrsophie.github.io/z3r/reference/z3_inspect.md)
  [`z3_apply()`](https://adamrsophie.github.io/z3r/reference/z3_inspect.md)
  : Inspect the structure of an expression
- [`z3_substitute()`](https://adamrsophie.github.io/z3r/reference/z3_substitute.md)
  [`z3_substitute_vars()`](https://adamrsophie.github.io/z3r/reference/z3_substitute.md)
  [`z3_substitute_funs()`](https://adamrsophie.github.io/z3r/reference/z3_substitute.md)
  : Substitution
- [`z3_update_args()`](https://adamrsophie.github.io/z3r/reference/z3_update_args.md)
  : Rebuild an application with new arguments
- [`z3_simplify()`](https://adamrsophie.github.io/z3r/reference/z3_simplify.md)
  : Simplify an expression
- [`z3_simplify_help()`](https://adamrsophie.github.io/z3r/reference/z3_simplify_help.md)
  : Simplify with parameters
- [`z3_identical()`](https://adamrsophie.github.io/z3r/reference/z3_identical.md)
  : Structural equality of expressions
- [`z3_ast_id()`](https://adamrsophie.github.io/z3r/reference/z3_ast_id.md)
  [`z3_ast_hash()`](https://adamrsophie.github.io/z3r/reference/z3_ast_id.md)
  [`z3_is_well_sorted()`](https://adamrsophie.github.io/z3r/reference/z3_ast_id.md)
  [`z3_is_ground()`](https://adamrsophie.github.io/z3r/reference/z3_ast_id.md)
  [`z3_depth()`](https://adamrsophie.github.io/z3r/reference/z3_ast_id.md)
  : Expression identity and properties
- [`z3_ast_map()`](https://adamrsophie.github.io/z3r/reference/z3_ast_map.md)
  [`z3_ast_map_insert()`](https://adamrsophie.github.io/z3r/reference/z3_ast_map.md)
  [`z3_ast_map_get()`](https://adamrsophie.github.io/z3r/reference/z3_ast_map.md)
  [`z3_ast_map_contains()`](https://adamrsophie.github.io/z3r/reference/z3_ast_map.md)
  [`z3_ast_map_erase()`](https://adamrsophie.github.io/z3r/reference/z3_ast_map.md)
  [`z3_ast_map_reset()`](https://adamrsophie.github.io/z3r/reference/z3_ast_map.md)
  [`z3_ast_map_keys()`](https://adamrsophie.github.io/z3r/reference/z3_ast_map.md)
  [`z3_ast_map_values()`](https://adamrsophie.github.io/z3r/reference/z3_ast_map.md)
  [`length(`*`<z3_ast_map>`*`)`](https://adamrsophie.github.io/z3r/reference/z3_ast_map.md)
  [`format(`*`<z3_ast_map>`*`)`](https://adamrsophie.github.io/z3r/reference/z3_ast_map.md)
  : Dictionaries keyed by Z3 terms
- [`z3_format_terms()`](https://adamrsophie.github.io/z3r/reference/z3_format_terms.md)
  : Z3's text for a list of terms
- [`z3_to_smtlib()`](https://adamrsophie.github.io/z3r/reference/z3_to_smtlib.md)
  [`z3_from_smtlib()`](https://adamrsophie.github.io/z3r/reference/z3_to_smtlib.md)
  [`z3_read_smtlib()`](https://adamrsophie.github.io/z3r/reference/z3_to_smtlib.md)
  [`z3_add_smtlib()`](https://adamrsophie.github.io/z3r/reference/z3_to_smtlib.md)
  [`z3_eval_smtlib()`](https://adamrsophie.github.io/z3r/reference/z3_to_smtlib.md)
  : SMT-LIB2 import and export
- [`z3_parser()`](https://adamrsophie.github.io/z3r/reference/z3_parser.md)
  [`z3_parser_add()`](https://adamrsophie.github.io/z3r/reference/z3_parser.md)
  [`z3_parse()`](https://adamrsophie.github.io/z3r/reference/z3_parser.md)
  : SMT-LIB2 parsers with shared declarations
- [`z3_from_symbolic()`](https://adamrsophie.github.io/z3r/reference/z3_from_symbolic.md)
  [`z3_to_symbolic()`](https://adamrsophie.github.io/z3r/reference/z3_from_symbolic.md)
  : Convert between symcompR and Z3 expressions

## Callbacks

- [`z3_propagator()`](https://adamrsophie.github.io/z3r/reference/z3_propagator.md)
  [`z3_propagate_register()`](https://adamrsophie.github.io/z3r/reference/z3_propagator.md)
  [`z3_propagator_function()`](https://adamrsophie.github.io/z3r/reference/z3_propagator.md)
  [`z3_propagate_consequence()`](https://adamrsophie.github.io/z3r/reference/z3_propagator.md)
  [`z3_propagate_conflict()`](https://adamrsophie.github.io/z3r/reference/z3_propagator.md)
  [`z3_propagate_register_cb()`](https://adamrsophie.github.io/z3r/reference/z3_propagator.md)
  [`z3_next_split()`](https://adamrsophie.github.io/z3r/reference/z3_propagator.md)
  [`z3_on_clause()`](https://adamrsophie.github.io/z3r/reference/z3_propagator.md)
  [`z3_on_model()`](https://adamrsophie.github.io/z3r/reference/z3_propagator.md)
  [`z3_set_proof_checker()`](https://adamrsophie.github.io/z3r/reference/z3_propagator.md)
  : User propagators
