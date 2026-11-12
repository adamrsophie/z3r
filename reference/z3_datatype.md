# Algebraic datatypes

Declare new sorts by their constructors, as in functional languages:
enumerations, records, lists, trees.

## Usage

``` r
z3_datatype(ctx, name, ..., parameters = NULL)

z3_datatypes(ctx, specs)

z3_datatype_of(sort)

z3_enum_sort(ctx, name, values)

z3_tuple_sort(ctx, name, ...)

z3_list_sort(ctx, name, element)

z3_tuple_fields(sort)

z3_update_field(x, field, value)

z3_type_variable(ctx, name)

z3_datatype_instance(ctx, name, sort)

z3_is_recursive_datatype(sort)
```

## Arguments

- ctx:

  A
  [`z3_context()`](https://adamrsophie.github.io/z3r/reference/z3_context.md).

- name:

  Name of the datatype.

- ...:

  Constructors, each a named list of field sorts. For `z3_tuple_sort()`,
  the named field sorts.

- parameters:

  Optional list of type variables.

- specs:

  For `z3_datatypes()`: a named list whose elements are lists of
  constructors as in `...`.

- sort:

  A datatype sort (or a `z3_datatype`), or sorts for the instance.

- values:

  Names of the enumeration values.

- element:

  Element sort of the list (or a `z3_datatype`).

- x:

  A value of a datatype.

- field:

  A field accessor (an element of the `z3_datatype`).

- value:

  New value of the field.

## Value

A `z3_datatype` (a named list of datatypes for `z3_datatypes()`), a sort
for `z3_type_variable()` and a `z3_expr` for `z3_update_field()`.
`z3_tuple_fields()` returns a list with `constructor`, a `z3_function`
building tuples, and `fields`, a named list of accessor functions.

## Details

`z3_datatype(ctx, name, ...)` takes one argument per constructor. Each
is a named list of fields, mapping field names to sorts; an empty list
makes a constructor without fields. A field may refer to the datatype
being declared by giving its name as a string, which makes the datatype
recursive. `z3_datatypes()` declares several datatypes at once, so that
they can refer to each other (mutual recursion).

The result is a `z3_datatype` with:

- `$sort`: the new sort;

- one element per constructor: a function building values (or, for a
  constructor without fields, the value itself);

- `$is_<constructor>`: recognizers, true when a value was built by that
  constructor;

- one element per field: accessor functions.

`z3_enum_sort()`, `z3_tuple_sort()` and `z3_list_sort()` are shortcuts
for the common shapes. `z3_enum_sort()` and `z3_list_sort()` use Z3's
dedicated constructors; since Z3 4.12.0 an enumeration cannot reuse the
name of a datatype already declared in the context. `z3_tuple_sort()`
names its constructor `mk_<name>`. A datatype with a single constructor
that has the datatype's own name and no recursive fields, as in
`z3_datatype(ctx, "Point", Point = list(x = I, y = I))`, is declared as
a Z3 tuple sort. `z3_update_field()` returns a copy of a value with one
field changed. `z3_datatype_of()` rebuilds the object from a datatype
sort, for example one read from SMT-LIB2. `z3_tuple_fields()` reads a
tuple sort (a datatype with one constructor and no recursive fields):
its constructor and its field accessors, in order.

Polymorphic datatypes (Z3 4.15.4 and later) take `parameters`, a list of
type variables made with `z3_type_variable()` that fields may use as
sorts. `z3_datatype_instance()` instantiates one at concrete sorts.

## Examples

``` r
ctx <- z3_context()
I <- z3_int_sort(ctx)
Tree <- z3_datatype(ctx, "Tree",
  leaf = list(),
  node = list(left = "Tree", value = I, right = "Tree"))
t <- z3_const(ctx, "t", Tree$sort)
s <- z3_solver(ctx)
z3_add(s, Tree$is_node(t), Tree$value(t) == 7, Tree$is_node(Tree$left(t)))
z3_check(s)
#> [1] "sat"
z3_eval(z3_model(s), t)
#> <z3_expr Tree> (node (node leaf 2 leaf) 7 leaf)

Color <- z3_enum_sort(ctx, "Color", c("red", "green", "blue"))
c1 <- z3_const(ctx, "c1", Color$sort)
s <- z3_solver(ctx)
z3_add(s, c1 != Color$red, c1 != Color$green)
z3_check(s)
#> [1] "sat"
z3_eval(z3_model(s), c1)
#> <z3_expr Color> blue

Point <- z3_datatype(ctx, "Point", Point = list(x = I, y = I))
tf <- z3_tuple_fields(Point)
names(tf$fields)
#> [1] "x" "y"
z3_simplify(tf$fields$y(tf$constructor(1, 2)))
#> <z3_expr Int> 2
```
