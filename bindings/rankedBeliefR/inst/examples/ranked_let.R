# Ranked let / procedural examples (illustrative)

ranked_let_demo <- function() {
  a <- rb_from_array_int(values = as.integer(c(1L,2L)), ranks = c(0,1))
  b <- rb_from_array_int(values = as.integer(c(3L,4L)), ranks = c(0,1))
  merged <- rb_merge_int(a,b)
  rb_take_n_int(merged, 5)
}

run_example <- function() {
  cat("Running ranked_let demo\n")
  print(ranked_let_demo())
}

if (interactive()) run_example()
