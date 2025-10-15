#!/usr/bin/env Rscript
# Build all vignettes for rankedBeliefR
# Run this AFTER installing the package

library(rmarkdown)

cat("Building rankedBeliefR vignettes...\n\n")

vignettes <- c(
  "introduction",
  "boolean-circuit", 
  "recursive-rankings"
)

for (vig in vignettes) {
  cat(sprintf("Building %s.Rmd...\n", vig))
  rmd_file <- file.path("vignettes", paste0(vig, ".Rmd"))
  
  # Temporarily enable evaluation
  content <- readLines(rmd_file)
  content <- gsub("eval = FALSE", "eval = TRUE", content, fixed = TRUE)
  temp_file <- tempfile(fileext = ".Rmd")
  writeLines(content, temp_file)
  
  tryCatch({
    render(temp_file, 
           output_dir = "vignettes",
           output_file = paste0(vig, ".html"))
    cat(sprintf("✓ %s.html created\n\n", vig))
  }, error = function(e) {
    cat(sprintf("✗ Error building %s: %s\n\n", vig, e$message))
  })
  
  unlink(temp_file)
}

cat("Done! Vignettes are in vignettes/ directory.\n")
cat("Open them in a browser to view.\n")
