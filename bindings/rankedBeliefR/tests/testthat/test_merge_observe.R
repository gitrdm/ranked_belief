test_that("merge and observe_value behave as expected", {
  a <- rb_from_array_int(values = c(1L,3L), ranks = c(0,2))
  b <- rb_from_array_int(values = c(2L), ranks = c(1))
  m <- rb_merge_int(a, b)
  dfm <- rb_take_n_int(m, 3)
  expect_equal(dfm$value, c(1L,2L,3L))

  obs <- rb_observe_value_int(m, 2L)
  dfobs <- rb_take_n_int(obs, 10)
  expect_equal(nrow(dfobs), 1)
  expect_equal(dfobs$value[1], 2L)

  rb_free(a); rb_free(b); rb_free(m); rb_free(obs)
})
