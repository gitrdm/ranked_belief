# Localisation (HMM) example (idiomatic R)

neighbours <- function(pos) list(c(pos[1]-1,pos[2]), c(pos[1]+1,pos[2]), c(pos[1],pos[2]-1), c(pos[1],pos[2]+1))

observable <- function(position) {
  # normally the true position, exceptionally any surrounding position
  rb_from_array_int(values = as.integer(c(1L,2L)), ranks = c(0,1))
}

hmm_localisation <- function(observations) {
  # This is an illustrative stub: a full port would mirror the Python logic
  # and produce ranked paths of positions. For brevity the example shows
  # composing priors and applying a simple observe.
  prior <- rb_from_array_int(values = as.integer(c(1L,2L,3L)), ranks = c(0,1,2))
  posterior <- rb_observe_value_int(prior, 1L)
  posterior
}

run_example <- function() {
  cat("Running localisation example (illustrative)\n")
  print(rb_take_n_int(hmm_localisation(list(c(0,0))), 5))
}

if (interactive()) run_example()
