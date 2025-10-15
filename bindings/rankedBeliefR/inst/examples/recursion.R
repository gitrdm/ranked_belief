# Recursive ranking example (full port from Python/Racket)
#
# Illustrates infinite lazy structures: 1@0, 2@1, 4@2, 8@3, 16@4, ...

library(rankedBeliefR)

recursive_fun <- function(value) {
  # Normally: value at rank 0
  # Exceptionally: recursive_fun(value * 2) at rank 1
  # Since R is eager, we enumerate a finite prefix
  max_depth <- 20
  
  values_vec <- integer(max_depth)
  ranks_vec <- numeric(max_depth)
  
  current_val <- value
  for (i in seq_len(max_depth)) {
    values_vec[i] <- current_val
    ranks_vec[i] <- i - 1  # rank starts at 0
    current_val <- current_val * 2L
  }
  
  rb_from_array_int(values = values_vec, ranks = ranks_vec)
}

first_values <- function(count = 10) {
  ranking <- recursive_fun(1L)
  df <- rb_take_n_int(ranking, count)
  rb_free(ranking)
  df
}

run_example <- function() {
  cat("Recursive ranking example (infinite lazy structure)\n\n")
  df <- first_values(10)
  cat("First 10 values from recursive_fun(1):\n")
  print(df)
}

if (!exists("SKIP_EXAMPLE_RUN")) run_example()
