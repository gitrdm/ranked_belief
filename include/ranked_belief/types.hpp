/**
 * @file types.hpp
 * @brief Strong type definitions for ranked belief APIs.
 *
 * This file defines strong types to replace primitive bool flags and other
 * parameters, providing better compile-time safety and self-documenting APIs.
 *
 * Benefits:
 * - Compile-time prevention of incorrect parameter usage
 * - Self-documenting function signatures
 * - No runtime overhead (enums are zero-cost abstractions)
 * - Backward compatibility via implicit conversions where needed
 */

#ifndef RANKED_BELIEF_TYPES_HPP
#define RANKED_BELIEF_TYPES_HPP

#include <type_traits>

namespace ranked_belief {

/**
 * @enum Deduplication
 * @brief Controls whether consecutive duplicate values are removed.
 *
 * Used in ranking function constructors, iterators, and operations to
 * explicitly control deduplication behavior.
 *
 * Example:
 * @code
 * auto rf = from_list(pairs, Deduplication::Enabled);
 * auto it = rf.begin(Deduplication::Disabled);
 * @endcode
 */
enum class Deduplication : bool {
    Enabled = true,   ///< Remove consecutive duplicate values
    Disabled = false  ///< Keep all values including duplicates
};

/**
 * @brief Convert Deduplication to bool for backward compatibility.
 *
 * Allows implicit conversion to bool in contexts where bool is expected.
 */
[[nodiscard]] constexpr bool to_bool(Deduplication dedup) noexcept {
    return static_cast<bool>(dedup);
}

/**
 * @brief Helper function to enable deduplication.
 *
 * Provides a named constant for better readability.
 */
[[nodiscard]] constexpr Deduplication enable_deduplication() noexcept {
    return Deduplication::Enabled;
}

/**
 * @brief Helper function to disable deduplication.
 *
 * Provides a named constant for better readability.
 */
[[nodiscard]] constexpr Deduplication disable_deduplication() noexcept {
    return Deduplication::Disabled;
}

/**
 * @brief Convert boolean to Deduplication enum.
 *
 * Utility for backward compatibility and internal use.
 */
[[nodiscard]] constexpr Deduplication from_bool(bool enable) noexcept {
    return enable ? Deduplication::Enabled : Deduplication::Disabled;
}

/**
 * @enum EvaluationStrategy
 * @brief Controls when computations are evaluated.
 *
 * Reserved for future use. Currently all evaluation is lazy by design.
 *
 * Example (future):
 * @code
 * auto rf = from_generator(gen, EvaluationStrategy::Lazy);
 * @endcode
 */
enum class EvaluationStrategy : bool {
    Lazy = true,   ///< Defer computation until needed (default)
    Eager = false  ///< Evaluate immediately (reserved for future use)
};

}  // namespace ranked_belief

#endif  // RANKED_BELIEF_TYPES_HPP
