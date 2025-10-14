#pragma once

#include "ranked_belief/operations/merge.hpp"
#include "ranked_belief/operations/merge_apply.hpp"
#include "ranked_belief/promise.hpp"
#include "ranked_belief/ranking_element.hpp"
#include "ranked_belief/ranking_function.hpp"

#include <cstddef>
#include <functional>
#include <mutex>
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
    Deduplication deduplicate = Deduplication::Disabled)
{
    auto normal_head = normal.head();
    if (!normal_head) {
        auto exceptional_rf = std::invoke(std::forward<ExceptionalThunk>(exceptional));
        if (exceptional_rank != Rank::zero()) {
            exceptional_rf = shift_ranks(exceptional_rf, exceptional_rank);
        }
        return exceptional_rf;
    }

    const bool normal_dedup = normal.is_deduplicating();
    auto dedup_from_bool = [](bool flag) {
        return flag ? Deduplication::Enabled : Deduplication::Disabled;
    };

    using ExceptionalThunkType = std::decay_t<ExceptionalThunk>;

    struct ExceptionalState {
        std::once_flag flag;
        RankingFunction<T> ranking;
    };

    auto state = std::make_shared<ExceptionalState>();
    auto thunk_storage = std::make_shared<std::optional<ExceptionalThunkType>>(std::in_place, std::forward<ExceptionalThunk>(exceptional));

    auto ensure_exceptional = [state, thunk_storage, exceptional_rank]() -> RankingFunction<T>& {
        std::call_once(state->flag, [&]() {
            RankingFunction<T> realised;
            if (auto& stored = *thunk_storage; stored) {
                auto rf = std::invoke(std::move(*stored));
                stored.reset();
                if (exceptional_rank != Rank::zero()) {
                    rf = shift_ranks(rf, exceptional_rank);
                }
                realised = std::move(rf);
            }
            state->ranking = std::move(realised);
        });
        return state->ranking;
    };

    const Rank normal_rank = normal_head->rank();
    bool use_normal_head = true;
    std::shared_ptr<RankingElement<T>> exceptional_head;

    if (exceptional_rank <= normal_rank) {
        auto& exceptional_rf = ensure_exceptional();
        exceptional_head = exceptional_rf.head();
        if (exceptional_head && exceptional_head->rank() < normal_rank) {
            use_normal_head = false;
        }
    }

    if (use_normal_head) {
        auto value_promise = make_promise([normal_head]() -> T {
            return normal_head->value();
        });

        auto next_promise = make_promise([normal_head, normal_dedup, deduplicate, ensure_exceptional, dedup_from_bool]() {
            auto next_head = normal_head->next();
            RankingFunction<T> normal_tail(
                std::move(next_head),
                dedup_from_bool(normal_dedup));

            auto& exceptional_rf = ensure_exceptional();
            auto merged = merge(normal_tail, exceptional_rf, deduplicate);
            return merged.head();
        });

        auto combined_head = std::make_shared<RankingElement<T>>(
            std::move(value_promise),
            normal_rank,
            std::move(next_promise));

        return RankingFunction<T>(std::move(combined_head), deduplicate);
    }

    // Exceptional head outranks the normal head.
    auto& exceptional_rf = ensure_exceptional();
    exceptional_head = exceptional_rf.head();

    if (!exceptional_head) {
        // Exceptional branch ended up empty; fall back to the normal branch.
        auto value_promise = make_promise([normal_head]() -> T {
            return normal_head->value();
        });

        auto next_promise = make_promise([normal_head, normal_dedup, deduplicate, ensure_exceptional, dedup_from_bool]() {
            auto next_head = normal_head->next();
            RankingFunction<T> normal_tail(
                std::move(next_head),
                dedup_from_bool(normal_dedup));

            auto& exceptional_rf_inner = ensure_exceptional();
            auto merged = merge(normal_tail, exceptional_rf_inner, deduplicate);
            return merged.head();
        });

        auto combined_head = std::make_shared<RankingElement<T>>(
            std::move(value_promise),
            normal_rank,
            std::move(next_promise));

        return RankingFunction<T>(std::move(combined_head), deduplicate);
    }

    auto exceptional_value_promise = make_promise([exceptional_head]() -> T {
        return exceptional_head->value();
    });

    auto next_promise = make_promise([normal, deduplicate, ensure_exceptional, exceptional_head, dedup_from_bool]() {
        auto& realised = ensure_exceptional();
        auto tail_head = exceptional_head->next();
        RankingFunction<T> exceptional_tail(
            std::move(tail_head),
            dedup_from_bool(realised.is_deduplicating()));

        auto merged = merge(normal, exceptional_tail, deduplicate);
        return merged.head();
    });

    auto combined_head = std::make_shared<RankingElement<T>>(
        std::move(exceptional_value_promise),
        exceptional_head->rank(),
        std::move(next_promise));

    return RankingFunction<T>(std::move(combined_head), deduplicate);
}

} // namespace ranked_belief
