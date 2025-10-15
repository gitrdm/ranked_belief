# Spelling correction example (full port from Racket)
#
# Simplified implementation using edit distance + dictionary lookup

library(rankedBeliefR)

# Read dictionary file
read_dictionary <- function(filename) {
  if (!file.exists(filename)) {
    # Try installed package path
    pkg_path <- system.file("examples", "google-10000-english-no-swears.txt", 
                            package = "rankedBeliefR")
    if (pkg_path != "") {
      filename <- pkg_path
    } else {
      stop("Dictionary file not found")
    }
  }
  
  words <- readLines(filename, warn = FALSE)
  tolower(words)
}

# Levenshtein distance
edit_distance <- function(s1, s2) {
  n <- nchar(s1)
  m <- nchar(s2)
  
  d <- matrix(0, nrow = n + 1, ncol = m + 1)
  d[1, ] <- 0:m
  d[, 1] <- 0:n
  
  for (i in 2:(n + 1)) {
    for (j in 2:(m + 1)) {
      cost <- if (substr(s1, i-1, i-1) == substr(s2, j-1, j-1)) 0 else 1
      d[i, j] <- min(
        d[i-1, j] + 1,      # deletion
        d[i, j-1] + 1,      # insertion
        d[i-1, j-1] + cost  # substitution
      )
    }
  }
  
  d[n + 1, m + 1]
}

# Encode word as integer hash (simple)
word_hash <- function(word) {
  chars <- utf8ToInt(word)
  sum(chars * (256 ^ seq_along(chars))) %% 2147483647L
}

# Generate ranked corrections
correct_word <- function(input, dict, max_candidates = 100) {
  input_lower <- tolower(input)
  
  # Compute edit distances
  distances <- sapply(dict[1:min(length(dict), max_candidates)], 
                     function(w) edit_distance(input_lower, w))
  
  # Sort by distance
  candidates <- dict[1:min(length(dict), max_candidates)]
  ord <- order(distances)
  
  values <- sapply(candidates[ord], word_hash)
  ranks <- distances[ord]
  
  rb_from_array_int(values = as.integer(values), ranks = ranks)
}

run_example <- function() {
  cat("Spelling correction example\n")
  
  dict_file <- "google-10000-english-no-swears.txt"
  if (!file.exists(dict_file)) {
    dict_file <- system.file("examples", "google-10000-english-no-swears.txt",
                            package = "rankedBeliefR")
  }
  
  if (dict_file == "" || !file.exists(dict_file)) {
    cat("Dictionary file not found, skipping example\n")
    return()
  }
  
  dictionary <- read_dictionary(dict_file)
  cat(sprintf("Loaded %d words from dictionary\n\n", length(dictionary)))
  
  input <- "switzerlandd"
  cat(sprintf("Correcting: '%s'\n", input))
  
  corrections <- correct_word(input, dictionary, max_candidates = 1000)
  samples <- rb_take_n_int(corrections, 5)
  
  cat("\nTop 5 corrections:\n")
  for (i in seq_len(nrow(samples))) {
    cat(sprintf("  [hash %d] (edit distance %g)\n", 
                samples$value[i], samples$rank[i]))
  }
}

if (!exists("SKIP_EXAMPLE_RUN")) run_example()
