# Robot localisation example (full port from Python)
#
# Simplified 2D grid localisation using HMM-style updates

library(rankedBeliefR)

neighbours <- function(pos) {
  x <- pos[1]
  y <- pos[2]
  list(c(x-1, y), c(x+1, y), c(x, y-1), c(x, y+1))
}

# Encode positions as single integers: x*100 + y
encode_pos <- function(x, y) as.integer(x * 100 + y)
decode_pos <- function(val) c(val %/% 100, val %% 100)

# Observable: normally true position, exceptionally nearby
observable <- function(position) {
  true_pos <- encode_pos(position[1], position[2])
  
  # Enumerate surrounding positions (8 neighbors)
  surround <- list()
  for (dx in -1:1) {
    for (dy in -1:1) {
      if (dx != 0 || dy != 0) {
        surround[[length(surround) + 1]] <- encode_pos(position[1] + dx, position[2] + dy)
      }
    }
  }
  
  values <- c(true_pos, unlist(surround))
  ranks <- c(0, rep(1, length(surround)))
  rb_from_array_int(values = as.integer(values), ranks = ranks)
}

# Simple localisation: start at (0,0), observe sequence
localise <- function(observations) {
  current_pos <- encode_pos(0, 0)
  current_rank <- 0
  
  for (obs in observations) {
    obs_encoded <- encode_pos(obs[1], obs[2])
    
    # Check if observation matches current position observable
    obs_df <- rb_take_n_int(observable(decode_pos(current_pos)), 20)
    matching <- obs_df[obs_df$value == obs_encoded, ]
    
    if (nrow(matching) > 0) {
      current_rank <- current_rank + matching$rank[1]
    } else {
      current_rank <- current_rank + 10  # penalty for inconsistency
    }
  }
  
  list(position = decode_pos(current_pos), rank = current_rank)
}

run_example <- function() {
  cat("Robot localisation example\n")
  cat("Observations: (0,0)\n\n")
  
  result <- localise(list(c(0, 0)))
  cat(sprintf("Inferred position: (%d, %d) with rank %g\n",
              result$position[1], result$position[2], result$rank))
}

if (!exists("SKIP_EXAMPLE_RUN")) run_example()
