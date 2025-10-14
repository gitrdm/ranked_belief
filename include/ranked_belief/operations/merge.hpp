/**
 * @file merge.hpp
 * @brief Lazy merge operations for combining RankingFunctions.
 *
 * Provides operations to merge multiple ranking functions while maintaining
 * proper rank ordering. All operations preserve lazy evaluation semantics.
 *
 * @author ranked_belief
 * @date 2024
 */

#ifndef RANKED_BELIEF_OPERATIONS_MERGE_HPP
#define RANKED_BELIEF_OPERATIONS_MERGE_HPP

#include "ranked_belief/promise.hpp"
#include "ranked_belief/rank.hpp"
#include "ranked_belief/ranking_element.hpp"
#include "ranked_belief/ranking_function.hpp"
#include <functional>
#include <memory>
#include <vector>

namespace ranked_belief {

/**
 * @brief Merge two ranking functions in rank order.
 *
 * Combines two ranking functions into a single sequence with elements ordered
 * by rank. When both functions have elements at the same rank, elements from
 * the first function appear before elements from the second.
 *
 * The merge is **fully lazy** - elements are only computed as needed during
 * traversal. This enables merging infinite sequences.
 *
 * This matches the Racket ranked-programming library's merge semantics:
 * - Elements interleaved by rank (ascending order)
 * - Elements at same rank: first function takes precedence
 * - Lazy evaluation throughout
 * - Memoization after first access
 *
 * @tparam T The value type in the ranking functions.
 * @param rf1 The first ranking function.
 * @param rf2 The second ranking function.
 * @param deduplicate Whether to deduplicate consecutive equal values.
 * @return A new ranking function containing all elements from both inputs,
 *         ordered by rank.
 *
 * @par Complexity
 * Time: O(1) to create the merge, O(n+m) to traverse all elements.
 * Space: O(1) for the initial call, O(depth) for recursive structure.
 *
 * @par Example
 * @code
 * auto rf1 = from_list<int>({{1, Rank::from_value(0)}, {3, Rank::from_value(2)}});
 * auto rf2 = from_list<int>({{2, Rank::from_value(1)}, {4, Rank::from_value(3)}});
 * auto merged = merge(rf1, rf2);
 * // merged contains: 1@0, 2@1, 3@2, 4@3
 * @endcode
 */
template<typename T>
[[nodiscard]] RankingFunction<T> merge(
    const RankingFunction<T>& rf1,
    const RankingFunction<T>& rf2,
    bool deduplicate = true)
{
    // Helper function to recursively merge sequences
    // Uses shared_ptr to allow safe capture in lazy computations
    using BuildFunc = std::function<std::shared_ptr<RankingElement<T>>(
        std::shared_ptr<RankingElement<T>>,
        std::shared_ptr<RankingElement<T>>,
        Rank)>;
    auto build_merged = std::make_shared<BuildFunc>();
    std::weak_ptr<BuildFunc> weak_build = build_merged;
    
    *build_merged = [weak_build](
        std::shared_ptr<RankingElement<T>> elem1,
        std::shared_ptr<RankingElement<T>> elem2,
        Rank seen_rank)
        -> std::shared_ptr<RankingElement<T>>
    {
        // If first sequence is exhausted, return second
        if (!elem1) {
            return elem2;
        }

        auto build_ref = weak_build.lock();
        
        // If first element's rank equals seen_rank, take it immediately and continue with same seen_rank
        // This handles continuing to consume elements from elem1 at the same rank
        if (elem1->rank() == seen_rank) {
            auto compute_next = [weak_build, build_ref, elem1, elem2, seen_rank]()
                -> std::shared_ptr<RankingElement<T>>
            {
                auto next_elem1 = elem1->next();

                if (auto locked = weak_build.lock()) {
                    return (*locked)(next_elem1, elem2, seen_rank);
                }

                if (build_ref) {
                    return (*build_ref)(next_elem1, elem2, seen_rank);
                }
                return nullptr;
            };
            
            return make_lazy_node(
                std::move(*elem1).extract_value_promise(),
                elem1->rank(),
                make_promise(std::move(compute_next))
            );
        }
        
        // If second sequence is exhausted, return first
        if (!elem2) {
            return elem1;
        }
        
        // Compare ranks to decide which element comes next
        if (elem1->rank() <= elem2->rank()) {
            // Take element from first sequence and update seen_rank to its rank
            auto elem1_rank = elem1->rank();
            auto compute_next = [weak_build, build_ref, elem1, elem2, elem1_rank]()
                -> std::shared_ptr<RankingElement<T>>
            {
                auto next_elem1 = elem1->next();

                if (auto locked = weak_build.lock()) {
                    return (*locked)(next_elem1, elem2, elem1_rank);
                }

                if (build_ref) {
                    return (*build_ref)(next_elem1, elem2, elem1_rank);
                }
                return nullptr;
            };
            
            return make_lazy_node(
                std::move(elem1->value_promise()),
                elem1_rank,
                make_promise(std::move(compute_next))
            );
        } else {
            // Take element from second sequence and update seen_rank to its rank
            auto elem2_rank = elem2->rank();
            auto compute_next = [weak_build, build_ref, elem1, elem2, elem2_rank]()
                -> std::shared_ptr<RankingElement<T>>
            {
                auto next_elem2 = elem2->next();

                if (auto locked = weak_build.lock()) {
                    return (*locked)(elem1, next_elem2, elem2_rank);
                }

                if (build_ref) {
                    return (*build_ref)(elem1, next_elem2, elem2_rank);
                }
                return nullptr;
            };
            
            return make_lazy_node(
                std::move(elem2->value_promise()),
                elem2_rank,
                make_promise(std::move(compute_next))
            );
        }
    };
    
    // Start merge with both heads and rank 0
    auto merged_head = (*build_merged)(rf1.head(), rf2.head(), Rank::zero());
    
    return RankingFunction<T>(merged_head, deduplicate);
}

/**
 * @brief Merge multiple ranking functions in rank order.
 *
 * Combines any number of ranking functions into a single sequence with
 * elements ordered by rank. Elements at the same rank are ordered by their
 * position in the input vector (earlier functions take precedence).
 *
 * The merge is **fully lazy** - elements are only computed as needed.
 *
 * This matches the Racket ranked-programming library's merge-list semantics.
 *
 * @tparam T The value type in the ranking functions.
 * @param rankings Vector of ranking functions to merge.
 * @param deduplicate Whether to deduplicate consecutive equal values.
 * @return A new ranking function containing all elements from all inputs,
 *         ordered by rank.
 *
 * @par Complexity
 * Time: O(k) to create the merge where k is the number of ranking functions,
 *       O(n1+n2+...+nk) to traverse all elements.
 * Space: O(k) for the merge structure.
 *
 * @par Example
 * @code
 * auto rf1 = from_list<int>({{1, Rank::zero()}, {4, Rank::from_value(3)}});
 * auto rf2 = from_list<int>({{2, Rank::from_value(1)}});
 * auto rf3 = from_list<int>({{3, Rank::from_value(2)}});
 * auto merged = merge_all({rf1, rf2, rf3});
 * // merged contains: 1@0, 2@1, 3@2, 4@3
 * @endcode
 */
template<typename T>
[[nodiscard]] RankingFunction<T> merge_all(
    const std::vector<RankingFunction<T>>& rankings,
    bool deduplicate = true)
{
    if (rankings.empty()) {
        return RankingFunction<T>();
    }
    
    if (rankings.size() == 1) {
        return rankings[0];
    }
    
    // Iteratively merge all ranking functions
    // Start with first, then merge each subsequent one
    RankingFunction<T> result = rankings[0];
    for (std::size_t i = 1; i < rankings.size(); ++i) {
        result = merge(result, rankings[i], deduplicate);
    }
    
    return result;
}

}  // namespace ranked_belief

#endif  // RANKED_BELIEF_OPERATIONS_MERGE_HPP
