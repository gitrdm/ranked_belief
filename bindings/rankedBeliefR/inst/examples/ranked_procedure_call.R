# Ranked procedure call demo (illustrative)

run_example <- function() {
  cat("Running ranked_procedure_call example (illustrative)\n")
  f <- function(x) rb_from_array_int(values = as.integer(c(x, x+1)), ranks = c(0,1))
  res <- rb_merge_int(f(1L), f(2L))
  print(rb_take_n_int(res, 6))
}

if (interactive()) run_example()
