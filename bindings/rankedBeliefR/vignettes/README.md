# rankedBeliefR Vignettes

This directory contains vignettes (tutorials) for the rankedBeliefR package.

## Available Vignettes

### 1. Introduction to rankedBeliefR (`introduction.Rmd`)
- Basic operations: creating, transforming, and combining rankings
- Memory management
- Dependent rankings example (beer and peanuts)
- Complete API reference

### 2. Boolean Circuit Diagnosis (`boolean-circuit.Rmd`)
- Practical application: fault diagnosis in a three-gate circuit
- Encoding complex scenarios efficiently
- Interpreting ranked beliefs for diagnosis
- Advantages over probabilistic approaches

### 3. Recursive Rankings and Lazy Evaluation (`recursive-rankings.Rmd`)
- Working with infinite structures
- Practical examples: retry logic with exponential backoff
- Combining and filtering infinite rankings
- Memory considerations and depth limits

## Building Vignettes

**Note:** Due to the package's dependency on the C API, vignettes cannot be built during
`R CMD build`. Instead, build them after installing the package.

### After Package Installation

Once the package is installed (see main README for installation instructions), build vignettes with:

```r
# Build all vignettes
setwd("bindings/rankedBeliefR")
library(rmarkdown)
render('vignettes/introduction.Rmd', output_dir='vignettes')
render('vignettes/boolean-circuit.Rmd', output_dir='vignettes')
render('vignettes/recursive-rankings.Rmd', output_dir='vignettes')
```

Or use the provided helper script:

```bash
cd bindings/rankedBeliefR
Rscript build_vignettes.R
```

## Accessing Vignettes

After installation with vignettes, access them via:

```r
# List all vignettes
vignette(package = "rankedBeliefR")

# Open specific vignette
vignette("introduction", package = "rankedBeliefR")
vignette("boolean-circuit", package = "rankedBeliefR")
vignette("recursive-rankings", package = "rankedBeliefR")
```

Or browse the HTML files directly in your browser.
