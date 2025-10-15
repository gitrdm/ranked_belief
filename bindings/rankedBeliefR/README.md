# rankedBeliefR

R bindings for the `ranked_belief` lazy ranking library. The package links
against the C interoperability layer that ships with the repository. It is
a PoC wrapper to test the ffi of the library for use in R.

## Prerequisites

1. Configure and build the project targets so that the C API archive is
   available. From the repository root this can be done via:

```sh
make build
```

   This produces `build/libranked_belief_c_api.a` which the R package links
   against.

2. Ensure your `R_HOME` toolchain can find an appropriate C++20 compiler.

## Building the package locally

The package is not on CRAN; it is intended to be installed from source inside
this monorepo. You can install it with:

```r
old <- Sys.getenv("RANKED_BELIEF_ROOT")
Sys.setenv(RANKED_BELIEF_ROOT = getwd())
remotes::install_local("bindings/rankedBeliefR")
Sys.setenv(RANKED_BELIEF_ROOT = old)
```

Alternatively, build a tarball via `R CMD build bindings/rankedBeliefR` (with
`RANKED_BELIEF_ROOT` exported) and install that artefact.

## Usage

```r
library(rankedBeliefR)

r <- rb_from_array_int(values = 1:3, ranks = c(0, 1, 2))
rb_take_n_int(r, 2)
rb_first_int(r)
rb_is_empty(r)
rb_free(r)
```

The handles returned by the package are external pointers that free underlying
resources when garbage-collected. `rb_free()` can be used to release them
explicitly.

## Documentation

The package includes three vignettes demonstrating different use cases:

1. **Introduction to rankedBeliefR** - Basic operations and API overview
2. **Boolean Circuit Diagnosis** - Practical fault diagnosis example  
3. **Recursive Rankings and Lazy Evaluation** - Working with infinite structures

After installing the package, build the vignettes with:

```bash
cd bindings/rankedBeliefR
Rscript build_vignettes.R
```

Then open the HTML files in `vignettes/` directory. See `vignettes/README.md` for details.

Additional examples are available in `inst/examples/`.

## Notes

- Only the integer-valued subset of the C API is currently wrapped. Extending
  coverage is straightforward by following the existing patterns.
- R does not have a native 64-bit integer type; ranks are returned as numeric
  vectors. Large ranks (above `2^53`) may not round-trip exactly.
- The package assumes it is built inside the repository so that `../..` points
  to the project root. Adjust `src/Makevars` if you relocate the package.
