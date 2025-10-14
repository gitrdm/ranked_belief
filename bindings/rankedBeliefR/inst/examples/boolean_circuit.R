# Boolean circuit diagnosis example using the R bindings for ranked_belief

# Gate behaviour helper: normal (TRUE) and exceptional (FALSE)
normal_exceptional <- function(normal_value, exceptional_value, exceptional_rank = 1) {
  function() {
    rb_from_array_int(values = as.integer(c(as.integer(normal_value), as.integer(exceptional_value))),
                      ranks = c(0, exceptional_rank))
  }
}

GATE_BEHAVIOUR <- normal_exceptional(TRUE, FALSE, exceptional_rank = 1)

circuit <- function(input1, input2, input3) {
  rb_merge_int(GATE_BEHAVIOUR(), function(n_gate) {
    # l1: if n_gate TRUE then !input1 else FALSE
    l1 <- if (as.logical(n_gate)) (!input1) else FALSE
    rb_merge_int(GATE_BEHAVIOUR(), function(o1_gate) {
      l2 <- if (as.logical(o1_gate)) (l1 || input2) else FALSE
      rb_merge_int(GATE_BEHAVIOUR(), function(o2_gate) {
        output <- if (as.logical(o2_gate)) (l2 || input3) else FALSE
        rb_singleton_int(as.integer(output))
      })
    })
  })
}

# Note: the above uses a simple combinator style; more ergonomic wrappers may be added.

if (interactive()) {
  res <- rb_observe_value_int(circuit(FALSE, FALSE, TRUE), 0L)
  print(rb_take_n_int(res, 6))
}
