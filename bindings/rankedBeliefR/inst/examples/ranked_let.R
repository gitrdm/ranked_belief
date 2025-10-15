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
  # Enumerate all (beer, peanuts) combinations with their ranks
  # beer: FALSE (0) rank 0, TRUE (1) rank 1
  # peanuts given beer=FALSE: always FALSE (0)
  # peanuts given beer=TRUE: TRUE (1) rank 0, FALSE (0) rank 1
  
  combinations <- list(
    list(beer = 0L, peanuts = 0L, rank = 0),  # beer=F rank 0, peanuts=F rank 0 -> 0+0=0
    list(beer = 1L, peanuts = 1L, rank = 1),  # beer=T rank 1, peanuts=T rank 0 -> 1+0=1
    list(beer = 1L, peanuts = 0L, rank = 2)   # beer=T rank 1, peanuts=F rank 1 -> 1+1=2
  )
  
  values <- integer(length(combinations))
  ranks <- numeric(length(combinations))
  
  for (i in seq_along(combinations)) {
    values[i] <- encode_pair(combinations[[i]]$beer, combinations[[i]]$peanuts)
    ranks[i] <- combinations[[i]]$rank
  }
  
  rb_from_array_int(values = values, ranks = ranks)
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
