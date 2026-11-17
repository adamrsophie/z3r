## Submission

This is a new submission.

## System requirements

z3r links to the Z3 theorem prover (libz3, version 4.8.13 or later), found
through `Z3_DIR`, `Z3_INCLUDE_DIR`/`Z3_LIB_DIR` or pkg-config. Debian and
Fedora ship it as libz3-dev and z3-devel. Nothing is downloaded during
installation unless the user sets `Z3R_DOWNLOAD=true` (or the build runs
on R-universe, which sets `MY_UNIVERSE`); then configure fetches one pinned
official Z3 release over https and verifies its SHA-256 checksum.

## Test environments

* GitHub Actions: Ubuntu 22.04 and 24.04 (R release and oldrel-1), macOS 15
  and Windows (R release), with official Z3 releases from 4.8.13 to 5.4.1;
  Debian testing with the distribution's libz3-dev; ASan and UBSan.
* A monthly workflow runs the tests against all 43 Z3 releases.

## R CMD check results

0 errors | 0 warnings | 1 note

* New submission.
