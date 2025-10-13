#pragma once

#include "ranked_belief/constructors.hpp"
#include "ranked_belief/ranking_function.hpp"

#include <type_traits>
#include <utility>

namespace ranked_belief {

namespace detail {

/// Trait detecting whether a type is an instantiation of RankingFunction.
template<typename T>
struct is_ranking_function : std::false_type {};

template<typename T>
struct is_ranking_function<RankingFunction<T>> : std::true_type {};

} // namespace detail

/**
 * @brief Concept that matches any (possibly cv/ref qualified) RankingFunction type.
 *
 * This concept recognises RankingFunction instantiations regardless of
 * reference or cv-qualifiers so that helper templates can branch on ranked
 * values versus raw scalars.
 */
template<typename T>
concept IsRankingFunction = detail::is_ranking_function<std::remove_cvref_t<T>>::value;

/**
 * @brief Autocast helper that wraps plain values in a singleton ranking.
 *
 * For non-ranking inputs the helper creates a RankingFunction containing the
 * supplied value at rank zero. Construction preserves laziness guarantees of
 * the underlying constructors and enables seamless operator overloading.
 *
 * @tparam T Plain value type (non-ranking).
 * @param value The value to lift into ranking space.
 * @return Ranking function containing @p value at rank zero.
 */
template<typename T>
requires (!IsRankingFunction<T>)
[[nodiscard]] auto autocast(T&& value)
{
    return singleton(std::forward<T>(value));
}

/**
 * @brief Pass-through overload that leaves ranking functions unchanged.
 *
 * When autocast receives an existing RankingFunction it simply forwards the
 * argument, preserving value category (lvalue references stay references,
 * rvalues remain movable temporaries) and deduplication semantics.
 *
 * @tparam RF Any ranking function instantiation.
 * @param rf Ranking function to forward.
 * @return Forwarded ranking function.
 */
template<typename RF>
requires IsRankingFunction<RF>
[[nodiscard]] constexpr RF&& autocast(RF&& rf) noexcept
{
    return std::forward<RF>(rf);
}

} // namespace ranked_belief
