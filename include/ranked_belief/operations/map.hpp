/**
 * @file map.hpp
 * @brief Map operations for RankingFunction - transform values while preserving ranks
 *
 * This file provides lazy map operations that transform the values in a
 * RankingFunction while preserving the rank structure. The operations are
 * fully lazy, meaning transformations are only applied when elements are
 * accessed.
 *
 * @copyright Copyright (c) 2024
 */

#ifndef RANKED_BELIEF_OPERATIONS_MAP_HPP
#define RANKED_BELIEF_OPERATIONS_MAP_HPP

#include "ranked_belief/promise.hpp"
#include "ranked_belief/rank.hpp"
#include "ranked_belief/ranking_element.hpp"
#include "ranked_belief/ranking_function.hpp"

#include <concepts>
#include <functional>
#include <memory>
#include <type_traits>

namespace ranked_belief {

/**
 * @brief Transform each value in a ranking function while preserving ranks.
 *
 * Applies a transformation function to each value in the ranking function,
 * preserving the original ranks. The transformation is **fully lazy** - neither
 * the transformation nor traversal to subsequent elements occurs until values
 * are accessed. Results are memoized after first access.
 *
 * This design matches the Racket ranked-programming library's map-value semantics:
 * - Calling map() returns immediately without computing anything
 * - Accessing the first element triggers transformation of that element only
 * - Traversing to subsequent elements happens lazily
 * - All computations are memoized for efficiency
 *
 * @tparam T The input value type.
 * @tparam F The function type (must be invocable with const T&).
 * @param rf The source ranking function.
 * @param func The transformation function to apply to each value.
 * @param deduplicate Whether to deduplicate consecutive elements with equal values.
 * @return A new ranking function with transformed values.
 *
 * @par Complexity
 * Time: O(1) to create the mapped ranking, O(n) to traverse all elements.
 * Space: O(1) for the initial call, O(depth) for recursive structure.
 *
 * @par Example
 * @code
 * auto rf = from_values_uniform({1, 2, 3});
 * auto doubled = map(rf, [](int x) { return x * 2; });  // Returns immediately
 * // doubled contains {2, 4, 6} with ranks {0, 0, 0}
 * // Values are only computed when accessed via iteration or first()
 * @endcode
 */
template<typename T, typename F>
requires std::invocable<F, const T&>
[[nodiscard]] auto map(
    const RankingFunction<T>& rf,
    F func,
    bool deduplicate = true)
    -> RankingFunction<std::invoke_result_t<F, const T&>>
{
    using R = std::invoke_result_t<F, const T&>;
    
    // Helper function to recursively build the mapped sequence
    // Use shared_ptr to allow safe capture in lazy computations
    using BuildFunc = std::function<std::shared_ptr<RankingElement<R>>(std::shared_ptr<RankingElement<T>>)>;
    auto build_mapped = std::make_shared<BuildFunc>();
    std::weak_ptr<BuildFunc> weak_build = build_mapped;
    
    *build_mapped = [func, weak_build](std::shared_ptr<RankingElement<T>> elem)
        -> std::shared_ptr<RankingElement<R>>
    {
        if (!elem) {
            return nullptr;
        }
        
        // Create a lazy computation that applies func to the element's value
        auto compute_value = [func, elem]() -> R {
            return func(elem->value());
        };
        
        // Create a lazy computation for the next element
        auto build_ref = weak_build.lock();
        auto compute_next = [weak_build, build_ref, elem]() -> std::shared_ptr<RankingElement<R>> {
            if (auto locked = weak_build.lock()) {
                return (*locked)(elem->next());
            }
            if (build_ref) {
                return (*build_ref)(elem->next());
            }
            return nullptr;
        };
        
        // Build the mapped element with lazy value and next
        return std::make_shared<RankingElement<R>>(
            make_promise(std::move(compute_value)),  // Lazy value transformation
            elem->rank(),                             // Preserve rank
            make_promise(std::move(compute_next))     // Lazy next
        );
    };
    
    // Build the head of the mapped sequence
    auto mapped_head = (*build_mapped)(rf.head());
    
    return RankingFunction<R>(mapped_head, deduplicate);
}

/**
 * @brief Map a function over both values and ranks in a ranking function.
 *
 * This function transforms each (value, rank) pair in the ranking function
 * using the provided function. Unlike the basic map operation, this allows
 * transforming the ranks as well as the values.
 *
 * The transformation is lazy - the function is only applied when elements
 * are accessed.
 *
 * @tparam T The input value type
 * @tparam F The function type (deduced)
 * @tparam R The result type (deduced from F's return type)
 *
 * @param rf The input ranking function
 * @param func The transformation function (T, Rank -> std::pair<R, Rank>)
 * @param deduplicate Whether to enable deduplication (default: true)
 *
 * @return RankingFunction<R> with transformed values and ranks
 *
 * @note The function must be callable with (const T&, Rank) and return
 *       a std::pair<R, Rank>.
 *
 * Example:
 * @code
 * auto rf = from_values_sequential({1, 2, 3});
 * auto transformed = map_with_rank(rf, [](int x, Rank r) {
 *     return std::make_pair(x * 10, r + Rank::from_value(5));
 * });
 * // Result: 10@5, 20@6, 30@7
 * @endcode
 */
template<typename T, typename F>
requires std::invocable<F, const T&, Rank>
[[nodiscard]] auto map_with_rank(
    const RankingFunction<T>& rf,
    F func,
    bool deduplicate = true)
    -> RankingFunction<typename std::invoke_result_t<F, const T&, Rank>::first_type>
{
    using PairType = std::invoke_result_t<F, const T&, Rank>;
    using R = typename PairType::first_type;
    
    // Helper function to recursively build the mapped sequence
    // Use shared_ptr to allow safe capture in lazy computations
    using BuildFunc = std::function<std::shared_ptr<RankingElement<R>>(std::shared_ptr<RankingElement<T>>)>;
    auto build_mapped = std::make_shared<BuildFunc>();
    std::weak_ptr<BuildFunc> weak_build = build_mapped;
    
    *build_mapped = [func, weak_build](std::shared_ptr<RankingElement<T>> elem)
        -> std::shared_ptr<RankingElement<R>>
    {
        if (!elem) {
            return nullptr;
        }
        
        // Create a lazy computation that applies func to get value and rank
        auto compute_pair = [func, elem]() -> PairType {
            return func(elem->value(), elem->rank());
        };
        
        // Create promise for the pair, then extract value and rank lazily
        auto pair_promise = std::make_shared<Promise<PairType>>(make_promise(std::move(compute_pair)));
        
        auto compute_value = [pair_promise]() -> R {
            return pair_promise->force().first;
        };
        
        auto compute_rank = [pair_promise]() -> Rank {
            return pair_promise->force().second;
        };
        
        // For rank, we need it immediately to construct the element
        // So we force it once here
        Rank mapped_rank = compute_rank();
        
        // Create a lazy computation for the next element
        auto build_ref = weak_build.lock();
        auto compute_next = [weak_build, build_ref, elem]() -> std::shared_ptr<RankingElement<R>> {
            if (auto locked = weak_build.lock()) {
                return (*locked)(elem->next());
            }
            if (build_ref) {
                return (*build_ref)(elem->next());
            }
            return nullptr;
        };
        
        // Build the mapped element with lazy value and lazy next
        return std::make_shared<RankingElement<R>>(
            make_promise(std::move(compute_value)),  // Lazy value transformation
            mapped_rank,                              // Rank (forced once)
            make_promise(std::move(compute_next))     // Lazy next
        );
    };
    
    // Build the head of the mapped sequence
    auto mapped_head = (*build_mapped)(rf.head());
    
    return RankingFunction<R>(mapped_head, deduplicate);
}

/**
 * @brief Map a function over values with access to their index.
 *
 * This function transforms each value in the ranking function along with
 * its index (position in the sequence), while preserving ranks.
 *
 * The transformation is lazy - the function is only applied when elements
 * are accessed.
 *
 * @tparam T The input value type
 * @tparam F The function type (deduced)
 * @tparam R The result type (deduced from F's return type)
 *
 * @param rf The input ranking function
 * @param func The transformation function (T, size_t -> R)
 * @param deduplicate Whether to enable deduplication (default: true)
 *
 * @return RankingFunction<R> with transformed values and preserved ranks
 *
 * @note The function must be callable with (const T&, size_t) and return R.
 * @note The index starts at 0 for the first element.
 *
 * Example:
 * @code
 * auto rf = from_values_uniform({10, 20, 30});
 * auto indexed = map_with_index(rf, [](int x, size_t idx) {
 *     return std::make_pair(idx, x);
 * });
 * // Result: (0, 10)@0, (1, 20)@0, (2, 30)@0
 * @endcode
 */
template<typename T, typename F>
requires std::invocable<F, const T&, size_t>
[[nodiscard]] auto map_with_index(
    const RankingFunction<T>& rf,
    F func,
    bool deduplicate = true)
    -> RankingFunction<std::invoke_result_t<F, const T&, size_t>>
{
    using R = std::invoke_result_t<F, const T&, size_t>;
    
    // Helper function to recursively build the mapped sequence with index
    // Use shared_ptr to allow safe capture in lazy computations
    using BuildFunc = std::function<std::shared_ptr<RankingElement<R>>(
        std::shared_ptr<RankingElement<T>>, size_t)>;
    auto build_mapped = std::make_shared<BuildFunc>();
    std::weak_ptr<BuildFunc> weak_build = build_mapped;
    
    *build_mapped = [func, weak_build](
        std::shared_ptr<RankingElement<T>> elem,
        size_t index) -> std::shared_ptr<RankingElement<R>>
    {
        if (!elem) {
            return nullptr;
        }
        
        // Create a lazy computation that applies func with index
        auto compute_value = [func, elem, index]() -> R {
            return func(elem->value(), index);
        };
        
        // Create a lazy computation for the next element
        auto build_ref = weak_build.lock();
        auto compute_next = [weak_build, build_ref, elem, index]()
            -> std::shared_ptr<RankingElement<R>>
        {
            if (auto locked = weak_build.lock()) {
                return (*locked)(elem->next(), index + 1);
            }
            if (build_ref) {
                return (*build_ref)(elem->next(), index + 1);
            }
            return nullptr;
        };
        
        // Build the mapped element with lazy value and next
        return std::make_shared<RankingElement<R>>(
            make_promise(std::move(compute_value)),  // Lazy value transformation
            elem->rank(),                             // Preserve rank
            make_promise(std::move(compute_next))     // Lazy next
        );
    };
    
    // Build the head of the mapped sequence starting at index 0
    auto mapped_head = (*build_mapped)(rf.head(), 0);
    
    return RankingFunction<R>(mapped_head, deduplicate);
}

}  // namespace ranked_belief

#endif  // RANKED_BELIEF_OPERATIONS_MAP_HPP
