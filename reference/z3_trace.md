# Z3 trace messages

`z3_trace(tag)` turns on Z3's internal trace messages for a tag, such as
`"arith"` or `"spacer"`, and `z3_trace(tag, FALSE)` turns them off again
(`Z3_enable_trace`, `Z3_disable_trace`). Only debug builds of Z3 trace;
they write the messages to the file `.z3-trace` in the working
directory. Release builds, which include the usual binary distributions
of Z3, ignore both calls.

## Usage

``` r
z3_trace(tag, enable = TRUE)
```

## Arguments

- tag:

  Trace tags, a character vector.

- enable:

  `TRUE` to turn the tags on, `FALSE` to turn them off.

## Value

`tag`, invisibly.

## Examples

``` r
z3_trace("arith")
z3_trace("arith", enable = FALSE)
```
