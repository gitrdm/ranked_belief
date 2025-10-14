#pragma once

#include "ranked_belief/operations/merge.hpp"
#include "ranked_belief/operations/merge_apply.hpp"
#include "ranked_belief/promise.hpp"
#include "ranked_belief/ranking_element.hpp"
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

template<typename T, typename ExceptionalThunk>
requires std::invocable<ExceptionalThunk>
[[nodiscard]] RankingFunction<T> normal_exceptional(
    const RankingFunction<T>& normal,
    ExceptionalThunk exceptional,
    Rank exceptional_rank = Rank::from_value(1),
    bool deduplicate = false)
{
    // Robust implementation: merge the normal ranking with the exceptional
    // ranking (shifted by exceptional_rank). The previous optimization that
    // unconditionally took `normal.head()` as the overall head could produce
    // a sequence where the first element has a larger rank than subsequent
    // exceptional elements (violating the non-decreasing rank invariant).
    // Merging here preserves ordering and is fully lazy via `merge`.
    auto exceptional_rf = exceptional();
    if (exceptional_rank != Rank::zero()) {
        exceptional_rf = shift_ranks(exceptional_rf, exceptional_rank);
    }

    // Merge normal and exceptional (shifted) ranking functions. `merge`
    // preserves rank ordering and will select the true minimum-ranked element
    // as the head, avoiding underflow during normalization later on.
    return merge(normal, exceptional_rf, deduplicate);
}

} // namespace ranked_belief
