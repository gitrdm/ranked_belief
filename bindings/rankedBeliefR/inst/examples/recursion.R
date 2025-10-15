# Recursive ranking example illustrating infinite lazy structures (idiomatic R)

# Construct a lazy recursive ranking where each exceptional branch doubles the value
recursive_fun <- function(value) {
  # The prior is: value @ rank 0, and recursively recursive_fun(value*2) @ rank 1
  normal <- rb_singleton_int(as.integer(value))
  exceptional <- rb_map_int(rb_singleton_int(as.integer(value * 2)), function(x) x)
  rb_merge_int(normal, exceptional)
}

first_values <- function(count = 10) {
  ranking <- recursive_fun(1)
  df <- rb_take_n_int(ranking, count)
  df
}

run_example <- function() {
  cat("Running recursion example...\n")
  print(first_values(10))
}

if (interactive()) run_example()
