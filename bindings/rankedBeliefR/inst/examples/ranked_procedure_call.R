# Ranked procedure call example (full port from Racket)
#
# Demonstrates ranked function application (add vs subtract)

library(rankedBeliefR)

# Encode operation result as integer
add_op <- function(a, b) as.integer(a + b)
sub_op <- function(a, b) as.integer(a - b)

# Apply ranked operation to ranked operands
ranked_apply <- function() {
  # Enumerate all (op, value) combinations with their combined ranks
  # op: 0=add (rank 0), 1=sub (rank 1)
  # value: 10 (rank 0), 20 (rank 1)
  
  combinations <- list(
    list(op = 0L, value = 10L, rank = 0),  # add 10+5 = 15, rank 0+0=0
    list(op = 0L, value = 20L, rank = 1),  # add 20+5 = 25, rank 0+1=1
    list(op = 1L, value = 10L, rank = 1),  # sub 10-5 = 5, rank 1+0=1
    list(op = 1L, value = 20L, rank = 2)   # sub 20-5 = 15, rank 1+1=2
  )
  
  values <- integer(length(combinations))
  ranks <- numeric(length(combinations))
  
  for (i in seq_along(combinations)) {
    op <- combinations[[i]]$op
    val <- combinations[[i]]$value
    
    if (op == 0L) {
      values[i] <- add_op(val, 5L)
    } else {
      values[i] <- sub_op(val, 5L)
    }
    
    ranks[i] <- combinations[[i]]$rank
  }
  
  rb_from_array_int(values = values, ranks = ranks)
}

run_example <- function() {
  cat("Ranked procedure call example\n")
  cat("Operation: normally +, exceptionally -\n")
  cat("First operand: normally 10, exceptionally 20\n")
  cat("Second operand: 5\n\n")
  
  result <- ranked_apply()
  samples <- rb_take_n_int(result, 10)
  
  cat("Possible results:\n")
  for (i in seq_len(nrow(samples))) {
    cat(sprintf("  %d (rank %g)\n", samples$value[i], samples$rank[i]))
  }
}

if (!exists("SKIP_EXAMPLE_RUN")) run_example()
