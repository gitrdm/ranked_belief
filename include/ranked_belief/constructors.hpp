/**
 * @file constructors.hpp
 * @brief Construction functions for creating RankingFunction instances.
 *
 * This file provides high-level construction functions that create ranking
 * functions from various sources:
 * - from_list: Create from value-rank pairs
 * - from_values: Create with uniform or custom rank assignment
 * - from_generator: Create infinite sequences from generator functions
 * - from_range: Create from C++20 ranges
 *
 * These functions complement the basic factory functions in ranking_function.hpp,
 * providing more convenient and expressive ways to construct ranking functions
 * for common use cases.
 *
 * Key features:
 * - Type deduction: Template parameters often deduced from arguments
 * - Lazy evaluation: Sequences constructed on-demand
 * - Flexible rank assignment: Uniform, sequential, or custom
 * - C++20 ranges support: Works with any range type
 * - Builder pattern: Chainable configuration options
 */

#ifndef RANKED_BELIEF_CONSTRUCTORS_HPP
#define RANKED_BELIEF_CONSTRUCTORS_HPP

#include "ranking_function.hpp"
#include <concepts>
#include <ranges>
#include <type_traits>
#include <vector>

namespace ranked_belief {

/**
 * @brief Create a ranking function from a list of value-rank pairs.
 *
 * Constructs a ranking function from an explicit list of (value, rank) pairs.
 * The pairs are processed in order, building a lazy linked list structure.
 *
 * @tparam T The value type (deduced from pairs)
 * @param pairs Vector of (value, rank) pairs
 * @param deduplicate If true, enable deduplication (default: true)
 * @return RankingFunction containing the specified pairs
 *
 * @note Pairs should be provided in non-decreasing rank order for proper
 *       ranking function semantics. No validation is performed.
 *
 * Example:
 * @code
 * auto rf = from_list<int>({
 *     {1, Rank::zero()},
 *     {2, Rank::from_value(1)},
 *     {3, Rank::from_value(2)}
 * });
 * @endcode
 */
template<typename T>
[[nodiscard]] RankingFunction<T> from_list(
    const std::vector<std::pair<T, Rank>>& pairs,
    bool deduplicate = true)
{
    if (pairs.empty()) {
        return RankingFunction<T>();
    }
    
    // Build the sequence from back to front
    std::shared_ptr<RankingElement<T>> head = nullptr;
    for (auto it = pairs.rbegin(); it != pairs.rend(); ++it) {
        auto next = head;
        head = make_element(it->first, it->second, next);
    }
    
    return RankingFunction<T>(head, deduplicate);
}

/**
 * @brief Create a ranking function from a list of values with uniform ranks.
 *
 * All values are assigned the same rank (default: Rank::zero()).
 * This represents a uniform distribution where all alternatives are equally normal.
 *
 * @tparam T The value type (deduced from values)
 * @param values Vector of values
 * @param rank The rank to assign to all values (default: Rank::zero())
 * @param deduplicate If true, enable deduplication (default: true)
 * @return RankingFunction with all values at the specified rank
 *
 * Example:
 * @code
 * auto rf = from_values_uniform({1, 2, 3, 4, 5});  // All at rank 0
 * @endcode
 */
template<typename T>
[[nodiscard]] RankingFunction<T> from_values_uniform(
    const std::vector<T>& values,
    Rank rank = Rank::zero(),
    bool deduplicate = true)
{
    if (values.empty()) {
        return RankingFunction<T>();
    }
    
    std::shared_ptr<RankingElement<T>> head = nullptr;
    for (auto it = values.rbegin(); it != values.rend(); ++it) {
        auto next = head;
        head = make_element(*it, rank, next);
    }
    
    return RankingFunction<T>(head, deduplicate);
}

/**
 * @brief Create a ranking function from values with sequential ranks.
 *
 * Values are assigned ranks in sequence: first value gets start_rank,
 * second gets start_rank + 1, etc. This represents a linear ordering
 * by increasing exceptionality.
 *
 * @tparam T The value type (deduced from values)
 * @param values Vector of values
 * @param start_rank The rank for the first value (default: Rank::zero())
 * @param deduplicate If true, enable deduplication (default: true)
 * @return RankingFunction with sequentially increasing ranks
 *
 * Example:
 * @code
 * auto rf = from_values_sequential({1, 2, 3});
 * // Creates: 1@0, 2@1, 3@2
 * @endcode
 */
template<typename T>
[[nodiscard]] RankingFunction<T> from_values_sequential(
    const std::vector<T>& values,
    Rank start_rank = Rank::zero(),
    bool deduplicate = true)
{
    if (values.empty()) {
        return RankingFunction<T>();
    }
    
    std::shared_ptr<RankingElement<T>> head = nullptr;
    
    // Build list in reverse, computing rank for each position
    for (size_t i = values.size(); i > 0; --i) {
        const size_t index = i - 1;
        Rank rank = start_rank + Rank::from_value(index);
        auto next = head;
        head = make_element(values[index], rank, next);
    }
    
    return RankingFunction<T>(head, deduplicate);
}

/**
 * @brief Create a ranking function from values with custom rank function.
 *
 * Each value is assigned a rank computed by the provided function.
 * This allows flexible rank assignment based on value properties.
 *
 * @tparam T The value type (deduced from values)
 * @tparam F The rank function type (deduced from rank_fn)
 * @param values Vector of values
 * @param rank_fn Function mapping value and index to rank: (const T&, size_t) -> Rank
 * @param deduplicate If true, enable deduplication (default: true)
 * @return RankingFunction with ranks computed by rank_fn
 *
 * @note rank_fn receives both the value and its index in the vector.
 *       The resulting sequence is NOT automatically sorted by rank.
 *
 * Example:
 * @code
 * auto rf = from_values_with_ranker({1, 2, 3, 4, 5},
 *     [](const int& v, size_t idx) { return Rank::from_value(v * v); });
 * // Creates: 1@1, 2@4, 3@9, 4@16, 5@25
 * @endcode
 */
template<typename T, typename F>
requires std::invocable<F, const T&, std::size_t> &&
         std::same_as<std::invoke_result_t<F, const T&, std::size_t>, Rank>
[[nodiscard]] RankingFunction<T> from_values_with_ranker(
    const std::vector<T>& values,
    F rank_fn,
    bool deduplicate = true)
{
    if (values.empty()) {
        return RankingFunction<T>();
    }
    
    // Build pairs first, then construct
    std::vector<std::pair<T, Rank>> pairs;
    pairs.reserve(values.size());
    
    for (std::size_t i = 0; i < values.size(); ++i) {
        pairs.emplace_back(values[i], rank_fn(values[i], i));
    }
    
    return from_list(pairs, deduplicate);
}

/**
 * @brief Create an infinite ranking function from a generator function.
 *
 * The generator is called with increasing indices (0, 1, 2, ...) to produce
 * value-rank pairs on demand. This enables lazy infinite sequences.
 *
 * @tparam T The value type
 * @tparam F The generator function type (deduced from generator)
 * @param generator Function mapping index to (value, rank): size_t -> std::pair<T, Rank>
 * @param start_index The starting index for generation (default: 0)
 * @param deduplicate If true, enable deduplication (default: true)
 * @return RankingFunction representing the infinite sequence
 *
 * @warning The resulting sequence is infinite. Operations like size() will
 *          not terminate. Use with caution.
 *
 * Example:
 * @code
 * auto rf = from_generator<int>([](size_t i) {
 *     return std::make_pair(static_cast<int>(i), Rank::from_value(i));
 * });
 * // Creates infinite sequence: 0@0, 1@1, 2@2, ...
 * @endcode
 */
template<typename T, typename F>
requires std::invocable<F, std::size_t> &&
         std::same_as<std::invoke_result_t<F, std::size_t>, std::pair<T, Rank>>
[[nodiscard]] RankingFunction<T> from_generator(
    F generator,
    std::size_t start_index = 0,
    bool deduplicate = true)
{
    auto head = make_infinite_sequence<T>(
        [gen = std::move(generator)](std::size_t i) {
            return gen(i);
        },
        start_index
    );
    
    return RankingFunction<T>(head, deduplicate);
}

/**
 * @brief Create a ranking function from a C++20 range.
 *
 * Converts any C++20 range of values into a ranking function with sequential
 * ranks starting from start_rank.
 *
 * @tparam R The range type (deduced from range)
 * @param range The input range of values
 * @param start_rank The rank for the first value (default: Rank::zero())
 * @param deduplicate If true, enable deduplication (default: true)
 * @return RankingFunction containing the range values
 *
 * @note This eagerly evaluates the range into a vector, then constructs
 *       the ranking function. For infinite ranges, this will not terminate.
 *
 * Example:
 * @code
 * std::vector<int> vec = {1, 2, 3, 4, 5};
 * auto rf = from_range(vec | std::views::filter([](int x) { return x % 2 == 0; }));
 * // Creates: 2@0, 4@1
 * @endcode
 */
template<std::ranges::input_range R>
[[nodiscard]] auto from_range(
    R&& range,
    Rank start_rank = Rank::zero(),
    bool deduplicate = true)
{
    using T = std::ranges::range_value_t<R>;
    
    std::vector<T> values;
    for (auto&& value : range) {
        values.push_back(std::forward<decltype(value)>(value));
    }
    
    return from_values_sequential(values, start_rank, deduplicate);
}

/**
 * @brief Create a ranking function from a range of value-rank pairs.
 *
 * Converts a range of (value, rank) pairs into a ranking function.
 * This is useful for transforming existing data structures.
 *
 * @tparam R The range type (deduced from range)
 * @param range The input range of (value, rank) pairs
 * @param deduplicate If true, enable deduplication (default: true)
 * @return RankingFunction containing the pairs
 *
 * Example:
 * @code
 * std::map<int, Rank> map = {{1, Rank::zero()}, {2, Rank::from_value(1)}};
 * auto rf = from_pair_range(map);
 * @endcode
 */
template<std::ranges::input_range R>
requires requires(R r) {
    { std::ranges::range_value_t<R>::first } -> std::convertible_to<typename std::ranges::range_value_t<R>::first_type>;
    { std::ranges::range_value_t<R>::second } -> std::convertible_to<Rank>;
}
[[nodiscard]] auto from_pair_range(
    R&& range,
    bool deduplicate = true)
{
    using PairType = std::ranges::range_value_t<R>;
    // Remove const from first_type (needed for std::map which has std::pair<const K, V>)
    using T = std::remove_const_t<typename PairType::first_type>;
    
    std::vector<std::pair<T, Rank>> pairs;
    for (const auto& [value, rank] : range) {
        pairs.emplace_back(value, rank);
    }
    
    return from_list<T>(pairs, deduplicate);
}

/**
 * @brief Create a single-element ranking function (alias for make_singleton_ranking).
 *
 * This is provided for consistency with other construction functions.
 *
 * @tparam T The value type (deduced from value)
 * @param value The single value
 * @param rank The rank (default: Rank::zero())
 * @return RankingFunction with single element
 */
template<typename T>
[[nodiscard]] RankingFunction<T> singleton(T value, Rank rank = Rank::zero()) {
    return make_singleton_ranking(std::move(value), rank);
}

/**
 * @brief Create an empty ranking function (alias for make_empty_ranking).
 *
 * This is provided for consistency with other construction functions.
 *
 * @tparam T The value type
 * @return Empty RankingFunction
 */
template<typename T>
[[nodiscard]] RankingFunction<T> empty() {
    return make_empty_ranking<T>();
}

} // namespace ranked_belief

#endif // RANKED_BELIEF_CONSTRUCTORS_HPP
