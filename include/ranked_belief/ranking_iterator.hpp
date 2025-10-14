/**
 * @file ranking_iterator.hpp
 * @brief Input iterator for traversing ranking sequences with optional deduplication.
 *
 * This file provides RankingIterator<T>, a C++20-compatible input iterator that
 * traverses lazy ranking sequences represented as linked lists of RankingElement<T> nodes.
 * The iterator supports optional deduplication of consecutive equal values, which is
 * essential for normalized ranking functions in the ranked belief framework.
 *
 * Key features:
 * - Input iterator semantics (single-pass, read-only)
 * - Lazy evaluation: only forces computation when iterator is advanced
 * - Optional deduplication: controlled by constructor flag
 * - Sentinel support: nullptr represents end of sequence
 * - C++20 ranges compatibility
 * - Thread-safe element access (via Promise memoization in RankingElement)
 *
 * @note Deduplication compares values using operator==. When enabled, consecutive
 * elements with equal values are skipped, keeping only the first occurrence
 * (which will have the minimum rank due to ranking function invariants).
 */

#ifndef RANKED_BELIEF_RANKING_ITERATOR_HPP
#define RANKED_BELIEF_RANKING_ITERATOR_HPP

#include "ranking_element.hpp"
#include "detail/any_equality_registry.hpp"
#include "types.hpp"
#include <any>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>

namespace ranked_belief {

/**
 * @brief Input iterator for traversing ranking sequences.
 *
 * RankingIterator provides C++20 input iterator interface over lazy ranking sequences.
 * It traverses a linked list of RankingElement<T> nodes, optionally deduplicating
 * consecutive equal values.
 *
 * The iterator is lightweight and copyable, maintaining only a shared_ptr to the
 * current element and a deduplication flag. Advancing the iterator forces evaluation
 * of lazy next pointers as needed.
 *
 * @tparam T The value type stored in the ranking sequence. Must be equality-comparable
 *           if deduplication is enabled.
 *
 * @note This is an input iterator (single-pass, read-only). It does not support
 *       multi-pass guarantees of forward iterators.
 *
 * Example usage:
 * @code
 * auto seq = make_lazy_element(1, Rank::zero(), []() {
 *     return make_terminal(2, Rank::from_value(1));
 * });
 * 
 * RankingIterator<int> it(seq, true);  // with deduplication
 * RankingIterator<int> end;            // end sentinel
 * 
 * for (; it != end; ++it) {
 *     auto [value, rank] = *it;
 *     std::cout << value << " @ rank " << rank << "\n";
 * }
 * @endcode
 */
template<typename T>
class RankingIterator {
public:
    // C++20 iterator traits
    using iterator_category = std::input_iterator_tag;
    using value_type = std::pair<T, Rank>;
    using difference_type = std::ptrdiff_t;
    using pointer = const value_type*;
    using reference = const value_type&;

    /**
     * @brief Construct an end sentinel iterator.
     *
     * Creates an iterator representing the end of a sequence (nullptr).
     * This is used for end comparisons in range-based for loops.
     */
    RankingIterator() noexcept 
        : current_(nullptr)
        , deduplicate_(false) 
    {}

    /**
     * @brief Construct an iterator starting at the given element.
     *
     * Creates an iterator positioned at the specified ranking element.
     * The first element is never considered a duplicate (there's nothing
     * before it to be a duplicate of), so no skipping occurs in the constructor.
     *
     * @param start The starting element (nullptr for end sentinel)
     * @param deduplicate If Enabled, skip consecutive elements with equal values
     */
    explicit RankingIterator(std::shared_ptr<RankingElement<T>> start, 
                            Deduplication deduplicate = Deduplication::Enabled)
        : current_(std::move(start))
        , deduplicate_(to_bool(deduplicate)) 
    {
        // Constructor does not skip - first element is always valid
    }

    /**
     * @brief Dereference the iterator to access the current value-rank pair.
     *
     * Returns a pair containing the value and rank of the current element.
     * The pair is constructed on-demand from the underlying element.
     *
     * @return std::pair<T, Rank> containing the current value and rank
     *
     * @pre Iterator must not be at end (current_ != nullptr)
     * @note Undefined behavior if called on end sentinel
     */
    [[nodiscard]] value_type operator*() const {
        return std::make_pair(current_->value(), current_->rank());
    }

    /**
     * @brief Pre-increment operator: advance to the next element.
     *
     * Moves the iterator to the next element in the sequence. If deduplication
     * is enabled, continues advancing past any elements with the same value
     * as the current element, effectively skipping duplicates.
     *
     * This operation forces evaluation of lazy next pointers via Promise::force().
     *
     * @return Reference to this iterator after advancement
     *
     * @pre Iterator must not be at end (current_ != nullptr)
     * @note Undefined behavior if called on end sentinel
     */
    RankingIterator& operator++() {
        if (current_) {
            if (deduplicate_) {
                // Save current value, then skip all elements with this value
                const T current_value = current_->value();
                current_ = current_->next();
                skip_duplicates_of(current_value);
            } else {
                // Without deduplication, just move to next
                current_ = current_->next();
            }
        }
        return *this;
    }

    /**
     * @brief Post-increment operator: advance and return pre-increment copy.
     *
     * Creates a copy of the current iterator, advances this iterator, then
     * returns the copy. This follows standard iterator post-increment semantics.
     *
     * @return Copy of the iterator before advancement
     *
     * @pre Iterator must not be at end (current_ != nullptr)
     * @note Less efficient than pre-increment due to copy; prefer ++it over it++
     */
    RankingIterator operator++(int) {
        RankingIterator tmp = *this;
        ++(*this);
        return tmp;
    }

    /**
     * @brief Equality comparison operator.
     *
     * Two iterators are equal if they point to the same element. This is
     * determined by comparing the underlying shared_ptr instances.
     *
     * @param other The iterator to compare with
     * @return true if iterators point to the same element, false otherwise
     *
     * @note Deduplication flags are not compared; only element identity matters
     */
    [[nodiscard]] bool operator==(const RankingIterator& other) const noexcept {
        return current_ == other.current_;
    }

    /**
     * @brief Inequality comparison operator.
     *
     * Two iterators are not equal if they point to different elements.
     *
     * @param other The iterator to compare with
     * @return true if iterators point to different elements, false otherwise
     */
    [[nodiscard]] bool operator!=(const RankingIterator& other) const noexcept {
        return !(*this == other);
    }

    /**
     * @brief Check if iterator is at end of sequence.
     *
     * An iterator is at the end if its current element is nullptr.
     *
     * @return true if at end (current_ == nullptr), false otherwise
     */
    [[nodiscard]] bool is_end() const noexcept {
        return current_ == nullptr;
    }

    /**
     * @brief Get the current element pointer (for testing/debugging).
     *
     * Returns the underlying shared_ptr to the current RankingElement.
     * This is primarily useful for testing and debugging scenarios.
     *
     * @return Shared pointer to current element (may be nullptr)
     */
    [[nodiscard]] std::shared_ptr<RankingElement<T>> current() const noexcept {
        return current_;
    }

    /**
     * @brief Check if deduplication is enabled.
     *
     * @return true if iterator performs deduplication, false otherwise
     */
    [[nodiscard]] bool is_deduplicating() const noexcept {
        return deduplicate_;
    }

private:
    /**
     * @brief Skip consecutive elements with duplicate values.
     *
     * This function is called after advancing the iterator to ensure we don't
     * land on a duplicate of the previous value. It does NOT skip duplicates
     * of the current value; instead, it's used by operator++ to skip over
     * elements that match the value we just left.
     *
     * The deduplication works as follows:
     * 1. Constructor with deduplication does nothing (first element is never a duplicate)
     * 2. When operator++ advances, it saves the current value before moving
     * 3. After moving to next, it skips any elements matching that saved value
     * 
     * This ensures we keep the first occurrence of each unique value.
     *
     * @param prev_value The value to skip duplicates of
     *
     * @note This is only called by operator++ when deduplication is enabled
     */
    void skip_duplicates_of(const T& prev_value) {
        if constexpr (std::is_same_v<T, std::any>) {
            while (current_ && detail::any_values_equal(current_->value(), prev_value)) {
                current_ = current_->next();
            }
        } else if constexpr (requires(const T& lhs, const T& rhs) { lhs == rhs; }) {
            while (current_ && current_->value() == prev_value) {
                current_ = current_->next();
            }
        } else {
            (void)prev_value;
        }
    }

    /// Pointer to the current element in the ranking sequence
    std::shared_ptr<RankingElement<T>> current_;
    
    /// Flag controlling deduplication behavior
    bool deduplicate_;
};

} // namespace ranked_belief

#endif // RANKED_BELIEF_RANKING_ITERATOR_HPP
