# Ranking network example (full port from Python)
#
# Demonstrates propagating ranked beliefs through a network

library(rankedBeliefR)

# Encode edges as integer pairs: from*1000 + to
encode_edge <- function(from, to) as.integer(from * 1000 + to)
decode_edge <- function(val) c(val %/% 1000, val %% 1000)

# Edge ranking: prefer low-numbered nodes first
rank_edge <- function(from, to) {
  as.integer(from + to)
}

# Generate ranked edges from a node
edges_from <- function(node, max_degree = 3) {
  candidates <- seq_len(max_degree)
  edges <- sapply(candidates, function(i) encode_edge(node, i))
  ranks <- sapply(candidates, function(i) rank_edge(node, i))
  
  rb_from_array_int(values = as.integer(edges), ranks = ranks)
}

# Propagate through network for n steps
propagate <- function(start_node, steps, max_degree = 3) {
  current <- edges_from(start_node, max_degree)
  
  for (step in seq_len(steps - 1)) {
    # Use rb_merge_apply_int: for each edge, generate new edges from destination
    current <- rb_merge_apply_int(current, function(edge_val) {
      edge <- decode_edge(edge_val)
      to_node <- edge[2]
      edges_from(to_node, max_degree)
    })
  }
  
  current
}

run_example <- function() {
  cat("Ranking network propagation example\n")
  cat("Starting from node 0, propagating for 2 steps\n\n")
  
  result <- propagate(start_node = 0, steps = 2)
  samples <- rb_take_n_int(result, 10)
  
  cat("Top 10 paths:\n")
  for (i in seq_len(nrow(samples))) {
    edge <- decode_edge(samples$value[i])
    cat(sprintf("  %d->%d (rank %g)\n", edge[1], edge[2], samples$rank[i]))
  }
}

if (!exists("SKIP_EXAMPLE_RUN")) run_example()
