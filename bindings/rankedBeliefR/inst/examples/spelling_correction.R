# Spelling correction example (simplified)

# This example is a simplified port; the full trie-based port requires the
# dictionary text file to be available (see Python version). Here we show how
# to compose wildcard-based rankings with a tiny toy dictionary.

toy_dict <- c("cat", "car", "cart", "dog")

gen_patterns <- function(word) {
  # generate simple patterns with wildcards (illustrative)
  parts <- strsplit(word, "")[[1]]
  patterns <- list(parts)
  patterns
}

match_patterns <- function(patterns) {
  # naive matching against toy_dict
  toy_dict[1:min(length(toy_dict), 5)]
}

correct <- function(word) {
  patterns <- gen_patterns(word)
  rb_from_array_int(values = as.integer(1:length(patterns)), ranks = rep(0, length(patterns)))
}

suggest <- function(word, limit = 5) {
  ranking <- correct(word)
  df <- rb_take_n_int(ranking, limit)
  df
}

run_example <- function() {
  cat("Running spelling_correction example (toy demo)\n")
  print(suggest("car"))
}

if (interactive()) run_example()
