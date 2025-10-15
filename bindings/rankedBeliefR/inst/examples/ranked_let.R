# Ranked let example (full port from Racket)
#
# Demonstrates dependent ranked variables (beer and peanuts)

library(rankedBeliefR)

# Boolean to integer encoding
bool_to_int <- function(b) if (b) 1L else 0L

# Normally FALSE, exceptionally TRUE
nrm_exc_bool <- function() {
  rb_from_array_int(values = c(0L, 1L), ranks = c(0, 1))
}

# Dependent peanut consumption
peanuts_given_beer <- function(beer_val) {
  # If beer=TRUE (1), normally TRUE else FALSE
  # If beer=FALSE (0), always FALSE
  if (beer_val == 1L) {
    rb_from_array_int(values = c(1L, 0L), ranks = c(0, 1))
  } else {
    rb_singleton_int(0L)
  }
}

# Encode (beer, peanuts) as single integer: beer*2 + peanuts
encode_pair <- function(beer, peanuts) as.integer(beer * 2 + peanuts)
decode_pair <- function(val) c(val %/% 2, val %% 2)

# Ranked let: beer then peanuts
ranked_let_example <- function() {
  beer_dist <- nrm_exc_bool()
  
  # Use rb_merge_apply_int: for each beer value, generate peanuts distribution
  combined <- rb_merge_apply_int(beer_dist, function(beer_val) {
    peanuts_dist <- peanuts_given_beer(beer_val)
    
    # Map peanuts to (beer, peanuts) pairs
    rb_merge_apply_int(peanuts_dist, function(peanuts_val) {
      rb_singleton_int(encode_pair(beer_val, peanuts_val))
    })
  })
  
  combined
}

run_example <- function() {
  cat("Ranked let example (beer and peanuts)\n\n")
  
  result <- ranked_let_example()
  samples <- rb_take_n_int(result, 10)
  
  cat("Possible scenarios:\n")
  for (i in seq_len(nrow(samples))) {
    pair <- decode_pair(samples$value[i])
    beer_str <- if (pair[1] == 1) "TRUE" else "FALSE"
    peanuts_str <- if (pair[2] == 1) "TRUE" else "FALSE"
    cat(sprintf("  beer: %s, peanuts: %s (rank %g)\n", 
                beer_str, peanuts_str, samples$rank[i]))
  }
}

if (!exists("SKIP_EXAMPLE_RUN")) run_example()
