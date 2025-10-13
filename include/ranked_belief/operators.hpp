#pragma once

/**
 * @file operators.hpp
 * @brief Point-wise arithmetic and comparison operators for ranking functions.
 *
 * This header provides a thin syntactic layer over the existing functional
 * primitives (autocast, map, merge-apply) so user code can write
 * `ranking + scalar` or `distribution1 < distribution2` in a natural style.
 *
 * Each operator is implemented lazily: the cartesian product of two rankings
 * is only explored on demand, and the ranks are combined by addition exactly
 * as merge-apply would. When mixing ranked and non-ranked operands, the
 * `autocast` helper lifts scalars into singleton rankings automatically.
 */

#include "ranked_belief/autocast.hpp"
#include "ranked_belief/operations/map.hpp"
#include "ranked_belief/operations/merge_apply.hpp"

#include <concepts>
#include <functional>
#include <type_traits>

namespace ranked_belief {

namespace detail {

template<typename T>
struct ranking_value_type;

template<typename T>
struct ranking_value_type<RankingFunction<T>> {
    using type = T;
};

template<typename T>
using ranking_value_type_t = typename ranking_value_type<std::remove_cvref_t<T>>::type;

template<typename L, typename R>
concept RankingOperand = IsRankingFunction<std::remove_cvref_t<L>> ||
                         IsRankingFunction<std::remove_cvref_t<R>>;

template<typename L, typename R, typename Op>
[[nodiscard]] auto combine_binary(L&& lhs, R&& rhs, Op op)
{
    auto lhs_rf = autocast(std::forward<L>(lhs));
    auto rhs_rf = autocast(std::forward<R>(rhs));

    using LValue = ranking_value_type_t<decltype(lhs_rf)>;
    using RValue = ranking_value_type_t<decltype(rhs_rf)>;
    using Result = std::invoke_result_t<Op, const LValue&, const RValue&>;

    auto rhs_copy = rhs_rf;
    const bool rhs_dedup = rhs_rf.is_deduplicating();
    const bool result_dedup = lhs_rf.is_deduplicating() && rhs_rf.is_deduplicating();

    auto apply_to_rhs = [rhs_copy, op, rhs_dedup](const LValue& lhs_value) {
        return map(
            rhs_copy,
            [lhs_value, op](const RValue& rhs_value) -> Result {
                return std::invoke(op, lhs_value, rhs_value);
            },
            rhs_dedup
        );
    };

    return merge_apply(lhs_rf, apply_to_rhs, result_dedup);
}

} // namespace detail

/**
 * @brief Lazily add two ranked operands point-wise.
 *
 * The operator forms the cartesian product of both rankings. For every pair of
 * values, it emits the sum while adding their ranks. Scalars are lifted into
 * singleton rankings via `autocast`, so expressions like `rf + 3` and
 * `3 + rf` are supported out of the box.
 */
template<typename L, typename R>
requires detail::RankingOperand<L, R>
[[nodiscard]] auto operator+(L&& lhs, R&& rhs)
{
    return detail::combine_binary(std::forward<L>(lhs), std::forward<R>(rhs), std::plus<>{});
}

/**
 * @brief Lazily subtract two ranked operands point-wise.
 */
template<typename L, typename R>
requires detail::RankingOperand<L, R>
[[nodiscard]] auto operator-(L&& lhs, R&& rhs)
{
    return detail::combine_binary(std::forward<L>(lhs), std::forward<R>(rhs), std::minus<>{});
}

/**
 * @brief Lazily multiply two ranked operands point-wise.
 */
template<typename L, typename R>
requires detail::RankingOperand<L, R>
[[nodiscard]] auto operator*(L&& lhs, R&& rhs)
{
    return detail::combine_binary(std::forward<L>(lhs), std::forward<R>(rhs), std::multiplies<>{});
}

/**
 * @brief Lazily divide two ranked operands point-wise.
 */
template<typename L, typename R>
requires detail::RankingOperand<L, R>
[[nodiscard]] auto operator/(L&& lhs, R&& rhs)
{
    return detail::combine_binary(std::forward<L>(lhs), std::forward<R>(rhs), std::divides<>{});
}

/**
 * @brief Lazily compare two ranked operands for equality point-wise.
 */
template<typename L, typename R>
requires detail::RankingOperand<L, R>
[[nodiscard]] auto operator==(L&& lhs, R&& rhs)
{
    return detail::combine_binary(std::forward<L>(lhs), std::forward<R>(rhs), std::equal_to<>{});
}

/**
 * @brief Lazily compare two ranked operands for inequality point-wise.
 */
template<typename L, typename R>
requires detail::RankingOperand<L, R>
[[nodiscard]] auto operator!=(L&& lhs, R&& rhs)
{
    return detail::combine_binary(std::forward<L>(lhs), std::forward<R>(rhs), std::not_equal_to<>{});
}

/**
 * @brief Lazily compare two ranked operands with `<` point-wise.
 */
template<typename L, typename R>
requires detail::RankingOperand<L, R>
[[nodiscard]] auto operator<(L&& lhs, R&& rhs)
{
    return detail::combine_binary(std::forward<L>(lhs), std::forward<R>(rhs), std::less<>{});
}

/**
 * @brief Lazily compare two ranked operands with `<=` point-wise.
 */
template<typename L, typename R>
requires detail::RankingOperand<L, R>
[[nodiscard]] auto operator<=(L&& lhs, R&& rhs)
{
    return detail::combine_binary(std::forward<L>(lhs), std::forward<R>(rhs), std::less_equal<>{});
}

/**
 * @brief Lazily compare two ranked operands with `>` point-wise.
 */
template<typename L, typename R>
requires detail::RankingOperand<L, R>
[[nodiscard]] auto operator>(L&& lhs, R&& rhs)
{
    return detail::combine_binary(std::forward<L>(lhs), std::forward<R>(rhs), std::greater<>{});
}

/**
 * @brief Lazily compare two ranked operands with `>=` point-wise.
 */
template<typename L, typename R>
requires detail::RankingOperand<L, R>
[[nodiscard]] auto operator>=(L&& lhs, R&& rhs)
{
    return detail::combine_binary(std::forward<L>(lhs), std::forward<R>(rhs), std::greater_equal<>{});
}

} // namespace ranked_belief
