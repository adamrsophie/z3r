# Relation sorts

The Datalog engine represents a whole relation as a term of a relation
sort
([`z3_sort_kind()`](https://adamrsophie.github.io/z3r/reference/z3_sort.md)
`"relation"`), whose columns are the sorts of the relation's arguments.
Such terms reach R as arguments of the callbacks of
[`z3_fixedpoint_external()`](https://adamrsophie.github.io/z3r/reference/z3_on_lemma.md).
`z3_relation_columns()` lists the column sorts (`Z3_get_relation_arity`,
`Z3_get_relation_column`); the arity is the length of the list.

## Usage

``` r
z3_relation_columns(sort)
```

## Arguments

- sort:

  A relation sort, for example `z3_sort(args[[1]])` inside an external
  relation callback.

## Value

A list of `z3_sort` objects, one per column.
