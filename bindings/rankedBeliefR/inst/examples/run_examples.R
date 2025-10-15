# Run all example scripts under inst/examples
examples <- list.files(system.file("examples", package = "rankedBeliefR"), full.names = TRUE)
for (ex in examples) {
  if (grepl("\\.R$", ex)) {
    cat("---- Running:", basename(ex), "----\n")
    tryCatch(source(ex), error = function(e) cat("Example failed:", conditionMessage(e), "\n"))
  }
}
