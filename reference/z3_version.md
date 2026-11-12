# Z3 library version

Reports the version of the Z3 shared library z3r is running against.

## Usage

``` r
z3_version()

z3_version_at_least(version)
```

## Arguments

- version:

  A version string such as `"4.12.2"`.

## Value

A list with elements `runtime` (the version of the loaded Z3 library,
major.minor.build.revision), `headers` (the version of the Z3 headers
z3r was compiled with) and `full` (Z3's full version string). z3r
refuses to load if the two versions differ.

For `z3_version_at_least()`, `TRUE` when the Z3 headers z3r was compiled
with are at least `version`, so that features needing that version are
available.

## Examples

``` r
z3_version()
#> $runtime
#> [1] "5.4.1.0"
#> 
#> $headers
#> [1] "5.4.1.0"
#> 
#> $full
#> [1] "5.4.1.0 145c0c51570a357cc558ce898ce77573ba9d4fbf 145c0c5"
#> 
z3_version_at_least("4.12.2")
#> [1] TRUE
```
