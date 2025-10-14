test_that("empty and NULL inputs behave gracefully", {
  expect_error(rb_singleton_int(NA_integer_))
  empty <- rb_from_array_int(values = integer(0), ranks = NULL)
  expect_true(rb_is_empty(empty))
  df <- rb_take_n_int(empty, 10)
  expect_equal(nrow(df), 0)
  rb_free(empty)
})
