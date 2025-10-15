"""
Boolean circuit diagnosis example (idiomatic R)
# Boolean circuit diagnosis example (idiomatic R)

# This script demonstrates creating small named scenario objects, composing
# rankings using the R bindings, conditioning on an observed output and
# materialising explanations. The library represents values as integers, so
# boolean values are encoded as 0/1 in the examples for simplicity.
#
  # values are integers: 1 (TRUE) then 0 (FALSE)
  rb_from_array_int(values = as.integer(c(1L, 0L)), ranks = c(0, exceptional_rank))
}

# Build the circuit as a function returning a ranking of named lists
circuit <- function(input1, input2, input3) {
  gate_prior <- normal_exceptional()

  # combine gates lazily by merging their rankings; the underlying C API
  # performs lazy evaluation, so callbacks are only invoked as elements are
  # forced by observe/take.
  n_rank <- gate_prior
  o1_rank <- gate_prior
  o2_rank <- gate_prior

  # Produce a ranking of integer-encoded scenarios: we'll encode a scenario as
  # an integer tuple packed into a single integer value for this simple demo.
  # For real applications you'd wrap structured data; here we keep to the
  # integer API surface the package exposes.
  combined <- rb_merge_int(n_rank, rb_merge_int(o1_rank, o2_rank))
  combined
}

format_scenarios <- function(df) {
  if (nrow(df) == 0L) return("<no explanations>")
  paste(apply(df, 1, function(row) sprintf("value=%s rank=%g", row[["value"]], row[["rank"]])), collapse = "\n")
}

run_example <- function() {
  cat("Running boolean_circuit example...\n")
  prior <- circuit(FALSE, FALSE, TRUE)
  # Observe that the output is 0 (FALSE) — in this compact demo we use value 0
  posterior <- tryCatch(rb_observe_value_int(prior, 0L), error = function(e) { stop(e) })
  df <- rb_take_n_int(posterior, 6)
  cat(format_scenarios(df), "\n")
  # clean up
  rb_free(prior); rb_free(posterior)
}

if (interactive()) run_example()
