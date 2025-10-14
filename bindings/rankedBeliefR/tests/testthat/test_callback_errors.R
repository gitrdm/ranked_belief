test_that("callback errors in map/filter propagate as R errors when forcing", {
  r <- rb_from_array_int(values = c(1L,2L,3L), ranks = c(0,1,2))
  mapped <- rb_map_int(r, function(x) stop("callback fail"))
  expect_error(rb_first_int(mapped), "callback fail")

  filtered <- rb_filter_int(r, function(x) stop("predicate fail"))
  expect_error(rb_take_n_int(filtered, 1), "predicate fail")

  rb_free(r); rb_free(mapped); rb_free(filtered)
})
