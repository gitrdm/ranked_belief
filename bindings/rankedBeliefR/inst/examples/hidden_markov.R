# Hidden Markov model example (idiomatic R)

# States and emissions are simple strings here; the R bindings use integers so
# we encode states/emissions as small integers and provide helpers to map back.

states <- c("rainy", "sunny")
emissions <- c("yes", "no")
state_idx <- setNames(seq_along(states), states)
emission_idx <- setNames(seq_along(emissions), emissions)

normal_exceptional_state <- function(normal, exceptional, exceptional_rank = 2) {
  rb_from_array_int(values = as.integer(c(state_idx[[normal]], state_idx[[exceptional]])),
                    ranks = c(0, exceptional_rank))
}

emit_for_state <- function(state) {
  if (state == "rainy") {
    rb_from_array_int(values = as.integer(c(emission_idx[["yes"]], emission_idx[["no"]])), ranks = c(0, 1))
  } else {
    rb_from_array_int(values = as.integer(c(emission_idx[["no"]], emission_idx[["yes"]])), ranks = c(0, 1))
  }
}

trans_for_state <- function(state) {
  if (state == "rainy") {
    normal_exceptional_state("rainy", "sunny", exceptional_rank = 2)
  } else {
    normal_exceptional_state("sunny", "rainy", exceptional_rank = 2)
  }
}

hmm <- function(observations) {
  initialise <- function(s) {
    rb_singleton_int(as.integer(state_idx[[s]]))
  }

  ranking <- rb_map_int(rb_from_array_int(values = as.integer(c(state_idx[["rainy"]], state_idx[["sunny"]])), ranks = c(0,1)), function(x) x)
  for (obs in observations) {
    # naive step: this is illustrative—full composition requires a richer DSL
    ranking <- rb_merge_int(ranking, rb_from_array_int(values = as.integer(c(emission_idx[[obs]])), ranks = c(0)))
  }
  ranking
}

run_example <- function() {
  cat("Running hidden_markov example (illustrative)\n")
  df <- rb_take_n_int(rb_from_array_int(values = as.integer(c(1L,2L,3L)), ranks = c(0,1,2)), 3)
  print(df)
}

if (interactive()) run_example()
