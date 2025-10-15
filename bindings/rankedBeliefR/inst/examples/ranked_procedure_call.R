# Ranked procedure call example (full port from Racket)
#
# Demonstrates ranked function application (add vs subtract)

library(rankedBeliefR)

# Encode operation result as integer
add_op <- function(a, b) as.integer(a + b)
sub_op <- function(a, b) as.integer(a - b)

# Apply ranked operation to ranked operands
ranked_apply <- function() {
  # Value: normally 10, exceptionally 20
  value_dist <- rb_from_array_int(values = c(10L, 20L), ranks = c(0, 1))
  
  # Operation: normally add, exceptionally subtract
  # Encode as: 0=add, 1=subtract
  op_dist <- rb_from_array_int(values = c(0L, 1L), ranks = c(0, 1))
  
  # Use rb_merge_apply_int to compose: for each op, apply it to each value
  result <- rb_merge_apply_int(op_dist, function(op_val) {
    rb_merge_apply_int(value_dist, function(val) {
      if (op_val == 0L) {
        rb_singleton_int(add_op(val, 5L))
      } else {
        rb_singleton_int(sub_op(val, 5L))
      }
    })
  })
  
  result
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
