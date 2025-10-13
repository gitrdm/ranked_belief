/**
 * @file rank.hpp
 * @brief Type-safe representation of ranks in the ranked belief framework.
 *
 * Ranks measure the degree of normality/exceptionality of values in a ranking function.
 * A rank of 0 represents the most normal case, while higher ranks indicate increasing
 * exceptionality. Infinity represents impossibility.
 *
 * This implementation provides a type-safe wrapper around uint64_t with special handling
 * for infinity and arithmetic operations that respect rank semantics.
 */

#ifndef RANKED_BELIEF_RANK_HPP
#define RANKED_BELIEF_RANK_HPP

#include <compare>
#include <cstdint>
#include <limits>
#include <ostream>
#include <stdexcept>

namespace ranked_belief {

/**
 * @class Rank
 * @brief Type-safe representation of a rank value.
 *
 * Ranks are non-negative integers from 0 to infinity, where:
 * - 0 represents the most normal (expected) outcome
 * - Higher finite values represent increasingly exceptional outcomes
 * - Infinity represents impossible outcomes
 *
 * Arithmetic operations are defined with the following semantics:
 * - Addition: Used for combining independent ranking functions (merge-apply)
 * - Minimum: Used for merging alternative ranking functions (merge)
 * - Infinity is an absorbing element for addition
 *
 * @invariant The internal value is either a valid uint64_t or the infinity flag is set.
 */
class Rank {
public:
    /**
     * @brief Construct a rank with value zero (most normal).
     * @return A rank representing the most normal outcome.
     */
    [[nodiscard]] static constexpr Rank zero() noexcept { return Rank(0, false); }

    /**
     * @brief Construct a rank representing infinity (impossibility).
     * @return A rank representing an impossible outcome.
     */
    [[nodiscard]] static constexpr Rank infinity() noexcept { return Rank(0, true); }

    /**
     * @brief Construct a rank from a finite non-negative integer value.
     * @param value The rank value (must be less than max_finite_value()).
     * @return A rank with the specified value.
     * @throws std::invalid_argument if value >= max_finite_value().
     */
    [[nodiscard]] static Rank from_value(uint64_t value) {
        if (value >= max_finite_value()) {
            throw std::invalid_argument(
                "Rank value must be less than max_finite_value() (2^63). Use Rank::infinity() "
                "for infinite ranks.");
        }
        return Rank(value, false);
    }

    /**
     * @brief Get the maximum representable finite rank value.
     *
     * This is 2^63 - 1, reserving the upper half of uint64_t range for potential
     * future use and ensuring arithmetic operations have headroom before overflow.
     *
     * @return The maximum finite rank value (9223372036854775807).
     */
    [[nodiscard]] static constexpr uint64_t max_finite_value() noexcept {
        return std::numeric_limits<int64_t>::max();
    }

    /**
     * @brief Default constructor creates rank zero.
     */
    constexpr Rank() noexcept : value_(0), is_infinity_(false) {}

    /**
     * @brief Check if this rank represents infinity.
     * @return true if this rank is infinite, false otherwise.
     */
    [[nodiscard]] constexpr bool is_infinity() const noexcept { return is_infinity_; }

    /**
     * @brief Check if this rank is finite.
     * @return true if this rank is not infinite, false otherwise.
     */
    [[nodiscard]] constexpr bool is_finite() const noexcept { return !is_infinity_; }

    /**
     * @brief Get the numeric value of this rank.
     *
     * @return The rank value if finite.
     * @throws std::logic_error if this rank is infinite.
     */
    [[nodiscard]] uint64_t value() const {
        if (is_infinity_) {
            throw std::logic_error("Cannot get numeric value of infinite rank");
        }
        return value_;
    }

    /**
     * @brief Get the numeric value or a default if infinite.
     * @param default_value The value to return if this rank is infinite.
     * @return The rank value if finite, otherwise default_value.
     */
    [[nodiscard]] constexpr uint64_t value_or(uint64_t default_value) const noexcept {
        return is_infinity_ ? default_value : value_;
    }

    /**
     * @brief Add two ranks.
     *
     * Addition semantics:
     * - finite + finite = finite (with overflow detection)
     * - finite + infinity = infinity
     * - infinity + anything = infinity
     *
     * @param other The rank to add.
     * @return The sum of the two ranks.
     * @throws std::overflow_error if the sum would exceed max_finite_value().
     */
    [[nodiscard]] Rank operator+(const Rank& other) const {
        if (is_infinity_ || other.is_infinity_) {
            return infinity();
        }

        // Check for overflow before performing addition
        if (value_ > max_finite_value() - other.value_) {
            throw std::overflow_error("Rank addition would overflow max_finite_value()");
        }

        return Rank(value_ + other.value_, false);
    }

    /**
     * @brief Compute the minimum of two ranks.
     *
     * Minimum semantics:
     * - min(a, b) = the smaller finite value
     * - min(finite, infinity) = finite
     * - min(infinity, infinity) = infinity
     *
     * @param other The rank to compare with.
     * @return The smaller of the two ranks.
     */
    [[nodiscard]] constexpr Rank min(const Rank& other) const noexcept {
        if (is_infinity_) {
            return other;
        }
        if (other.is_infinity_) {
            return *this;
        }
        return value_ <= other.value_ ? *this : other;
    }

    /**
     * @brief Compute the maximum of two ranks.
     *
     * Maximum semantics:
     * - max(a, b) = the larger finite value
     * - max(finite, infinity) = infinity
     * - max(infinity, infinity) = infinity
     *
     * @param other The rank to compare with.
     * @return The larger of the two ranks.
     */
    [[nodiscard]] constexpr Rank max(const Rank& other) const noexcept {
        if (is_infinity_ || other.is_infinity_) {
            return infinity();
        }
        return value_ >= other.value_ ? *this : other;
    }

    /**
     * @brief Subtract a rank from this rank.
     *
     * Subtraction is only defined for finite ranks where this >= other.
     *
     * @param other The rank to subtract.
     * @return The difference (this - other).
     * @throws std::logic_error if either rank is infinite.
     * @throws std::underflow_error if this < other.
     */
    [[nodiscard]] Rank operator-(const Rank& other) const {
        if (is_infinity_ || other.is_infinity_) {
            throw std::logic_error("Cannot subtract infinite ranks");
        }
        if (value_ < other.value_) {
            throw std::underflow_error("Rank subtraction would result in negative value");
        }
        return Rank(value_ - other.value_, false);
    }

    /**
     * @brief Three-way comparison operator.
     *
     * Ordering: 0 < 1 < 2 < ... < infinity
     *
     * @param other The rank to compare with.
     * @return std::strong_ordering indicating the relationship.
     */
    [[nodiscard]] constexpr std::strong_ordering operator<=>(const Rank& other) const noexcept {
        // Infinity is greater than any finite value
        if (is_infinity_ && other.is_infinity_) {
            return std::strong_ordering::equal;
        }
        if (is_infinity_) {
            return std::strong_ordering::greater;
        }
        if (other.is_infinity_) {
            return std::strong_ordering::less;
        }
        return value_ <=> other.value_;
    }

    /**
     * @brief Equality comparison operator.
     * @param other The rank to compare with.
     * @return true if the ranks are equal, false otherwise.
     */
    [[nodiscard]] constexpr bool operator==(const Rank& other) const noexcept {
        return is_infinity_ == other.is_infinity_ && (is_infinity_ || value_ == other.value_);
    }

    /**
     * @brief Increment operator (prefix).
     *
     * @return Reference to this rank after incrementing.
     * @throws std::overflow_error if incrementing would exceed max_finite_value().
     * @throws std::logic_error if this rank is infinite.
     */
    Rank& operator++() {
        if (is_infinity_) {
            throw std::logic_error("Cannot increment infinite rank");
        }
        if (value_ >= max_finite_value() - 1) {
            throw std::overflow_error("Rank increment would exceed max_finite_value()");
        }
        ++value_;
        return *this;
    }

    /**
     * @brief Increment operator (postfix).
     *
     * @return Copy of this rank before incrementing.
     * @throws std::overflow_error if incrementing would exceed max_finite_value().
     * @throws std::logic_error if this rank is infinite.
     */
    Rank operator++(int) {
        Rank old = *this;
        ++(*this);
        return old;
    }

    /**
     * @brief Decrement operator (prefix).
     *
     * @return Reference to this rank after decrementing.
     * @throws std::underflow_error if this rank is already zero.
     * @throws std::logic_error if this rank is infinite.
     */
    Rank& operator--() {
        if (is_infinity_) {
            throw std::logic_error("Cannot decrement infinite rank");
        }
        if (value_ == 0) {
            throw std::underflow_error("Cannot decrement rank below zero");
        }
        --value_;
        return *this;
    }

    /**
     * @brief Decrement operator (postfix).
     *
     * @return Copy of this rank before decrementing.
     * @throws std::underflow_error if this rank is already zero.
     * @throws std::logic_error if this rank is infinite.
     */
    Rank operator--(int) {
        Rank old = *this;
        --(*this);
        return old;
    }

    /**
     * @brief Output stream operator.
     *
     * Formats rank as its numeric value or "∞" for infinity.
     *
     * @param os The output stream.
     * @param rank The rank to output.
     * @return Reference to the output stream.
     */
    friend std::ostream& operator<<(std::ostream& os, const Rank& rank) {
        if (rank.is_infinity_) {
            os << "∞";
        } else {
            os << rank.value_;
        }
        return os;
    }

private:
    /**
     * @brief Private constructor for internal use.
     * @param value The numeric rank value.
     * @param is_infinity Whether this rank represents infinity.
     */
    constexpr Rank(uint64_t value, bool is_infinity) noexcept
        : value_(value), is_infinity_(is_infinity) {}

    uint64_t value_;       ///< Numeric rank value (meaningful only if not infinite)
    bool is_infinity_;     ///< Whether this rank represents infinity
};

}  // namespace ranked_belief

#endif  // RANKED_BELIEF_RANK_HPP
