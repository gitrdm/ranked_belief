test_that("callback errors in map/filter propagate as R errors when forcing", {
  r <- rb_from_array_int(values = c(1L,2L,3L), ranks = c(0,1,2))
  mapped <- rb_map_int(r, function(x) stop("callback fail"))
  expect_error(rb_first_int(mapped), "callback error")

  # Filter may raise either when creating the filtered ranking or when forcing it
  filtered_created <- TRUE
  filtered <- NULL
  tryCatch({
    filtered <<- rb_filter_int(r, function(x) stop("predicate fail"))
  }, error = function(e) {
    filtered_created <<- FALSE
    expect_match(conditionMessage(e), "callback error")
  })
  if (filtered_created) {
    expect_error(rb_take_n_int(filtered, 1), "callback error")
    rb_free(filtered)
  }

  rb_free(r); rb_free(mapped); rb_free(filtered)
})
