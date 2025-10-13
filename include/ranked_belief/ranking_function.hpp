/**
 * @file ranking_function.hpp
 * @brief The main user-facing class for ranked belief programming.
 *
 * This file provides RankingFunction<T>, which represents a ranking function - 
 * a sequence of value-rank pairs ordered by increasing rank. Ranking functions
 * are the core abstraction in the ranked belief framework, representing 
 * probability distributions, belief states, or sets of alternatives ordered
 * by their normality/exceptionality.
 *
 * Key features:
 * - Lazy evaluation: sequences are computed on-demand
 * - Infinite sequences: can represent unbounded rankings efficiently
 * - Optional deduplication: consecutive equal values can be merged
 * - Range interface: compatible with C++20 ranges and algorithms
 * - Immutable semantics: ranking functions don't change after construction
 * - Memory efficient: shared storage via std::shared_ptr
 *
 * Invariants:
 * - Ranks are non-decreasing (rank[i] <= rank[i+1])
 * - With deduplication: all values are unique (no consecutive duplicates)
 * - Empty ranking functions have no elements
 *
 * @note This is the primary interface for users of the library. Most operations
 *       (map, filter, merge, etc.) will be implemented as functions that take
 *       and return RankingFunction instances.
 */

#ifndef RANKED_BELIEF_RANKING_FUNCTION_HPP
#define RANKED_BELIEF_RANKING_FUNCTION_HPP

#include "ranking_element.hpp"
#include "ranking_iterator.hpp"
#include <memory>
#include <optional>
#include <utility>

namespace ranked_belief {

/**
 * @brief A ranking function representing a sequence of ranked values.
 *
 * RankingFunction<T> is the main user-facing class in the ranked belief library.
 * It represents a lazy, potentially infinite sequence of (value, rank) pairs,
 * where ranks are non-decreasing. The sequence is stored as a linked list of
 * RankingElement<T> nodes, with lazy evaluation via Promise.
 *
 * The class provides a range interface (begin/end) that returns RankingIterator
 * instances, making it compatible with C++20 ranges and standard algorithms.
 * It also provides query methods for common operations like first() and is_empty().
 *
 * @tparam T The value type stored in the ranking function. Must be copyable and
 *           equality-comparable if deduplication is enabled.
 *
 * Example usage:
 * @code
 * // Create a simple ranking function
 * auto head = make_lazy_element(1, Rank::zero(), []() {
 *     return make_terminal(2, Rank::from_value(1));
 * });
 * RankingFunction<int> rf(head);
 * 
 * // Iterate using range-based for loop
 * for (auto [value, rank] : rf) {
 *     std::cout << value << " @ rank " << rank << "\n";
 * }
 * 
 * // Query operations
 * if (auto first = rf.first()) {
 *     auto [value, rank] = *first;
 *     std::cout << "First element: " << value << "\n";
 * }
 * @endcode
 */
template<typename T>
class RankingFunction {
public:
    /// Iterator type for this ranking function
    using iterator = RankingIterator<T>;
    
    /// Const iterator type (same as iterator since elements are immutable)
    using const_iterator = RankingIterator<T>;
    
    /// Value type yielded by iteration
    using value_type = std::pair<T, Rank>;

    /**
     * @brief Construct an empty ranking function.
     *
     * Creates a ranking function with no elements. is_empty() will return true,
     * first() will return std::nullopt, and begin() == end().
     */
    RankingFunction() noexcept
        : head_(nullptr)
        , deduplicate_(true)
    {}

    /**
     * @brief Construct a ranking function from a head element.
     *
     * Creates a ranking function starting at the given element. The element
     * and all its successors (accessed via next() pointers) form the sequence.
     *
     * @param head The first element of the sequence (nullptr for empty)
     * @param deduplicate If true, iterators will skip consecutive equal values
     *
     * @note The head element is stored via shared_ptr, so ownership is shared
     *       with the caller if they retain a reference.
     */
    explicit RankingFunction(std::shared_ptr<RankingElement<T>> head,
                            bool deduplicate = true) noexcept
        : head_(std::move(head))
        , deduplicate_(deduplicate)
    {}

    /**
     * @brief Get an iterator to the beginning of the sequence.
     *
     * Returns an iterator positioned at the first element of the ranking
     * function. If the ranking function is empty, returns an iterator equal
     * to end().
     *
     * The iterator will respect the deduplication flag set during construction.
     *
     * @return Iterator to the first element
     *
     * @note Multiple calls to begin() return independent iterators positioned
     *       at the start. Each iterator can advance independently.
     */
    [[nodiscard]] iterator begin() const noexcept {
        return iterator(head_, deduplicate_);
    }

    /**
     * @brief Get an iterator representing the end of the sequence.
     *
     * Returns a sentinel iterator that compares equal to any iterator that
     * has reached the end of the sequence (nullptr).
     *
     * @return End sentinel iterator
     */
    [[nodiscard]] iterator end() const noexcept {
        return iterator();  // Default constructor creates end sentinel
    }

    /**
     * @brief Get the first element of the ranking function.
     *
     * Returns the first (value, rank) pair if the ranking function is non-empty,
     * or std::nullopt if empty. This is more efficient than using begin() and
     * dereferencing when only the first element is needed.
     *
     * @return std::optional containing first element, or std::nullopt if empty
     *
     * @note With deduplication enabled, this always returns the first unique value,
     *       which will have the minimum rank among all occurrences.
     */
    [[nodiscard]] std::optional<value_type> first() const {
        if (!head_) {
            return std::nullopt;
        }
        return std::make_pair(head_->value(), head_->rank());
    }

    /**
     * @brief Check if the ranking function is empty.
     *
     * Returns true if the ranking function has no elements (head is nullptr),
     * false otherwise. This is a constant-time operation.
     *
     * @return true if empty, false if non-empty
     */
    [[nodiscard]] bool is_empty() const noexcept {
        return head_ == nullptr;
    }

    /**
     * @brief Check if deduplication is enabled.
     *
     * Returns true if iterators will skip consecutive equal values, false if
     * all elements will be visited.
     *
     * @return true if deduplication is enabled, false otherwise
     */
    [[nodiscard]] bool is_deduplicating() const noexcept {
        return deduplicate_;
    }

    /**
     * @brief Get the head element (for advanced use/testing).
     *
     * Returns the underlying shared_ptr to the first RankingElement. This is
     * primarily useful for testing, debugging, or advanced operations that
     * need direct access to the element structure.
     *
     * @return Shared pointer to head element (may be nullptr)
     */
    [[nodiscard]] std::shared_ptr<RankingElement<T>> head() const noexcept {
        return head_;
    }

    /**
     * @brief Get the number of elements in the ranking function.
     *
     * Counts the total number of elements by traversing the entire sequence.
     * With deduplication enabled, only counts unique values.
     *
     * @return Number of elements in the sequence
     *
     * @warning This operation forces evaluation of all lazy elements and may
     *          not terminate for infinite sequences. Use with caution.
     *
     * @note This is O(n) where n is the number of elements. For large or
     *       infinite sequences, this may be expensive or non-terminating.
     */
    [[nodiscard]] std::size_t size() const {
        std::size_t count = 0;
        for (auto it = begin(); it != end(); ++it) {
            ++count;
        }
        return count;
    }

    /**
     * @brief Compare two ranking functions for equality.
     *
     * Two ranking functions are equal if they point to the same head element.
     * This is a shallow comparison (pointer equality), not a deep comparison
     * of sequence contents.
     *
     * @param other The ranking function to compare with
     * @return true if both have the same head element, false otherwise
     *
     * @note This does not compare sequence contents. Two ranking functions
     *       with identical sequences but different head pointers will compare
     *       as unequal. For deep equality, use std::ranges::equal with iterators.
     */
    [[nodiscard]] bool operator==(const RankingFunction& other) const noexcept {
        return head_ == other.head_ && deduplicate_ == other.deduplicate_;
    }

    /**
     * @brief Compare two ranking functions for inequality.
     *
     * @param other The ranking function to compare with
     * @return true if ranking functions differ, false if equal
     */
    [[nodiscard]] bool operator!=(const RankingFunction& other) const noexcept {
        return !(*this == other);
    }

private:
    /// The first element of the ranking sequence (nullptr for empty)
    std::shared_ptr<RankingElement<T>> head_;
    
    /// Flag controlling whether iterators deduplicate consecutive equal values
    bool deduplicate_;
};

/**
 * @brief Create an empty ranking function.
 *
 * Convenience factory function for creating an empty RankingFunction<T>.
 * Equivalent to RankingFunction<T>() but with type deduction from context.
 *
 * @tparam T The value type (typically inferred from usage context)
 * @return An empty ranking function
 *
 * Example:
 * @code
 * auto empty = make_empty_ranking<int>();
 * assert(empty.is_empty());
 * @endcode
 */
template<typename T>
[[nodiscard]] RankingFunction<T> make_empty_ranking() noexcept {
    return RankingFunction<T>();
}

/**
 * @brief Create a ranking function from a head element.
 *
 * Convenience factory function for creating a RankingFunction<T> from
 * a head element. Type T is deduced from the element type.
 *
 * @tparam T The value type (deduced from head)
 * @param head The first element of the sequence
 * @param deduplicate If true, enable deduplication (default: true)
 * @return A ranking function starting at head
 *
 * Example:
 * @code
 * auto head = make_terminal(42, Rank::zero());
 * auto rf = make_ranking_function(head);
 * @endcode
 */
template<typename T>
[[nodiscard]] RankingFunction<T> make_ranking_function(
    std::shared_ptr<RankingElement<T>> head,
    bool deduplicate = true) noexcept
{
    return RankingFunction<T>(std::move(head), deduplicate);
}

/**
 * @brief Create a single-element ranking function.
 *
 * Convenience factory for creating a ranking function with exactly one element.
 *
 * @tparam T The value type (deduced from value)
 * @param value The single value in the ranking
 * @param rank The rank of the value (default: Rank::zero())
 * @return A ranking function containing only the given value-rank pair
 *
 * Example:
 * @code
 * auto rf = make_singleton_ranking(42, Rank::from_value(5));
 * assert(!rf.is_empty());
 * assert(rf.first()->first == 42);
 * @endcode
 */
template<typename T>
[[nodiscard]] RankingFunction<T> make_singleton_ranking(
    T value,
    Rank rank = Rank::zero()) noexcept
{
    auto head = make_terminal(std::move(value), rank);
    return RankingFunction<T>(std::move(head), true);
}

} // namespace ranked_belief

#endif // RANKED_BELIEF_RANKING_FUNCTION_HPP
