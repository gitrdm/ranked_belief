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
  # Manually enumerate paths for given steps
  # For 2 steps: 0 -> i -> j for i,j in 1:max_degree
  
  if (steps == 1) {
    return(edges_from(start_node, max_degree))
  }
  
  if (steps == 2) {
    # Generate all 2-hop paths from start_node
    values <- integer(0)
    ranks <- numeric(0)
    
    for (i in seq_len(max_degree)) {
      edge1_val <- encode_edge(start_node, i)
      edge1_rank <- rank_edge(start_node, i)
      
      for (j in seq_len(max_degree)) {
        edge2_val <- encode_edge(i, j)
        edge2_rank <- rank_edge(i, j)
        
        # Final edge in path
        values <- c(values, edge2_val)
        ranks <- c(ranks, edge1_rank + edge2_rank)
      }
    }
    
    return(rb_from_array_int(values = as.integer(values), ranks = ranks))
  }
  
  stop("Only steps 1 and 2 supported for this simple example")
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
