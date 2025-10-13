#pragma once

#include "ranked_belief/ranking_function.hpp"

#include <cstddef>
#include <optional>
#include <utility>
#include <vector>

namespace ranked_belief {

/**
 * @brief Retrieve the most normal value from a ranking function.
 *
 * This helper peeks at the head of the ranking function and returns its value
 * if the sequence is non-empty. The underlying ranking function remains lazy;
 * only the first element is inspected.
 *
 * @tparam T Value type stored in the ranking function.
 * @param rf Ranking function to inspect.
 * @return std::nullopt when the ranking is empty, otherwise the most normal
 *         value (i.e., the one with minimum rank).
 */
template<typename T>
[[nodiscard]] std::optional<T> most_normal(const RankingFunction<T>& rf)
{
    auto first = rf.first();
    if (!first) {
        return std::nullopt;
    }
    return std::make_optional(first->first);
}

/**
 * @brief Materialise the first @p count entries of a ranking function.
 *
 * The function walks the ranking lazily and copies at most @p count
 * value/rank pairs into a std::vector. Only the inspected prefix is forced;
 * the remainder of the ranking stays unevaluated.
 *
 * @tparam T Value type stored in the ranking function.
 * @param rf Source ranking function.
 * @param count Maximum number of entries to retrieve.
 * @return Vector containing up to @p count pairs in increasing rank order.
 */
template<typename T>
[[nodiscard]] std::vector<std::pair<T, Rank>> take_n(
    const RankingFunction<T>& rf,
    std::size_t count)
{
    std::vector<std::pair<T, Rank>> result;
    if (count == 0 || rf.is_empty()) {
        return result;
    }

    result.reserve(count);

    auto it = rf.begin();
    auto end = rf.end();

    std::size_t consumed = 0;
    while (it != end && consumed < count) {
        result.push_back(*it);
        ++it;
        ++consumed;
    }

    return result;
}

} // namespace ranked_belief
