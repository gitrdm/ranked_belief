/**
 * @file ranking_element.hpp
 * @brief Lazy linked list node for ranking sequences.
 *
 * RankingElement<T> represents a single element in a ranking function's lazy sequence.
 * Each element contains a value, its associated rank, and a lazily-evaluated pointer
 * to the next element in the sequence.
 *
 * This design allows for efficient representation of infinite ranking sequences, as
 * subsequent elements are only computed when accessed.
 */

#ifndef RANKED_BELIEF_RANKING_ELEMENT_HPP
#define RANKED_BELIEF_RANKING_ELEMENT_HPP

#include "promise.hpp"
#include "rank.hpp"

#include <memory>
#include <utility>

namespace ranked_belief {

// Forward declaration for the next element type
template<typename T>
class RankingElement;

/**
 * @brief Type alias for a lazy pointer to the next ranking element.
 *
 * The next element is wrapped in a Promise to enable lazy evaluation.
 * The shared_ptr allows multiple references to the same element in the sequence.
 */
template<typename T>
using LazyNext = Promise<std::shared_ptr<RankingElement<T>>>;

/**
 * @class RankingElement
 * @brief A node in a lazy linked list representing a ranking sequence.
 *
 * Each RankingElement contains:
 * - A value of type T
 * - A rank indicating how exceptional/normal the value is
 * - A lazy pointer to the next element (or nullptr for sequence end)
 *
 * The lazy evaluation of the next pointer is critical for representing infinite
 * sequences efficiently, as elements are only computed when accessed.
 *
 * Thread Safety: Individual elements are immutable after construction (value and rank).
 * The next pointer is lazily evaluated and thread-safe due to Promise's guarantees.
 *
 * @tparam T The type of values in the ranking sequence.
 *
 * Example:
 * @code
 * // Create a finite sequence: 1@0 -> 2@1 -> 3@2
 * auto elem3 = std::make_shared<RankingElement<int>>(3, Rank::from_value(2), 
 *                                                     make_promise_value(nullptr));
 * auto elem2 = std::make_shared<RankingElement<int>>(2, Rank::from_value(1),
 *                                                     make_promise_value(elem3));
 * auto elem1 = std::make_shared<RankingElement<int>>(1, Rank::zero(),
 *                                                     make_promise_value(elem2));
 * @endcode
 *
 * @invariant value_ and rank_ are immutable after construction.
 * @invariant next_ is evaluated at most once (Promise guarantee).
 */
template<typename T>
class RankingElement {
public:
    /**
     * @brief Construct a ranking element with a lazy next pointer.
     *
     * This is the primary constructor for building lazy sequences. The next
     * element is provided as a Promise that will be evaluated when accessed.
     *
     * @param value The value associated with this rank.
     * @param rank The rank of this value (0 = most normal, higher = more exceptional).
     * @param next A promise that produces the next element (or nullptr for end).
     */
    RankingElement(T value, Rank rank, LazyNext<T> next)
        : value_(std::move(value)), rank_(std::move(rank)), next_(std::move(next)) {}

    /**
     * @brief Construct a ranking element with an already-known next pointer.
     *
     * Convenience constructor when the next element is already available
     * (not lazily computed). Wraps the pointer in a forced promise.
     *
     * @param value The value associated with this rank.
     * @param rank The rank of this value.
     * @param next Pointer to the next element (or nullptr for end).
     */
    RankingElement(T value, Rank rank, std::shared_ptr<RankingElement<T>> next)
        : value_(std::move(value)),
          rank_(std::move(rank)),
          next_(make_promise_value(std::move(next))) {}

    /**
     * @brief Construct a terminal ranking element (last in sequence).
     *
     * Creates an element with no next element (end of sequence).
     *
     * @param value The value associated with this rank.
     * @param rank The rank of this value.
     */
    RankingElement(T value, Rank rank)
        : value_(std::move(value)),
          rank_(std::move(rank)),
          next_(make_promise_value(std::shared_ptr<RankingElement<T>>(nullptr))) {}

    // Ranking elements are immutable after construction, so disable copying and moving
    // to prevent accidental modifications and maintain clear ownership semantics.
    RankingElement(const RankingElement&) = delete;
    RankingElement& operator=(const RankingElement&) = delete;
    RankingElement(RankingElement&&) = delete;
    RankingElement& operator=(RankingElement&&) = delete;

    /**
     * @brief Get the value associated with this ranking element.
     *
     * @return Const reference to the value.
     */
    [[nodiscard]] const T& value() const noexcept { return value_; }

    /**
     * @brief Get the rank associated with this value.
     *
     * @return The rank (copy is lightweight as Rank is small).
     */
    [[nodiscard]] Rank rank() const noexcept { return rank_; }

    /**
     * @brief Get the next element in the sequence.
     *
     * This forces evaluation of the lazy next pointer. The result is memoized,
     * so subsequent calls return the same pointer without re-computation.
     *
     * Thread Safety: Safe to call from multiple threads concurrently due to
     * Promise's thread-safe memoization.
     *
     * @return Shared pointer to the next element, or nullptr if this is the last element.
     */
    [[nodiscard]] std::shared_ptr<RankingElement<T>> next() const {
        return next_.force();
    }

    /**
     * @brief Check if this is the last element in the sequence.
     *
     * This forces evaluation of the next pointer.
     *
     * @return true if next() would return nullptr, false otherwise.
     */
    [[nodiscard]] bool is_last() const { return next() == nullptr; }

    /**
     * @brief Check if the next element has been evaluated.
     *
     * @return true if next() has been called before, false otherwise.
     */
    [[nodiscard]] bool next_is_forced() const noexcept { return next_.is_forced(); }

private:
    T value_;             ///< The value at this position in the ranking
    Rank rank_;           ///< The rank of this value
    mutable LazyNext<T> next_;  ///< Lazy pointer to the next element (mutable for lazy evaluation)
};

/**
 * @brief Helper function to create a terminal ranking element.
 *
 * Provides a convenient way to create the last element in a sequence.
 *
 * @tparam T The type of the value.
 * @param value The value for the terminal element.
 * @param rank The rank of the value.
 * @return A shared pointer to the new terminal element.
 *
 * Example:
 * @code
 * auto last = make_terminal(42, Rank::from_value(5));
 * @endcode
 */
template<typename T>
[[nodiscard]] std::shared_ptr<RankingElement<T>> make_terminal(T value, Rank rank) {
    return std::make_shared<RankingElement<T>>(std::move(value), std::move(rank));
}

/**
 * @brief Helper function to create a ranking element with a known next element.
 *
 * Provides a convenient way to build sequences when all elements are known.
 *
 * @tparam T The type of the value.
 * @param value The value for this element.
 * @param rank The rank of the value.
 * @param next Pointer to the next element.
 * @return A shared pointer to the new element.
 *
 * Example:
 * @code
 * auto elem2 = make_terminal(2, Rank::from_value(1));
 * auto elem1 = make_element(1, Rank::zero(), elem2);
 * @endcode
 */
template<typename T>
[[nodiscard]] std::shared_ptr<RankingElement<T>> make_element(
    T value,
    Rank rank,
    std::shared_ptr<RankingElement<T>> next) {
    return std::make_shared<RankingElement<T>>(
        std::move(value),
        std::move(rank),
        std::move(next));
}

/**
 * @brief Helper function to create a ranking element with a lazy next computation.
 *
 * Provides a convenient way to build lazy sequences.
 *
 * @tparam T The type of the value.
 * @param value The value for this element.
 * @param rank The rank of the value.
 * @param next_computation Function that produces the next element when called.
 * @return A shared pointer to the new element.
 *
 * Example:
 * @code
 * auto elem = make_lazy_element(1, Rank::zero(), []() {
 *     return make_terminal(2, Rank::from_value(1));
 * });
 * @endcode
 */
template<typename T, typename F>
[[nodiscard]] std::shared_ptr<RankingElement<T>> make_lazy_element(
    T value,
    Rank rank,
    F&& next_computation) {
    return std::make_shared<RankingElement<T>>(
        std::move(value),
        std::move(rank),
        make_promise(std::forward<F>(next_computation)));
}

/**
 * @brief Helper function to create an infinite ranking sequence.
 *
 * Creates a lazy infinite sequence where each element is generated on demand.
 * Useful for representing unbounded ranking functions.
 *
 * @tparam T The type of values in the sequence.
 * @tparam Generator A callable that takes an index and returns std::pair<T, Rank>.
 * @param generator Function that generates the (value, rank) pair for index i.
 * @param start_index The starting index (default 0).
 * @return A shared pointer to the first element of the infinite sequence.
 *
 * Example:
 * @code
 * // Infinite sequence: 0@0, 1@1, 2@2, ...
 * auto infinite = make_infinite_sequence<int>([](size_t i) {
 *     return std::make_pair(static_cast<int>(i), Rank::from_value(i));
 * });
 * @endcode
 */
template<typename T, typename Generator>
[[nodiscard]] std::shared_ptr<RankingElement<T>> make_infinite_sequence(
    Generator generator,
    size_t start_index = 0) {
    auto [value, rank] = generator(start_index);
    
    return std::make_shared<RankingElement<T>>(
        std::move(value),
        std::move(rank),
        make_promise([generator, index = start_index + 1]() mutable {
            return make_infinite_sequence<T>(generator, index);
        }));
}

}  // namespace ranked_belief

#endif  // RANKED_BELIEF_RANKING_ELEMENT_HPP
