# Recursive ranking example illustrating infinite lazy structures using R bindings

recursive_fun <- function(value) {
  # returns a ranking: normal value, exceptional recursive doubling
  rb_merge_int(rb_singleton_int(as.integer(value)), rb_map_int(rb_singleton_int(as.integer(value * 2)), function(x) x))
}

first_values <- function(count = 10) {
  ranking <- recursive_fun(1)
  rb_take_n_int(ranking, count)
}

if (interactive()) {
  print(first_values(10))
}
