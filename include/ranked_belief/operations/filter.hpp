/**
 * @file filter.hpp
 * @brief Lazy filtering operations for RankingFunction.
 *
 * Provides operations to filter ranking functions based on predicates while
 * maintaining lazy evaluation. All operations preserve the lazy structure and
 * only evaluate elements as needed.
 *
 * @author ranked_belief
 * @date 2024
 */

#ifndef RANKED_BELIEF_OPERATIONS_FILTER_HPP
#define RANKED_BELIEF_OPERATIONS_FILTER_HPP

#include "ranked_belief/promise.hpp"
#include "ranked_belief/rank.hpp"
#include "ranked_belief/ranking_element.hpp"
#include "ranked_belief/ranking_function.hpp"
#include <concepts>
#include <functional>
#include <memory>

namespace ranked_belief {

/**
 * @brief Filter a ranking function by a value predicate.
 *
 * Returns a new ranking function containing only elements whose values satisfy
 * the given predicate. The filtering is **fully lazy** - the predicate is only
 * evaluated when elements are accessed. Ranks of filtered elements are preserved.
 *
 * This matches the Racket ranked-programming library's filter-ranking semantics:
 * - Predicate called only on accessed elements
 * - Elements not satisfying predicate are skipped
 * - Original ranks preserved for elements that pass
 * - Results memoized after first access
 *
 * @tparam T The value type in the ranking function.
 * @tparam Pred The predicate type (must be invocable with const T&).
 * @param rf The source ranking function.
 * @param predicate A function that returns true for values to keep.
 * @param deduplicate Whether to deduplicate consecutive equal values.
 * @return A new ranking function with only elements satisfying the predicate.
 *
 * @par Complexity
 * Time: O(1) to create the filtered ranking, O(n) to traverse all elements.
 * Space: O(1) for the initial call, O(depth) for recursive structure.
 *
 * @par Example
 * @code
 * auto rf = from_values_sequential({1, 2, 3, 4, 5});
 * auto evens = filter(rf, [](int x) { return x % 2 == 0; });
 * // evens contains {2, 4} with ranks {1, 3}
 * @endcode
 */
template<typename T, typename Pred>
requires std::invocable<Pred, const T&> && 
         std::same_as<std::invoke_result_t<Pred, const T&>, bool>
[[nodiscard]] RankingFunction<T> filter(
    const RankingFunction<T>& rf,
    Pred predicate,
    bool deduplicate = true)
{
    // Helper function to recursively build the filtered sequence
    // Use shared_ptr to allow safe capture in lazy computations
    using BuildFunc = std::function<std::shared_ptr<RankingElement<T>>(
        std::shared_ptr<RankingElement<T>>)>;
    auto build_filtered = std::make_shared<BuildFunc>();
    
    *build_filtered = [predicate, build_filtered](
        std::shared_ptr<RankingElement<T>> elem)
        -> std::shared_ptr<RankingElement<T>>
    {
        if (!elem) {
            return nullptr;
        }
        
        // Check if current element satisfies predicate (lazy evaluation)
        if (predicate(elem->value())) {
            // Keep this element - create lazy computation for next
            auto compute_next = [build_filtered, elem]() 
                -> std::shared_ptr<RankingElement<T>>
            {
                return (*build_filtered)(elem->next());
            };
            
            // Create new element with same value/rank, filtered next
            return std::make_shared<RankingElement<T>>(
                elem->value(),
                elem->rank(),
                make_promise(std::move(compute_next))
            );
        } else {
            // Skip this element - recursively try next
            return (*build_filtered)(elem->next());
        }
    };
    
    // Build the head of the filtered sequence
    auto filtered_head = (*build_filtered)(rf.head());
    
    return RankingFunction<T>(filtered_head, deduplicate);
}

/**
 * @brief Take only the first n elements from a ranking function.
 *
 * Returns a new ranking function containing at most the first n elements.
 * The operation is **fully lazy** - elements are only accessed as needed.
 * Ranks are preserved for all taken elements.
 *
 * This matches the Racket ranked-programming library's filter-after semantics.
 *
 * @tparam T The value type in the ranking function.
 * @param rf The source ranking function.
 * @param n The maximum number of elements to take.
 * @param deduplicate Whether to deduplicate consecutive equal values.
 * @return A new ranking function with at most n elements.
 *
 * @par Complexity
 * Time: O(1) to create the filtered ranking, O(min(n, m)) to traverse,
 *       where m is the size of the input.
 * Space: O(1) for the initial call, O(min(n, depth)) for recursive structure.
 *
 * @par Example
 * @code
 * auto rf = from_values_uniform({1, 2, 3, 4, 5});
 * auto first_three = take(rf, 3);
 * // first_three contains {1, 2, 3} with rank 0
 * @endcode
 */
template<typename T>
[[nodiscard]] RankingFunction<T> take(
    const RankingFunction<T>& rf,
    std::size_t n,
    bool deduplicate = true)
{
    // Base case: if n is 0, return empty ranking
    if (n == 0) {
        return RankingFunction<T>();
    }
    
    // Helper function to recursively build the taken sequence
    // Use shared_ptr to allow safe capture in lazy computations
    using BuildFunc = std::function<std::shared_ptr<RankingElement<T>>(
        std::shared_ptr<RankingElement<T>>, std::size_t)>;
    auto build_taken = std::make_shared<BuildFunc>();
    
    *build_taken = [build_taken](
        std::shared_ptr<RankingElement<T>> elem,
        std::size_t remaining)
        -> std::shared_ptr<RankingElement<T>>
    {
        // If no more elements to take or reached end, return nullptr
        if (remaining == 0 || !elem) {
            return nullptr;
        }
        
        // Create lazy computation for next element (with decremented count)
        auto compute_next = [build_taken, elem, remaining]() 
            -> std::shared_ptr<RankingElement<T>>
        {
            return (*build_taken)(elem->next(), remaining - 1);
        };
        
        // Create new element with same value/rank, limited next
        return std::make_shared<RankingElement<T>>(
            elem->value(),
            elem->rank(),
            make_promise(std::move(compute_next))
        );
    };
    
    // Build the head of the taken sequence
    auto taken_head = (*build_taken)(rf.head(), n);
    
    return RankingFunction<T>(taken_head, deduplicate);
}

/**
 * @brief Take elements while their rank is less than or equal to a threshold.
 *
 * Returns a new ranking function containing only elements with ranks <= max_rank.
 * The operation is **fully lazy** - elements are only accessed as needed.
 * Once an element with rank > max_rank is encountered, the sequence terminates.
 *
 * This matches the Racket ranked-programming library's up-to-rank semantics.
 *
 * @tparam T The value type in the ranking function.
 * @param rf The source ranking function.
 * @param max_rank The maximum rank threshold (inclusive).
 * @param deduplicate Whether to deduplicate consecutive equal values.
 * @return A new ranking function with only elements having rank <= max_rank.
 *
 * @par Complexity
 * Time: O(1) to create the filtered ranking, O(k) to traverse,
 *       where k is the number of elements with rank <= max_rank.
 * Space: O(1) for the initial call, O(depth) for recursive structure.
 *
 * @par Example
 * @code
 * auto rf = from_values_sequential({1, 2, 3, 4, 5});  // ranks 0,1,2,3,4
 * auto low_rank = take_while_rank(rf, Rank::from_value(2));
 * // low_rank contains {1, 2, 3} with ranks {0, 1, 2}
 * @endcode
 */
template<typename T>
[[nodiscard]] RankingFunction<T> take_while_rank(
    const RankingFunction<T>& rf,
    Rank max_rank,
    bool deduplicate = true)
{
    // Helper function to recursively build the rank-filtered sequence
    // Use shared_ptr to allow safe capture in lazy computations
    using BuildFunc = std::function<std::shared_ptr<RankingElement<T>>(
        std::shared_ptr<RankingElement<T>>)>;
    auto build_filtered = std::make_shared<BuildFunc>();
    
    *build_filtered = [max_rank, build_filtered](
        std::shared_ptr<RankingElement<T>> elem)
        -> std::shared_ptr<RankingElement<T>>
    {
        // If reached end or rank exceeds threshold, terminate
        if (!elem || elem->rank() > max_rank) {
            return nullptr;
        }
        
        // Keep this element - create lazy computation for next
        auto compute_next = [build_filtered, elem]() 
            -> std::shared_ptr<RankingElement<T>>
        {
            return (*build_filtered)(elem->next());
        };
        
        // Create new element with same value/rank, filtered next
        return std::make_shared<RankingElement<T>>(
            elem->value(),
            elem->rank(),
            make_promise(std::move(compute_next))
        );
    };
    
    // Build the head of the rank-filtered sequence
    auto filtered_head = (*build_filtered)(rf.head());
    
    return RankingFunction<T>(filtered_head, deduplicate);
}

}  // namespace ranked_belief

#endif  // RANKED_BELIEF_OPERATIONS_FILTER_HPP
