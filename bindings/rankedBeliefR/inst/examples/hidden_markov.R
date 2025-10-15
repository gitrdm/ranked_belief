# Hidden Markov model example (full port from Python)
#
# Weather HMM: rainy/sunny states, umbrella yes/no observations

library(rankedBeliefR)

# Encode states and emissions as integers
RAINY <- 1L
SUNNY <- 2L
YES <- 1L
NO <- 2L

state_name <- c("rainy", "sunny")
emission_name <- c("yes", "no")

# Initial distribution: equal plausibility
init_distribution <- function() {
  rb_from_array_int(values = c(RAINY, SUNNY), ranks = c(0, 0))
}

# Transition model
trans <- function(state) {
  if (state == RAINY) {
    # rainy -> rainy (normal), rainy -> sunny (exceptional, rank 2)
    rb_from_array_int(values = c(RAINY, SUNNY), ranks = c(0, 2))
  } else {
    # sunny -> sunny (normal), sunny -> rainy (exceptional, rank 2)
    rb_from_array_int(values = c(SUNNY, RAINY), ranks = c(0, 2))
  }
}

# Emission model
emit <- function(state) {
  if (state == RAINY) {
    # yes (normal), no (exceptional, rank 1)
    rb_from_array_int(values = c(YES, NO), ranks = c(0, 1))
  } else {
    # no (normal), yes (exceptional, rank 1)
    rb_from_array_int(values = c(NO, YES), ranks = c(0, 1))
  }
}

# Simple HMM forward pass (behaviorally equivalent but not fully lazy)
# Returns ranking over final states given observations
hmm <- function(observations) {
  # Start with initial distribution
  current_states <- rb_take_n_int(init_distribution(), 10)
  
  # For each observation, update beliefs
  for (obs in observations) {
    next_scenarios <- list()
    
    for (i in seq_len(nrow(current_states))) {
      state <- current_states$value[i]
      state_rank <- current_states$rank[i]
      
      # Transition to next states
      trans_df <- rb_take_n_int(trans(state), 10)
      
      for (j in seq_len(nrow(trans_df))) {
        next_state <- trans_df$value[j]
        trans_rank <- trans_df$rank[j]
        
        # Check emission consistency
        emit_df <- rb_take_n_int(emit(next_state), 10)
        matching <- emit_df[emit_df$value == obs, ]
        
        if (nrow(matching) > 0) {
          emit_rank <- matching$rank[1]
          total_rank <- state_rank + trans_rank + emit_rank
          next_scenarios[[length(next_scenarios) + 1]] <- 
            list(state = next_state, rank = total_rank)
        }
      }
    }
    
    # Aggregate and build next distribution
    if (length(next_scenarios) > 0) {
      states_vec <- sapply(next_scenarios, function(s) s$state)
      ranks_vec <- sapply(next_scenarios, function(s) s$rank)
      temp <- rb_from_array_int(values = as.integer(states_vec), ranks = ranks_vec)
      current_states <- rb_take_n_int(temp, 100)
      rb_free(temp)
    }
  }
  
  current_states
}

run_example <- function() {
  cat("Hidden Markov model example\n")
  cat("Observations: umbrella=yes, umbrella=yes\n\n")
  
  result <- hmm(c(YES, YES))
  result <- result[order(result$rank), ]
  top <- head(result, 5)
  
  cat("Top state sequences:\n")
  for (i in seq_len(nrow(top))) {
    state_label <- state_name[top$value[i]]
    cat(sprintf("%d. state=%s rank=%g\n", i, state_label, top$rank[i]))
  }
}

if (!exists("SKIP_EXAMPLE_RUN")) run_example()
