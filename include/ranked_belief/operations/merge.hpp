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
    Deduplication deduplicate = Deduplication::Enabled)
{
    // Special case: if both ranking functions have the same head, return one of them
    // This handles merging a sequence with itself, which should produce the same sequence
    if (rf1.head() == rf2.head()) {
        if (deduplicate == Deduplication::Disabled) {
            // If deduplication is off, make a lazy deep copy of rf2 and merge rf1 with the copy directly
            auto rf2_copy_head = lazy_deepcopy_ranking_sequence<T>(rf2.head());
            // Do NOT call merge recursively with rf1 and rf2_copy, as that can reintroduce shared structure
            // Instead, merge rf1 and rf2_copy using a local merge implementation that never compares pointer-equal nodes
            auto build_merged_impl = [](
                auto&& self,
                std::shared_ptr<RankingElement<T>> elem1,
                std::shared_ptr<RankingElement<T>> elem2,
                Rank seen_rank) -> std::shared_ptr<RankingElement<T>>
            {
                if (!elem1) return elem2;
                if (!elem2) return elem1;
                // Do NOT check for pointer equality here: after deep copy, all nodes are unique
                if (elem1->rank() == seen_rank) {
                    auto elem1_rank = elem1->rank();
                    // Create a promise wrapper to avoid moving from elem1
                    auto value_wrapper = make_promise([elem1]() { return elem1->value(); });
                    auto compute_next = [self, elem1, elem2, seen_rank]() {
                        auto next_elem1 = elem1->next();
                        return self(self, next_elem1, elem2, seen_rank);
                    };
                    return make_lazy_node(
                        std::move(value_wrapper),
                        elem1_rank,
                        make_promise(std::move(compute_next))
                    );
                }
                if (elem1->rank() <= elem2->rank()) {
                    auto elem1_rank = elem1->rank();
                    // Create a promise wrapper to avoid moving from elem1
                    auto value_wrapper = make_promise([elem1]() { return elem1->value(); });
                    auto compute_next = [self, elem1, elem2, elem1_rank]() {
                        auto next_elem1 = elem1->next();
                        return self(self, next_elem1, elem2, elem1_rank);
                    };
                    return make_lazy_node(
                        std::move(value_wrapper),
                        elem1_rank,
                        make_promise(std::move(compute_next))
                    );
                } else {
                    auto elem2_rank = elem2->rank();
                    // Create a promise wrapper to avoid moving from elem2
                    auto value_wrapper = make_promise([elem2]() { return elem2->value(); });
                    auto compute_next = [self, elem1, elem2, elem2_rank]() {
                        auto next_elem2 = elem2->next();
                        return self(self, elem1, next_elem2, elem2_rank);
                    };
                    return make_lazy_node(
                        std::move(value_wrapper),
                        elem2_rank,
                        make_promise(std::move(compute_next))
                    );
                }
            };
            auto merged_head = build_merged_impl(build_merged_impl, rf1.head(), rf2_copy_head, Rank::zero());
            return RankingFunction<T>(merged_head, Deduplication::Disabled);
        }
        return rf1;
    }
    
    // Helper function to recursively merge sequences
    auto build_merged_impl = [](
        auto&& self,
        std::shared_ptr<RankingElement<T>> elem1,
        std::shared_ptr<RankingElement<T>> elem2,
        Rank seen_rank)
        -> std::shared_ptr<RankingElement<T>>
    {
        // If both pointers are the same, just return one (avoid double-move)
        if (elem1 == elem2) {
            return elem1;
        }
        // If first sequence is exhausted, return second
        if (!elem1) {
            return elem2;
        }
        // If second sequence is exhausted, return first
        if (!elem2) {
            return elem1;
        }
        // If first element's rank equals seen_rank, take it immediately and continue with same seen_rank
        if (elem1->rank() == seen_rank) {
            auto elem1_rank = elem1->rank();
            // Create a promise wrapper to avoid moving from elem1
            auto value_wrapper = make_promise([elem1]() { return elem1->value(); });
            auto compute_next = [self](std::shared_ptr<RankingElement<T>> n1, std::shared_ptr<RankingElement<T>> n2, Rank r) -> std::shared_ptr<RankingElement<T>> {
                if (n1 == n2) return n1;
                return self(self, n1, n2, r);
            };
            return make_lazy_node(
                std::move(value_wrapper),
                elem1_rank,
                make_promise([elem1, elem2, seen_rank, compute_next]() {
                    auto next_elem1 = elem1->next();
                    if (next_elem1 == elem2) return next_elem1;
                    return compute_next(next_elem1, elem2, seen_rank);
                })
            );
        }
        // Compare ranks to decide which element comes next
        if (elem1->rank() <= elem2->rank()) {
            auto elem1_rank = elem1->rank();
            // Create a promise wrapper to avoid moving from elem1
            auto value_wrapper = make_promise([elem1]() { return elem1->value(); });
            auto compute_next = [self](std::shared_ptr<RankingElement<T>> n1, std::shared_ptr<RankingElement<T>> n2, Rank r) -> std::shared_ptr<RankingElement<T>> {
                if (n1 == n2) return n1;
                return self(self, n1, n2, r);
            };
            return make_lazy_node(
                std::move(value_wrapper),
                elem1_rank,
                make_promise([elem1, elem2, elem1_rank, compute_next]() {
                    auto next_elem1 = elem1->next();
                    if (next_elem1 == elem2) return next_elem1;
                    return compute_next(next_elem1, elem2, elem1_rank);
                })
            );
        } else {
            auto elem2_rank = elem2->rank();
            // Create a promise wrapper to avoid moving from elem2
            auto value_wrapper = make_promise([elem2]() { return elem2->value(); });
            auto compute_next = [self](std::shared_ptr<RankingElement<T>> n1, std::shared_ptr<RankingElement<T>> n2, Rank r) -> std::shared_ptr<RankingElement<T>> {
                if (n1 == n2) return n1;
                return self(self, n1, n2, r);
            };
            return make_lazy_node(
                std::move(value_wrapper),
                elem2_rank,
                make_promise([elem1, elem2, elem2_rank, compute_next]() {
                    auto next_elem2 = elem2->next();
                    if (elem1 == next_elem2) return elem1;
                    return compute_next(elem1, next_elem2, elem2_rank);
                })
            );
        }
    };
    // Start merge with both heads and rank 0
    auto merged_head = build_merged_impl(build_merged_impl, rf1.head(), rf2.head(), Rank::zero());
    
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
    Deduplication deduplicate = Deduplication::Enabled)
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
