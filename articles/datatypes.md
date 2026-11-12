# Algebraic datatypes: enumerations, records, lists and trees

``` r

library(z3r)
ctx <- z3_context()
I <- z3_int_sort(ctx)
```

Algebraic datatypes declare new sorts by their constructors, as in
functional languages. Z3 knows that values built by different
constructors differ, that constructors are injective, and that recursive
values are finite.

## Enumerations

``` r

Color <- z3_enum_sort(ctx, "Color", c("red", "green", "blue"))
Color
#> <z3_datatype Color>
#>   red
#>   green
#>   blue
c1 <- z3_const(ctx, "c1", Color$sort)
s <- z3_solver(ctx)
z3_add(s, c1 != Color$red, c1 != Color$green)
z3_check(s)
#> [1] "sat"
z3_eval(z3_model(s), c1)
#> <z3_expr Color> blue
```

Four distinct colors cannot exist:

``` r

k <- lapply(1:4, function(i) z3_const(ctx, paste0("k", i), Color$sort))
s <- z3_solver(ctx)
z3_add(s, z3_distinct(k))
z3_check(s)
#> [1] "unsat"
```

## Map coloring

Enumerations make constraint problems readable. Color the regions of a
map so that neighbors differ:

``` r

regions <- c("A", "B", "C", "D", "E")
borders <- list(c("A", "B"), c("A", "C"), c("B", "C"), c("B", "D"), c("C", "D"), c("D", "E"))
col <- stats::setNames(lapply(regions, function(r) z3_const(ctx, r, Color$sort)), regions)
s <- z3_solver(ctx)
for (b in borders) z3_add(s, col[[b[1]]] != col[[b[2]]])
z3_check(s)
#> [1] "sat"
m <- z3_model(s)
vapply(col, function(v) format(z3_eval(m, v)), "")
#>       A       B       C       D       E 
#>  "blue" "green"   "red"  "blue" "green"
```

## Records (tuples)

``` r

Point <- z3_tuple_sort(ctx, "Point", x = I, y = I)
p <- Point$mk_Point(3, 4)
z3_simplify(Point$x(p))
#> <z3_expr Int> 3
z3_simplify(Point$y(z3_update_field(p, Point$y, 10)))
#> <z3_expr Int> 10
```

## Lists

``` r

L <- z3_list_sort(ctx, "IntList", I)
l <- L$cons(1, L$cons(2, L$cons(3, L$nil)))
l
#> <z3_expr IntList> (cons 1 (cons 2 (cons 3 nil)))
z3_simplify(L$head(L$tail(l)))
#> <z3_expr Int> 2
xs <- z3_const(ctx, "xs", L$sort)
s <- z3_solver(ctx)
z3_add(s, L$is_cons(xs), L$head(xs) == 7, L$tail(xs) == L$cons(8, L$nil))
z3_check(s)
#> [1] "sat"
z3_eval(z3_model(s), xs)
#> <z3_expr IntList> (cons 7 (cons 8 nil))
```

## Recursive trees

A field may name the datatype being declared:

``` r

Tree <- z3_datatype(ctx, "Tree",
  leaf = list(),
  node = list(left = "Tree", value = I, right = "Tree"))
Tree
#> <z3_datatype Tree>
#>   leaf
#>   node(left: Tree, value: Int, right: Tree)
t <- z3_const(ctx, "t", Tree$sort)
s <- z3_solver(ctx)
z3_add(s, Tree$is_node(t), Tree$value(t) == 10,
       Tree$is_node(Tree$left(t)), Tree$value(Tree$left(t)) < Tree$value(t))
z3_check(s)
#> [1] "sat"
z3_eval(z3_model(s), t)
#> <z3_expr Tree> (node (node leaf 0 leaf) 10 leaf)
```

There are no infinite trees, so a tree cannot contain itself:

``` r

s <- z3_solver(ctx)
z3_add(s, t == Tree$node(t, 1, Tree$leaf))
z3_check(s)
#> [1] "unsat"
```

Combined with recursive functions
([`vignette("functions-quantifiers")`](https://adamrsophie.github.io/z3r/articles/functions-quantifiers.md)),
datatypes support reasoning about recursive programs: for example, the
size or depth of a tree.

## Mutually recursive datatypes

``` r

dts <- z3_datatypes(ctx, list(
  Rose = list(rose = list(label = I, kids = "Forest")),
  Forest = list(empty = list(), grow = list(first = "Rose", rest = "Forest"))
))
dts$Forest
#> <z3_datatype Forest>
#>   empty
#>   grow(first: Rose, rest: Forest)
```

## Polymorphic datatypes

From Z3 4.15.4 a datatype can take sort parameters:

``` r

A <- z3_type_variable(ctx, "A")
Box <- z3_datatype(ctx, "Box", box = list(content = A), parameters = list(A))
BoxInt <- z3_datatype_instance(ctx, "Box", I)
b <- z3_const(ctx, "b", BoxInt$sort)
s <- z3_solver(ctx)
z3_add(s, BoxInt$content(b) == 5)
z3_check(s)
#> [1] "sat"
z3_eval(z3_model(s), b)
#> <z3_expr (Box Int)> (box 5)
```
