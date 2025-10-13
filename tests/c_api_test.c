#include "ranked_belief/c_api.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#define CHECK(expr)                                                                   \
    do {                                                                              \
        if (!(expr)) {                                                                \
            abort();                                                                  \
        }                                                                             \
    } while (0)

#define EXPECT_OK(expr)                                                               \
    do {                                                                              \
        rb_status _status = (expr);                                                   \
        if (_status != RB_STATUS_OK) {                                                \
            abort();                                                                  \
        }                                                                             \
    } while (0)

static void test_singleton_and_first(void)
{
    rb_ranking_t *ranking = NULL;
    EXPECT_OK(rb_singleton_int(42, &ranking));
    CHECK(ranking != NULL);

    int value = 0;
    uint64_t rank = 0;
    int has_value = 0;
    EXPECT_OK(rb_first_int(ranking, &value, &rank, &has_value));
    CHECK(has_value == 1);
    CHECK(value == 42);
    CHECK(rank == 0);

    int is_empty = 0;
    EXPECT_OK(rb_is_empty(ranking, &is_empty));
    CHECK(is_empty == 0);

    rb_ranking_free(ranking);
}

typedef struct {
    size_t calls;
} map_context;

static rb_status doubling_callback(int input_value, void *context, int *output_value)
{
    map_context *ctx = (map_context *)context;
    if (ctx != NULL) {
        ctx->calls += 1;
    }
    *output_value = input_value * 2;
    return RB_STATUS_OK;
}

static void test_map_is_lazy(void)
{
    int values[] = {1, 2, 3};
    rb_ranking_t *source = NULL;
    EXPECT_OK(rb_from_array_int(values, NULL, 3, &source));

    map_context ctx = {0};
    rb_ranking_t *mapped = NULL;
    EXPECT_OK(rb_map_int(source, doubling_callback, &ctx, &mapped));
    CHECK(mapped != NULL);
    CHECK(ctx.calls == 0); /* No evaluation yet. */

    int value = 0;
    uint64_t rank = 0;
    int has_value = 0;
    EXPECT_OK(rb_first_int(mapped, &value, &rank, &has_value));
    CHECK(has_value == 1);
    CHECK(value == 2);
    CHECK(rank == 0);
    CHECK(ctx.calls == 1);

    int out_values[3] = {0};
    uint64_t out_ranks[3] = {0};
    size_t out_count = 0;
    EXPECT_OK(rb_take_n_int(mapped, 3, out_values, out_ranks, 3, &out_count));
    CHECK(out_count == 3);
    CHECK(out_values[0] == 2);
    CHECK(out_values[1] == 4);
    CHECK(out_values[2] == 6);
    CHECK(out_ranks[1] == 1);
    CHECK(ctx.calls == 3); /* Only two additional evaluations. */

    rb_ranking_free(mapped);
    rb_ranking_free(source);
}

typedef struct {
    size_t calls;
} filter_context;

static rb_status keep_even_callback(int input_value, void *context, int *keep)
{
    filter_context *ctx = (filter_context *)context;
    if (ctx != NULL) {
        ctx->calls += 1;
    }
    *keep = (input_value % 2 == 0) ? 1 : 0;
    return RB_STATUS_OK;
}

static void test_filter_selects_only_even_values(void)
{
    int values[] = {1, 2, 3, 4};
    rb_ranking_t *source = NULL;
    EXPECT_OK(rb_from_array_int(values, NULL, 4, &source));

    filter_context ctx = {0};
    rb_ranking_t *filtered = NULL;
    EXPECT_OK(rb_filter_int(source, keep_even_callback, &ctx, &filtered));

    int out_values[2] = {0};
    uint64_t out_ranks[2] = {0};
    size_t out_count = 0;
    EXPECT_OK(rb_take_n_int(filtered, 2, out_values, out_ranks, 2, &out_count));
    CHECK(out_count == 2);
    CHECK(out_values[0] == 2);
    CHECK(out_values[1] == 4);
    CHECK(out_ranks[0] == 1);
    CHECK(out_ranks[1] == 3);
    CHECK(ctx.calls >= 2); /* Lazy predicate evaluation on demand. */

    rb_ranking_free(filtered);
    rb_ranking_free(source);
}

static void test_merge_and_observe(void)
{
    int left_values[] = {1, 3};
    uint64_t left_ranks[] = {0, 2};
    int right_values[] = {2};
    uint64_t right_ranks[] = {1};

    rb_ranking_t *lhs = NULL;
    EXPECT_OK(rb_from_array_int(left_values, left_ranks, 2, &lhs));

    rb_ranking_t *rhs = NULL;
    EXPECT_OK(rb_from_array_int(right_values, right_ranks, 1, &rhs));

    rb_ranking_t *merged = NULL;
    EXPECT_OK(rb_merge_int(lhs, rhs, &merged));

    int out_values[3] = {0};
    uint64_t out_ranks[3] = {0};
    size_t out_count = 0;
    EXPECT_OK(rb_take_n_int(merged, 3, out_values, out_ranks, 3, &out_count));
    CHECK(out_count == 3);
    CHECK(out_values[0] == 1);
    CHECK(out_values[1] == 2);
    CHECK(out_values[2] == 3);
    CHECK(out_ranks[1] == 1);

    rb_ranking_t *observed = NULL;
    EXPECT_OK(rb_observe_value_int(merged, 2, &observed));

    int value = 0;
    uint64_t rank = 0;
    int has_value = 0;
    EXPECT_OK(rb_first_int(observed, &value, &rank, &has_value));
    CHECK(has_value == 1);
    CHECK(value == 2);
    CHECK(rank == 0);

    rb_ranking_free(observed);
    rb_ranking_free(merged);
    rb_ranking_free(rhs);
    rb_ranking_free(lhs);
}

static rb_status error_after_two_callback(int input_value, void *context, int *output_value)
{
    (void)context;
    if (input_value == 3) {
        (void)output_value;
        return RB_STATUS_CALLBACK_ERROR;
    }
    *output_value = input_value;
    return RB_STATUS_OK;
}

static void test_callback_error_propagates(void)
{
    int values[] = {1, 2, 3};
    rb_ranking_t *source = NULL;
    EXPECT_OK(rb_from_array_int(values, NULL, 3, &source));

    rb_ranking_t *mapped = NULL;
    EXPECT_OK(rb_map_int(source, error_after_two_callback, NULL, &mapped));

    int out_values[3] = {0};
    uint64_t out_ranks[3] = {0};
    size_t out_count = 0;
    rb_status status = rb_take_n_int(mapped, 3, out_values, out_ranks, 3, &out_count);
    CHECK(status == RB_STATUS_CALLBACK_ERROR);
    CHECK(out_count == 0);

    rb_ranking_free(mapped);
    rb_ranking_free(source);
}

static void test_take_n_buffer_check(void)
{
    int values[] = {7, 8};
    rb_ranking_t *ranking = NULL;
    EXPECT_OK(rb_from_array_int(values, NULL, 2, &ranking));

    int out_values[1] = {0};
    uint64_t out_ranks[1] = {0};
    size_t out_count = 0;
    rb_status status = rb_take_n_int(ranking, 2, out_values, out_ranks, 1, &out_count);
    CHECK(status == RB_STATUS_INSUFFICIENT_BUFFER);
    CHECK(out_count == 0);

    rb_ranking_free(ranking);
}

static void test_merge_with_null_operand(void)
{
    int values[] = {9};
    rb_ranking_t *ranking = NULL;
    EXPECT_OK(rb_from_array_int(values, NULL, 1, &ranking));

    rb_ranking_t *merged = NULL;
    EXPECT_OK(rb_merge_int(NULL, ranking, &merged));

    int value = 0;
    uint64_t rank = 0;
    int has_value = 0;
    EXPECT_OK(rb_first_int(merged, &value, &rank, &has_value));
    CHECK(has_value == 1);
    CHECK(value == 9);

    rb_ranking_free(merged);
    rb_ranking_free(ranking);
}

static void test_empty_construction(void)
{
    rb_ranking_t *ranking = NULL;
    EXPECT_OK(rb_from_array_int(NULL, NULL, 0, &ranking));
    CHECK(ranking != NULL);

    int is_empty = 0;
    EXPECT_OK(rb_is_empty(ranking, &is_empty));
    CHECK(is_empty == 1);

    rb_ranking_free(ranking);
}

int main(void)
{
    test_singleton_and_first();
    test_map_is_lazy();
    test_filter_selects_only_even_values();
    test_merge_and_observe();
    test_callback_error_propagates();
    test_take_n_buffer_check();
    test_merge_with_null_operand();
    test_empty_construction();

    rb_ranking_free(NULL); /* Should be a no-op. */
    return 0;
}
