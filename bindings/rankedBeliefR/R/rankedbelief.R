#' Create a ranking containing a single integer.
#'
#' @param value Integer scalar.
#' @return An external pointer representing a lazy ranking.
#' @export
rb_singleton_int <- function(value) {
  value <- as.integer(value)
  if (length(value) != 1L || is.na(value)) {
    stop("`value` must be a single non-missing integer", call. = FALSE)
  }
  .Call(rankedbeliefr_singleton_int, value)
}

#' Construct a ranking from parallel vectors.
#'
#' @param values Integer vector of values.
#' @param ranks Optional non-negative numeric vector of ranks. If `NULL`, ranks
#'   are assigned sequentially starting at zero.
#' @return A lazy ranking handle.
#' @export
rb_from_array_int <- function(values, ranks = NULL) {
  values <- as.integer(values)
  if (anyNA(values)) {
    stop("`values` must not contain missing entries", call. = FALSE)
  }
  if (!is.null(ranks)) {
    if (!is.numeric(ranks) && !is.integer(ranks)) {
      stop("`ranks` must be numeric or integer", call. = FALSE)
    }
    if (length(ranks) != length(values)) {
      stop("`ranks` must have the same length as `values`", call. = FALSE)
    }
    if (any(is.na(ranks))) {
      stop("`ranks` must not contain missing entries", call. = FALSE)
    }
  }
  .Call(rankedbeliefr_from_array_int, values, ranks)
}

#' Materialise the first `n` elements of a ranking.
#'
#' @param ranking Ranking handle created by other functions in this package.
#' @param n Number of elements to request.
#' @return A data frame with columns `value` and `rank`.
#' @export
rb_take_n_int <- function(ranking, n) {
  n <- as.integer(n)
  if (length(n) != 1L || is.na(n) || n < 0L) {
    stop("`n` must be a single non-negative integer", call. = FALSE)
  }
  result <- .Call(rankedbeliefr_take_n_int, ranking, n)
  structure(result, class = c("ranked_belief_materialised", class(result)))
}

#' Obtain the most plausible element from a ranking.
#'
#' @param ranking Ranking handle.
#' @return `NULL` if the ranking is empty, otherwise a list with `value` and `rank`.
#' @export
rb_first_int <- function(ranking) {
  .Call(rankedbeliefr_first_int, ranking)
}

#' Query whether a ranking has any elements.
#'
#' @param ranking Ranking handle.
#' @return Logical scalar.
#' @export
rb_is_empty <- function(ranking) {
  .Call(rankedbeliefr_is_empty, ranking)
}

#' Merge two rankings by rank order.
#'
#' @param lhs,rhs Ranking handles. Either may be `NULL` to represent an empty ranking.
#' @return A new ranking handle representing the merged sequence.
#' @export
rb_merge_int <- function(lhs, rhs) {
  .Call(rankedbeliefr_merge_int, lhs, rhs)
}

#' Condition a ranking on a specific value.
#'
#' @param ranking Ranking handle.
#' @param value Integer value to retain.
#' @return A new ranking handle representing the observed sequence.
#' @export
rb_observe_value_int <- function(ranking, value) {
  value <- as.integer(value)
  if (length(value) != 1L || is.na(value)) {
    stop("`value` must be a single non-missing integer", call. = FALSE)
  }
  .Call(rankedbeliefr_observe_value_int, ranking, value)
}

#' Explicitly release a ranking handle.
#'
#' @param ranking Ranking handle.
#' @export
rb_free <- function(ranking) {
  invisible(.Call(rankedbeliefr_free, ranking))
}

#' Lazily map values in a ranking using an R callback.
#'
#' @param ranking A ranking handle.
#' @param f A function of one argument (value) returning an integer.
#' @return A new ranking handle representing the mapped sequence.
#' @export
rb_map_int <- function(ranking, f) {
  if (!is.function(f)) stop("`f` must be a function", call. = FALSE)
  .Call(rankedbeliefr_map_int, ranking, f)
}

#' Lazily filter values in a ranking using an R predicate.
#'
#' @param ranking A ranking handle.
#' @param pred A function of one argument (value) returning TRUE/FALSE.
#' @return A new ranking handle representing the filtered sequence.
#' @export
rb_filter_int <- function(ranking, pred) {
  if (!is.function(pred)) stop("`pred` must be a function", call. = FALSE)
  .Call(rankedbeliefr_filter_int, ranking, pred)
}

#' Monadic bind (flatMap) - map each element to a ranking and merge results.
#'
#' This is the fundamental operation for composing ranked computations without
#' manual enumeration. For each element (value, rank) in the input ranking,
#' the callback produces a new ranking whose ranks are shifted by `rank`,
#' and all results are merged in rank order.
#'
#' @param ranking A ranking handle.
#' @param f A function of one argument (value) returning a ranking handle.
#' @return A new ranking handle representing the merged result.
#' @export
rb_merge_apply_int <- function(ranking, f) {
  if (!is.function(f)) stop("`f` must be a function", call. = FALSE)
  .Call(rankedbeliefr_merge_apply_int, ranking, f)
}

#' @export
print.ranked_belief_ranking <- function(x, n = 6L, ...) {
  cat("<ranked_belief_ranking>\n", sep = "")
  preview <- try(rb_take_n_int(x, n), silent = TRUE)
  if (inherits(preview, "try-error")) {
    cat("  <unable to materialise preview>\n")
  } else if (nrow(preview) == 0L) {
    cat("  <empty>\n")
  } else {
    utils::str(preview, vec.len = n, give.head = FALSE)
  }
  invisible(x)
}
