test_that("map and filter callbacks work", {
  r <- rb_from_array_int(values = c(1L,2L,3L), ranks = c(0,1,2))
  mapped <- rb_map_int(r, function(x) as.integer(x * 10L))
  dfm <- rb_take_n_int(mapped, 3)
  expect_equal(dfm$value, c(10L,20L,30L))

  filtered <- rb_filter_int(r, function(x) x != 2L)
  dff <- rb_take_n_int(filtered, 10)
  expect_equal(dff$value, c(1L,3L))

  rb_free(r); rb_free(mapped); rb_free(filtered)
})
