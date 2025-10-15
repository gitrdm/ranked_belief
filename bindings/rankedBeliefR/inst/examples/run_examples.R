# Run all examples
#
# This script sources all example R files and runs them

# Ensure we can find the library
library(rankedBeliefR)

# Get example directory
example_dir <- system.file("examples", package = "rankedBeliefR")
if (example_dir == "") {
  # Running from source tree
  example_dir <- getwd()
}

cat("Running examples from:", example_dir, "\n\n")

# List of examples
examples <- c(
  "boolean_circuit.R",
  "recursion.R",
  "hidden_markov.R",
  "localisation.R",
  "ranking_network.R",
  "spelling_correction.R",
  "ranked_let.R",
  "ranked_procedure_call.R"
)

# Run each example
for (example in examples) {
  example_path <- file.path(example_dir, example)
  
  if (!file.exists(example_path)) {
    cat(sprintf("SKIP: %s (not found)\n\n", example))
    next
  }
  
  cat(sprintf("=== Running %s ===\n", example))
  tryCatch({
    source(example_path, chdir = TRUE)
    cat("\n")
  }, error = function(e) {
    cat(sprintf("ERROR: %s\n%s\n\n", example, conditionMessage(e)))
  })
}

cat("Done.\n")
