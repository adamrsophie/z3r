// How z3r was built and which Z3 it runs with, for z3_build_info().
#include "z3r.h"
#include "z3r_build.h"

using namespace z3r;

extern "C" const char* z3r_loaded_library(void);

// [[Rcpp::export(rng = false)]]
Rcpp::List cpp_build_info() {
  using Rcpp::Named;
#if defined(__VERSION__)
  const char* compiler = __VERSION__;
#elif defined(_MSC_VER)
  const char* compiler = "MSVC";
#else
  const char* compiler = "";
#endif
  return Rcpp::List::create(Named("provider") = Z3R_BUILD_PROVIDER, Named("source") = Z3R_BUILD_SOURCE,
                            Named("asset") = Z3R_BUILD_ASSET, Named("sha256") = Z3R_BUILD_SHA256,
                            Named("library") = std::string(z3r_loaded_library()),
                            Named("compiler") = compiler);
}
