/**
 * @file c_api.h
 * @brief C interoperability layer for the ranked belief library.
 *
 * The functions declared here expose a minimal, lazy-preserving API that allows
 * plain C callers to construct, transform, and observe ranked belief sequences
 * without depending on C++ syntax or runtime features. All handles returned by
 * this interface are opaque and must be released with ::rb_ranking_free when no
 * longer needed.
 */

#ifndef RANKED_BELIEF_C_API_H
#define RANKED_BELIEF_C_API_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Status codes returned by the C API.
 */
typedef enum rb_status {
    /** Operation completed successfully. */
    RB_STATUS_OK = 0,
    /** One or more pointer arguments were null or an input was otherwise invalid. */
    RB_STATUS_INVALID_ARGUMENT = 1,
    /** Allocation failed while constructing a ranking. */
    RB_STATUS_ALLOCATION_FAILURE = 2,
    /** User-provided callback reported an error. */
    RB_STATUS_CALLBACK_ERROR = 3,
    /** Requested buffer was too small to hold the materialised prefix. */
    RB_STATUS_INSUFFICIENT_BUFFER = 4,
    /** Lazy evaluation raised an unexpected internal exception. */
    RB_STATUS_INTERNAL_ERROR = 100
} rb_status;

/**
 * @brief Opaque ranking handle for C callers.
 */
typedef struct rb_ranking_t rb_ranking_t;

/**
 * @brief Callback signature used by ::rb_map_int.
 *
 * @param input_value The value being transformed.
 * @param context Caller-provided context pointer passed through unchanged.
 * @param output_value Receives the transformed value on success.
 * @return ::RB_STATUS_OK on success, any other ::rb_status value to signal an error.
 *
 * @note The callback is evaluated lazily: it is invoked only when elements of
 *       the mapped ranking are forced via ::rb_first_int or ::rb_take_n_int.
 */
typedef rb_status (*rb_map_callback_t)(int input_value, void *context, int *output_value);

/**
 * @brief Callback signature used by ::rb_filter_int.
 *
 * @param input_value The candidate value.
 * @param context Caller-provided context pointer passed through unchanged.
 * @param keep Receives 1 if the value should be kept, 0 to drop it.
 * @return ::RB_STATUS_OK on success, any other ::rb_status value to signal an error.
 *
 * @note Like ::rb_map_callback_t, filter callbacks are evaluated lazily.
 */
typedef rb_status (*rb_filter_callback_t)(int input_value, void *context, int *keep);

/**
 * @brief Allocate a ranking containing a single integer at rank zero.
 *
 * @param value The value to store.
 * @param out_ranking Receives the newly allocated ranking handle.
 * @return ::RB_STATUS_OK on success, or ::RB_STATUS_INVALID_ARGUMENT / ::RB_STATUS_ALLOCATION_FAILURE otherwise.
 */
rb_status rb_singleton_int(int value, rb_ranking_t **out_ranking);

/**
 * @brief Construct a ranking from parallel arrays of values and ranks.
 *
 * @param values Pointer to an array of `count` integer values.
 * @param ranks Pointer to an array of `count` ranks (as unsigned 64-bit integers).
 *              If `ranks` is `NULL`, ranks are assigned sequentially starting at zero.
 * @param count Number of elements to copy from the arrays.
 * @param out_ranking Receives the resulting ranking handle on success.
 * @return ::RB_STATUS_OK on success, ::RB_STATUS_INVALID_ARGUMENT for bad inputs,
 *         or ::RB_STATUS_ALLOCATION_FAILURE if allocation fails.
 */
rb_status rb_from_array_int(const int *values, const uint64_t *ranks, size_t count, rb_ranking_t **out_ranking);

/**
 * @brief Lazily map each value in a ranking through a user-provided callback.
 *
 * @param ranking Source ranking to transform.
 * @param callback Function invoked lazily for each element.
 * @param context Arbitrary pointer supplied to each callback invocation.
 * @param out_ranking Receives the mapped ranking on success.
 * @return ::RB_STATUS_OK on success, ::RB_STATUS_INVALID_ARGUMENT for bad inputs,
 *         or ::RB_STATUS_ALLOCATION_FAILURE if allocation fails during wrapper creation.
 *
 * @note Callback failures are propagated when elements are forced (for example
 *       via ::rb_first_int); the mapping itself remains lazy.
 */
rb_status rb_map_int(const rb_ranking_t *ranking, rb_map_callback_t callback, void *context, rb_ranking_t **out_ranking);

/**
 * @brief Lazily filter elements using a user-provided predicate.
 *
 * @param ranking Source ranking to filter.
 * @param callback Predicate evaluated lazily for each candidate.
 * @param context Arbitrary pointer supplied to each callback invocation.
 * @param out_ranking Receives the filtered ranking on success.
 * @return ::RB_STATUS_OK on success, ::RB_STATUS_INVALID_ARGUMENT for bad inputs,
 *         or ::RB_STATUS_ALLOCATION_FAILURE if allocation fails during wrapper creation.
 */
rb_status rb_filter_int(const rb_ranking_t *ranking, rb_filter_callback_t callback, void *context, rb_ranking_t **out_ranking);

/**
 * @brief Merge two rankings, returning a lazy union ordered by rank.
 *
 * @param lhs Left-hand ranking operand (may be `NULL` to represent an empty ranking).
 * @param rhs Right-hand ranking operand (may be `NULL` to represent an empty ranking).
 * @param out_ranking Receives the merged ranking on success.
 * @return ::RB_STATUS_OK on success, ::RB_STATUS_INVALID_ARGUMENT for bad inputs,
 *         or ::RB_STATUS_ALLOCATION_FAILURE if allocation fails during wrapper creation.
 */
rb_status rb_merge_int(const rb_ranking_t *lhs, const rb_ranking_t *rhs, rb_ranking_t **out_ranking);

/**
 * @brief Condition a ranking on equality with a specific value.
 *
 * @param ranking Source ranking to observe.
 * @param value Value to retain (only elements equal to `value` are kept).
 * @param out_ranking Receives the observed ranking on success.
 * @return ::RB_STATUS_OK on success, or ::RB_STATUS_INVALID_ARGUMENT / ::RB_STATUS_ALLOCATION_FAILURE otherwise.
 */
rb_status rb_observe_value_int(const rb_ranking_t *ranking, int value, rb_ranking_t **out_ranking);

/**
 * @brief Query whether a ranking is empty without forcing any elements.
 *
 * @param ranking Ranking handle to query.
 * @param out_is_empty Receives 1 if the ranking has no elements, 0 otherwise.
 * @return ::RB_STATUS_OK on success or ::RB_STATUS_INVALID_ARGUMENT if parameters are bad.
 */
rb_status rb_is_empty(const rb_ranking_t *ranking, int *out_is_empty);

/**
 * @brief Obtain the most plausible value (if any) without materialising the entire ranking.
 *
 * @param ranking Ranking handle to query.
 * @param out_value Receives the value when present.
 * @param out_rank Receives the associated rank when present.
 * @param out_has_value Receives 1 when a value was returned, 0 if the ranking is empty.
 * @return ::RB_STATUS_OK on success, or a callback-propagated status if lazy evaluation failed.
 */
rb_status rb_first_int(const rb_ranking_t *ranking, int *out_value, uint64_t *out_rank, int *out_has_value);

/**
 * @brief Materialise up to `n` elements from the head of a ranking.
 *
 * @param ranking Ranking handle to query.
 * @param n Maximum number of elements to materialise.
 * @param out_values Output buffer with capacity for at least `buffer_size` values.
 * @param out_ranks Output buffer with capacity for at least `buffer_size` rank entries.
 * @param buffer_size Number of elements available in `out_values`/`out_ranks`.
 * @param out_count Receives the number of elements written (may be less than `n`).
 * @return ::RB_STATUS_OK on success, ::RB_STATUS_INSUFFICIENT_BUFFER if `buffer_size < n`,
 *         or a callback-propagated status if lazy evaluation fails.
 */
rb_status rb_take_n_int(const rb_ranking_t *ranking, size_t n, int *out_values, uint64_t *out_ranks, size_t buffer_size, size_t *out_count);

/**
 * @brief Release a ranking handle allocated by the C API.
 *
 * @param ranking Handle to release (may be `NULL`).
 */
void rb_ranking_free(rb_ranking_t *ranking);

#ifdef __cplusplus
}
#endif

#endif // RANKED_BELIEF_C_API_H
