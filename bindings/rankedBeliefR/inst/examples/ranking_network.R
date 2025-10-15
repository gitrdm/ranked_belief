# Ranking network example (idiomatic R)

run_example <- function() {
  cat("Running ranking_network example (illustrative)\n")
  # Very small demo: compose a few priors and condition
  h <- rb_from_array_int(values = as.integer(c(0L,1L)), ranks = c(0,15))
  b <- rb_from_array_int(values = as.integer(c(0L,1L)), ranks = c(0,4))
  f <- rb_from_array_int(values = as.integer(c(1L,0L)), ranks = c(0,10))
  s <- rb_from_array_int(values = as.integer(c(0L,1L)), ranks = c(0,3))
  net <- rb_merge_int(h, rb_merge_int(b, rb_merge_int(f, s)))
  posterior <- rb_observe_value_int(net, 1L)
  print(rb_take_n_int(posterior, 8))
}

if (interactive()) run_example()
