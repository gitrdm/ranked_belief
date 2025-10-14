#pragma once

#include "ranked_belief/ranking_element.hpp"
#include "ranked_belief/ranking_function.hpp"
#include "ranked_belief/operations/filter.hpp"

#include <concepts>
#include <functional>
#include <memory>
#include <utility>

namespace ranked_belief {

namespace detail {

/**
 * @brief Lazily subtract a constant rank from every element in a sequence.
 *
 * Builds a new ranking sequence that mirrors the input but with each element's
 * rank decreased by `shift_amount`. The computation is fully lazy: the tail is
 * only normalized when forced.
 */
template<typename T>
[[nodiscard]] std::shared_ptr<RankingElement<T>> normalize_with_shift(
    const std::shared_ptr<RankingElement<T>>& elem,
    Rank shift_amount)
{
    using BuildFunc = std::function<std::shared_ptr<RankingElement<T>>(
        std::shared_ptr<RankingElement<T>>) >;
    auto build_normalized = std::make_shared<BuildFunc>();
    std::weak_ptr<BuildFunc> weak_build = build_normalized;

    *build_normalized = [weak_build, shift_amount](
        std::shared_ptr<RankingElement<T>> current) -> std::shared_ptr<RankingElement<T>> {
        if (!current) {
            return nullptr;
        }

        if (current->rank().is_infinity()) {
            return nullptr;
        }

        auto build_ref = weak_build.lock();

        auto compute_next = [weak_build, build_ref, current]() -> std::shared_ptr<RankingElement<T>> {
            auto next_elem = current->next();

            if (auto locked = weak_build.lock()) {
                return (*locked)(next_elem);
            }

            if (build_ref) {
                return (*build_ref)(next_elem);
            }
            return nullptr;
        };

        Rank adjusted_rank = current->rank() - shift_amount;

        return std::make_shared<RankingElement<T>>(
            current->value(),
            adjusted_rank,
            make_promise(std::move(compute_next))
        );
    };

    return (*build_normalized)(elem);
}

} // namespace detail

/**
 * @brief Condition a ranking function on a predicate (observe evidence).
 *
 * Matches the Racket `observe` semantics: the ranking is filtered by the
 * predicate and then normalized so that the lowest-rank satisfying element
 * becomes rank 0. All remaining elements retain their relative differences by
 * subtracting the same offset.
 *
 * @tparam T Value type of the ranking function.
 * @tparam Pred Predicate type; must return bool when invoked with `const T&`.
 * @param rf Source ranking function.
 * @param predicate Condition describing the observed evidence.
 * @param deduplicate Whether the resulting ranking should deduplicate values.
 * @return Ranking function conditioned on the observation.
 */
template<typename T, typename Pred>
requires std::invocable<Pred, const T&> &&
         std::same_as<std::invoke_result_t<Pred, const T&>, bool>
[[nodiscard]] RankingFunction<T> observe(
    const RankingFunction<T>& rf,
    Pred predicate,
    Deduplication deduplicate = Deduplication::Enabled)
{
    auto filtered = filter(rf, std::move(predicate), deduplicate);
    auto head = filtered.head();

    if (!head) {
        return filtered;
    }

    const Rank shift_amount = head->rank();
    if (shift_amount.is_infinity()) {
        return RankingFunction<T>(nullptr, from_bool(filtered.is_deduplicating()));
    }
    if (shift_amount == Rank::zero()) {
        return filtered;
    }

    auto normalized_head = detail::normalize_with_shift(head, shift_amount);
    return RankingFunction<T>(normalized_head, from_bool(filtered.is_deduplicating()));
}

/**
 * @brief Convenience overload that conditions on equality with a specific value.
 */
template<typename T>
[[nodiscard]] RankingFunction<T> observe(
    const RankingFunction<T>& rf,
    const T& observed_value,
    Deduplication deduplicate = Deduplication::Enabled)
{
    return observe(
        rf,
        [observed_value](const T& value) { return value == observed_value; },
        deduplicate
    );
}

} // namespace ranked_belief
