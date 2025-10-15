test_that("rb_merge_apply_int works for ranked procedure application", {
  # Ranked operation: normally add (0), exceptionally subtract (1)
  ops <- rb_from_array_int(values = c(0L, 1L), ranks = c(0, 1))
  
  # Ranked value: normally 10, exceptionally 20
  values <- rb_from_array_int(values = c(10L, 20L), ranks = c(0, 1))
  
  # Apply: for each operation, apply it to each value
  result <- rb_merge_apply_int(ops, function(op) {
    rb_merge_apply_int(values, function(val) {
      if (op == 0L) {
        # add: val + 5
        rb_singleton_int(val + 5L)
      } else {
        # subtract: val - 5
        rb_singleton_int(val - 5L)
      }
    })
  })
  
  samples <- rb_take_n_int(result, 10)
  
  # Expected results:
  # op=0 (rank 0), val=10 (rank 0): 10+5=15 at rank 0+0=0
  # op=0 (rank 0), val=20 (rank 1): 20+5=25 at rank 0+1=1
  # op=1 (rank 1), val=10 (rank 0): 10-5=5  at rank 1+0=1
  # op=1 (rank 1), val=20 (rank 1): 20-5=15 at rank 1+1=2
  
  expect_equal(nrow(samples), 4)
  expect_equal(samples$value[1], 15)
  expect_equal(samples$rank[1], 0)
  
  # Ranks 1 should have both 25 and 5
  rank1 <- samples[samples$rank == 1, ]
  expect_equal(nrow(rank1), 2)
  expect_true(25 %in% rank1$value)
  expect_true(5 %in% rank1$value)
  
  # Rank 2 should have 15
  rank2 <- samples[samples$rank == 2, ]
  expect_equal(nrow(rank2), 1)
  expect_equal(rank2$value[1], 15)
})

test_that("rb_merge_apply_int handles empty rankings", {
  empty <- rb_from_array_int(values = integer(0), ranks = numeric(0))
  
  result <- rb_merge_apply_int(empty, function(x) rb_singleton_int(x * 2L))
  
  expect_true(rb_is_empty(result))
})

test_that("rb_merge_apply_int propagates callback errors", {
  ranking <- rb_singleton_int(42L)
  
  expect_error(
    rb_merge_apply_int(ranking, function(x) stop("test error")),
    "callback error"
  )
})
