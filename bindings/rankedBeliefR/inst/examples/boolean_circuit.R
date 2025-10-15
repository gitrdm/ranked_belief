# Boolean circuit diagnosis example (full port from Python/Racket)
#
# This example demonstrates fault diagnosis in a three-gate boolean circuit.
# Each gate can fail (rank 1) or work normally (rank 0). We observe the output
# and infer which gate failures explain it.

library(rankedBeliefR)

# Helper: normal/exceptional ranking for booleans encoded as 0/1
normal_exceptional <- function(normal_val, exceptional_val, exceptional_rank = 1) {
  rb_from_array_int(
    values = as.integer(c(normal_val, exceptional_val)),
    ranks = c(0, exceptional_rank)
  )
}

# Compute circuit output given inputs and gate states
# Gates: N (NOT gate), O1, O2 (OR gates)
# Circuit: output = ((NOT input1) OR input2) OR input3 when all gates work
circuit <- function(input1, input2, input3) {
  # Each gate: normally active (1), exceptionally fails (0)
  # We enumerate all 2^3=8 scenarios and compute output + rank for each
  
  scenarios <- list()
  for (n in c(1L, 0L)) {
    for (o1 in c(1L, 0L)) {
      for (o2 in c(1L, 0L)) {
        l1 <- if (n == 1L) as.integer(!input1) else 0L
        l2 <- if (o1 == 1L) as.integer(l1 | input2) else 0L
        output <- if (o2 == 1L) as.integer(l2 | input3) else 0L
        
        # Rank = sum of gate failures
        rank <- (1L - n) + (1L - o1) + (1L - o2)
        
        # Encode as single integer: output*8 + n*4 + o1*2 + o2
        encoded <- output * 8L + n * 4L + o1 * 2L + o2
        scenarios[[length(scenarios) + 1]] <- list(value = encoded, rank = rank)
      }
    }
  }
  
  # Build ranking from scenarios
  values <- sapply(scenarios, function(s) s$value)
  ranks <- sapply(scenarios, function(s) s$rank)
  rb_from_array_int(values = as.integer(values), ranks = ranks)
}

# Decode scenario integer back to components
decode_scenario <- function(val) {
  output <- bitwShiftR(val, 3) %% 2
  n_gate <- bitwShiftR(val, 2) %% 2
  o1_gate <- bitwShiftR(val, 1) %% 2
  o2_gate <- val %% 2
  list(output = output, N = n_gate, O1 = o1_gate, O2 = o2_gate)
}

run_example <- function() {
  cat("Boolean circuit diagnosis example\n")
  cat("Inputs: input1=FALSE, input2=FALSE, input3=TRUE\n")
  cat("Observed output: FALSE\n\n")
  
  prior <- circuit(FALSE, FALSE, TRUE)
  
  # Observe output = FALSE (encoded in bit 3, so output=0 means values with bit3=0)
  posterior_df <- rb_take_n_int(prior, 100)
  
  # Filter for output=0
  output_0_rows <- posterior_df[bitwShiftR(posterior_df$value, 3) %% 2 == 0, ]
  output_0_rows <- output_0_rows[order(output_0_rows$rank), ]
  top_explanations <- head(output_0_rows, 6)
  
  cat("Top explanations (rank = # of failed gates):\n")
  for (i in seq_len(nrow(top_explanations))) {
    sc <- decode_scenario(top_explanations$value[i])
    cat(sprintf("%d. rank=%g  output=%d N=%d O1=%d O2=%d\n",
                i, top_explanations$rank[i], sc$output, sc$N, sc$O1, sc$O2))
  }
  
  rb_free(prior)
}

if (!exists("SKIP_EXAMPLE_RUN")) run_example()
