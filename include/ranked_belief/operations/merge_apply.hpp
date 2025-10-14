/**
 * @file merge_apply.hpp
 * @brief Applicative merge operation for combining ranking functions.
 *
 * Implements merge-apply, which applies a function that returns ranking functions
 * to each element of an input ranking, then merges all results with shifted ranks.
 * This is similar to a monadic bind operation.
 *
 * @author ranked_belief
 * @date 2024
 */

#ifndef RANKED_BELIEF_OPERATIONS_MERGE_APPLY_HPP
#define RANKED_BELIEF_OPERATIONS_MERGE_APPLY_HPP

#include "ranked_belief/promise.hpp"
#include "ranked_belief/rank.hpp"
#include "ranked_belief/ranking_element.hpp"
#include "ranked_belief/ranking_function.hpp"
#include "ranked_belief/operations/merge.hpp"
#include <functional>
#include <memory>
#include <type_traits>

namespace ranked_belief {

// Helper trait to extract element type from RankingFunction<T>
template<typename RF>
struct ranking_function_element_type;

template<typename T>
struct ranking_function_element_type<RankingFunction<T>> {
    using type = T;
};

template<typename RF>
using ranking_function_element_type_t = typename ranking_function_element_type<RF>::type;

// Forward declaration for merge_with_ranks helper
namespace detail {
    template<typename T>
    using ElementPromisePtr = std::shared_ptr<Promise<std::shared_ptr<RankingElement<T>>>>;

    template<typename T>
    std::shared_ptr<RankingElement<T>> merge_with_ranks(
        std::shared_ptr<RankingElement<T>> first,
        Rank first_rank,
        ElementPromisePtr<T> second_promise,
        Rank second_min_rank);
}

/**
 * @brief Shift all ranks in a ranking function by a constant amount.
 *
 * Creates a new ranking function where every element's rank is increased by
 * the specified shift amount. This is fully lazy - ranks are computed on
 * demand during traversal.
 *
 * @tparam T The value type in the ranking function.
 * @param rf The ranking function to shift.
 * @param shift_amount The amount to add to each rank.
 * @return A new ranking function with shifted ranks.
 *
 * @par Complexity
 * Time: O(1) to create, O(n) to traverse all elements.
 * Space: O(1) for the initial call, O(depth) for recursive structure.
 *
 * @par Example
 * @code
 * auto rf = from_values_sequential({1, 2, 3});  // ranks: 0, 1, 2
 * auto shifted = shift_ranks(rf, Rank::from_value(10));  // ranks: 10, 11, 12
 * @endcode
 */
template<typename T>
[[nodiscard]] RankingFunction<T> shift_ranks(
    const RankingFunction<T>& rf,
    Rank shift_amount)
{
    if (shift_amount == Rank::zero()) {
        return rf;
    }
    
    // Use shared_ptr to allow capture in lazy computations
    using BuildFunc = std::function<std::shared_ptr<RankingElement<T>>(
        std::shared_ptr<RankingElement<T>>)>;
    auto build_shifted = std::make_shared<BuildFunc>();
    std::weak_ptr<BuildFunc> weak_build = build_shifted;
    
    *build_shifted = [weak_build, shift_amount](
        std::shared_ptr<RankingElement<T>> elem)
        -> std::shared_ptr<RankingElement<T>>
    {
        if (!elem) {
            return nullptr;
        }

        Rank new_rank = elem->rank() + shift_amount;

        auto build_ref = weak_build.lock();

        auto compute_value = [elem]() -> T {
            return elem->value();
        };

        auto compute_next = [weak_build, build_ref, elem]()
            -> std::shared_ptr<RankingElement<T>>
        {
            auto next_elem = elem->next();

            if (auto locked = weak_build.lock()) {
                return (*locked)(next_elem);
            }

            if (build_ref) {
                return (*build_ref)(next_elem);
            }
            return nullptr;
        };

        return std::make_shared<RankingElement<T>>(
            make_promise(std::move(compute_value)),
            new_rank,
            make_promise(std::move(compute_next))
        );
    };
    
    return RankingFunction<T>(
        (*build_shifted)(rf.head()),
        rf.is_deduplicating()
    );
}

/**
 * @brief Apply a function to each element and merge all resulting ranking functions.
 *
 * For each value `v` at rank `r` in the input ranking function, applies `func(v)`
 * which must return a RankingFunction\<U\>. All resulting ranking functions are
 * merged together, with their ranks shifted by `r` (added to the original ranks).
 *
 * This is the core operation for monadic composition of ranking functions,
 * similar to flatMap or bind in functional programming.
 *
 * The operation is **fully lazy** - functions are only applied and results merged
 * as elements are accessed during traversal.
 *
 * This matches the Racket ranked-programming library's merge-apply semantics:
 * - Apply function to each value in input ranking
 * - Function returns a ranking function
 * - Shift result ranks by input element's rank
 * - Merge all results maintaining rank order
 * - Fully lazy evaluation
 *
 * @tparam T The input value type.
 * @tparam Func The function type (must return RankingFunction\<U\>).
 * @tparam U The output value type (deduced from Func return type).
 * @param rf The input ranking function.
 * @param func A function taking const T& and returning RankingFunction\<U\>.
 * @param deduplicate Whether to deduplicate the result (default: true).
 * @return A new ranking function containing all merged results.
 *
 * @par Complexity
 * Time: O(1) to create the merge_apply, O(n*m) to traverse all elements where
 *       n is input size and m is average result size.
 * Space: O(1) for the initial call, O(depth) for recursive structure.
 *
 * @par Example
 * @code
 * // Create a ranking of values: 1@0, 2@1, 3@2
 * auto rf = from_values_sequential({1, 2, 3});
 * 
 * // For each value n, create ranking: n@0, n*10@1
 * auto result = merge_apply(rf, [](int n) {
 *     return from_list<int>({{n, Rank::zero()}, {n*10, Rank::from_value(1)}});
 * });
 * // Result: 1@0 (from 1), 10@1 (from 1), 2@1 (from 2), 20@2 (from 2),
 * //         3@2 (from 3), 30@3 (from 3)
 * @endcode
 */
template<typename T, typename Func>
[[nodiscard]] auto merge_apply(
    const RankingFunction<T>& rf,
    Func&& func,
    bool deduplicate = true)
    -> std::invoke_result_t<Func, const T&>
{
    using ResultRF = std::invoke_result_t<Func, const T&>;
    using U = ranking_function_element_type_t<ResultRF>;
    
    // Recursive builder function
    using BuildFunc = std::function<std::shared_ptr<RankingElement<U>>(
        std::shared_ptr<RankingElement<T>>)>;
    auto build_merge_apply = std::make_shared<BuildFunc>();
    std::weak_ptr<BuildFunc> weak_build = build_merge_apply;
    
    *build_merge_apply = [weak_build, func](
        std::shared_ptr<RankingElement<T>> elem)
        -> std::shared_ptr<RankingElement<U>>
    {
        if (!elem) {
            return nullptr;
        }

    auto build_ref = weak_build.lock();
        
    // Get current element's rank and value
        Rank current_rank = elem->rank();
        T current_value = elem->value();
        
        // Apply function to get result ranking
        RankingFunction<U> result_rf = func(current_value);
        
        // Shift result ranks by current element's rank
        RankingFunction<U> shifted_rf = shift_ranks(result_rf, current_rank);
        
        // Head of shifted result (may be null if function returns empty ranking)
        auto shifted_head = shifted_rf.head();

        // Prepare lazy computation for the rest of the input sequence
        auto next_input = elem->next();
        Rank rest_min_rank = next_input ? next_input->rank() : Rank::infinity();

        detail::ElementPromisePtr<U> rest_promise;
        if (next_input) {
            rest_promise = std::make_shared<Promise<std::shared_ptr<RankingElement<U>>>>(
                make_promise([weak_build, build_ref, next_input]() -> std::shared_ptr<RankingElement<U>> {
                    if (auto locked = weak_build.lock()) {
                        return (*locked)(next_input);
                    }

                    if (build_ref) {
                        return (*build_ref)(next_input);
                    }
                    return nullptr;
                })
            );
        }

        // Merge shifted results with the recursively computed rest using lazy ranks
        return detail::merge_with_ranks(
            shifted_head,
            shifted_head ? shifted_head->rank() : Rank::infinity(),
            rest_promise,
            rest_min_rank
        );
    };
    
    return RankingFunction<U>(
        (*build_merge_apply)(rf.head()),
        deduplicate
    );
}

namespace detail {
    /**
     * @brief Merge two element sequences when their first elements' ranks are known.
     *
     * This is a lazy merge operation used by merge_apply. It takes the first
     * element from each sequence (which may be null) and their ranks, and returns
     * a merged sequence. The recursive calls are wrapped in Promises to maintain
     * lazy evaluation.
     *
     * @tparam T The value type.
    * @param first First element of the primary sequence (may be null).
    * @param first_rank Rank of the primary element (infinity if @p first is null).
    * @param second_promise Lazy thunk that can produce the head of the secondary sequence.
    * @param second_min_rank Lowest possible rank available from the secondary sequence.
     * @return Lazily merged sequence head.
     */
    template<typename T>
    std::shared_ptr<RankingElement<T>> merge_with_ranks(
        std::shared_ptr<RankingElement<T>> first,
        Rank first_rank,
        ElementPromisePtr<T> second_promise,
        Rank second_min_rank)
    {
        // If the first sequence is empty, defer to the second sequence
        if (!first) {
            return second_promise ? second_promise->force() : nullptr;
        }

        // If the second sequence cannot produce any elements (infinite rank), take from first
        if (second_min_rank == Rank::infinity() || !second_promise) {
            return first;
        }

        // If the first element's rank is lower or equal to the minimum possible
        // rank of the second sequence, take from the first sequence without
        // forcing the second sequence yet.
        if (first_rank <= second_min_rank) {
            auto compute_next = [first, second_promise, second_min_rank]()
                -> std::shared_ptr<RankingElement<T>>
            {
                auto first_next = first->next();
                Rank next_rank = first_next ? first_next->rank() : Rank::infinity();
                return merge_with_ranks(first_next, next_rank, second_promise, second_min_rank);
            };

            return std::make_shared<RankingElement<T>>(
                first->value(),
                first_rank,
                make_promise(std::move(compute_next))
            );
        }

        // Otherwise, force the second sequence to obtain its head (needed now)
        auto second = second_promise->force();
        if (!second) {
            return first;
        }

        auto second_next = second->next();
        Rank second_next_min_rank = second_next ? second_next->rank() : Rank::infinity();

        ElementPromisePtr<T> next_second_promise;
        if (second_next) {
            next_second_promise = std::make_shared<Promise<std::shared_ptr<RankingElement<T>>>>(
                make_promise([second_next]() -> std::shared_ptr<RankingElement<T>> {
                    return second_next;
                })
            );
        }

        auto compute_next = [first, first_rank, next_second_promise, second_next_min_rank]()
            -> std::shared_ptr<RankingElement<T>>
        {
            return merge_with_ranks(first, first_rank, next_second_promise, second_next_min_rank);
        };

        return std::make_shared<RankingElement<T>>(
            second->value(),
            second->rank(),
            make_promise(std::move(compute_next))
        );
    }
}

} // namespace ranked_belief

#endif // RANKED_BELIEF_OPERATIONS_MERGE_APPLY_HPP
