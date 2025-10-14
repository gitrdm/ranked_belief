test_that("very large ranks round-trip to numeric but may lose precision", {
  vals <- 1:3
  big_ranks <- as.numeric(c(2^52, 2^52 + 1, 2^52 + 2))
  r <- rb_from_array_int(values = as.integer(vals), ranks = big_ranks)
  df <- rb_take_n_int(r, 3)
  expect_equal(df$rank, big_ranks)
  rb_free(r)
})
