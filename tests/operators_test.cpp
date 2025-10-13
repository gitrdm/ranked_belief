#include <gtest/gtest.h>

#include "ranked_belief/autocast.hpp"
#include "ranked_belief/constructors.hpp"
#include "ranked_belief/operators.hpp"

#include <algorithm>
#include <string>
#include <vector>

using namespace ranked_belief;

namespace {

template<typename T>
std::vector<std::pair<T, Rank>> collect_pairs(const RankingFunction<T>& rf, std::size_t limit = 20)
{
    std::vector<std::pair<T, Rank>> result;
    auto it = rf.begin();
    auto end = rf.end();
    for (std::size_t index = 0; index < limit && it != end; ++index, ++it) {
        result.push_back(*it);
    }
    return result;
}

} // namespace

TEST(OperatorsTest, AdditionCombinesRanksAndValues)
{
    auto lhs = from_list<int>({
        {1, Rank::from_value(0)},
        {2, Rank::from_value(2)}
    });

    auto rhs = from_list<int>({
        {10, Rank::from_value(1)},
        {20, Rank::from_value(3)}
    });

    auto combined = lhs + rhs;
    auto items = collect_pairs(combined);

    ASSERT_EQ(items.size(), 4u);
    EXPECT_EQ(items[0], std::make_pair(11, Rank::from_value(1))); // 1@0 + 10@1

    auto sorted_items = items;
    auto expected = std::vector{
        std::make_pair(11, Rank::from_value(1)),
        std::make_pair(12, Rank::from_value(3)),
        std::make_pair(21, Rank::from_value(3)),
        std::make_pair(22, Rank::from_value(5))
    };
    auto compare_by_rank_then_value = [](const auto& lhs, const auto& rhs) {
        if (lhs.second == rhs.second) {
            return lhs.first < rhs.first;
        }
        return lhs.second < rhs.second;
    };

    std::sort(sorted_items.begin(), sorted_items.end(), compare_by_rank_then_value);
    std::sort(expected.begin(), expected.end(), compare_by_rank_then_value);
    EXPECT_EQ(sorted_items, expected);
}

TEST(OperatorsTest, MixedScalarAdditionUsesAutocast)
{
    auto rf = from_values_sequential<int>({1, 2, 3});

    auto shifted = rf + 5;
    auto items = collect_pairs(shifted);

    ASSERT_EQ(items.size(), 3u);
    EXPECT_EQ(items[0], std::make_pair(6, Rank::from_value(0)));
    EXPECT_EQ(items[1], std::make_pair(7, Rank::from_value(1)));
    EXPECT_EQ(items[2], std::make_pair(8, Rank::from_value(2)));

    auto reverse = 5 + rf;
    auto reverse_items = collect_pairs(reverse);
    EXPECT_EQ(reverse_items, items);
}

TEST(OperatorsTest, MultiplicationProducesExpectedRanks)
{
    auto numbers = from_list<int>({
        {2, Rank::from_value(0)},
        {4, Rank::from_value(3)}
    });

    auto factors = from_list<int>({
        {3, Rank::from_value(1)}
    });

    auto products = numbers * factors;
    auto items = collect_pairs(products);

    ASSERT_EQ(items.size(), 2u);
    EXPECT_EQ(items[0], std::make_pair(6, Rank::from_value(1)));
    EXPECT_EQ(items[1], std::make_pair(12, Rank::from_value(4)));
}

TEST(OperatorsTest, ComparisonProducesBooleanRanking)
{
    auto lhs = from_values_uniform<int>({1, 2});
    auto rhs = from_values_uniform<int>({2, 2});

    auto equal_ranking = lhs == rhs;
    auto less_ranking = lhs < rhs;

    auto equal_items = collect_pairs(equal_ranking);
    auto less_items = collect_pairs(less_ranking);

    ASSERT_EQ(equal_items.size(), 2u);
    ASSERT_EQ(less_items.size(), 2u);

    EXPECT_TRUE((equal_items[0].first == false && equal_items[1].first == true) ||
                (equal_items[0].first == true && equal_items[1].first == false));
    EXPECT_EQ(equal_items[0].second, Rank::from_value(0));
    EXPECT_EQ(equal_items[1].second, Rank::from_value(0));

    EXPECT_TRUE((less_items[0].first == true && less_items[1].first == false) ||
                (less_items[0].first == false && less_items[1].first == true));
    EXPECT_EQ(less_items[0].second, Rank::from_value(0));
    EXPECT_EQ(less_items[1].second, Rank::from_value(0));
}

TEST(OperatorsTest, OperatorIsLazy)
{
    std::size_t lhs_calls = 0;
    std::size_t rhs_calls = 0;

    auto lhs = from_generator<int>([&lhs_calls](std::size_t index) {
        ++lhs_calls;
        return std::make_pair(static_cast<int>(index), Rank::from_value(index));
    });

    auto rhs = from_generator<int>([&rhs_calls](std::size_t index) {
        ++rhs_calls;
        return std::make_pair(static_cast<int>(index * 2), Rank::from_value(index));
    });

    EXPECT_LE(lhs_calls, 2u);
    EXPECT_LE(rhs_calls, 2u);

    auto lhs_before = lhs_calls;
    auto rhs_before = rhs_calls;

    auto sum = lhs + rhs;

    EXPECT_LE(lhs_calls, lhs_before + 1);
    EXPECT_LE(rhs_calls, rhs_before + 1);

    auto first = sum.first();
    ASSERT_TRUE(first.has_value());
    EXPECT_EQ(first->first, 0);
    EXPECT_EQ(first->second, Rank::from_value(0));

    EXPECT_LE(lhs_calls, 2u);
    EXPECT_LE(rhs_calls, 2u);
}
