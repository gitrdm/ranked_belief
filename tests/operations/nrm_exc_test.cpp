#include <gtest/gtest.h>

#include "ranked_belief/constructors.hpp"
#include "ranked_belief/operations/nrm_exc.hpp"

#include <string>
#include <vector>

using namespace ranked_belief;

namespace {

template<typename T>
std::vector<T> collect_values(const std::vector<std::pair<T, Rank>>& pairs) {
    std::vector<T> values;
    values.reserve(pairs.size());
    for (const auto& [value, rank] : pairs) {
        (void)rank;
        values.push_back(value);
    }
    return values;
}

template<typename T>
std::vector<Rank> collect_ranks(const std::vector<std::pair<T, Rank>>& pairs) {
    std::vector<Rank> ranks;
    ranks.reserve(pairs.size());
    for (const auto& [value, rank] : pairs) {
        (void)value;
        ranks.push_back(rank);
    }
    return ranks;
}

} // namespace

TEST(NormalExceptionalOperationsTest, MostNormalReturnsValue) {
    auto rf = from_list<std::string>({
        {"alpha", Rank::from_value(2)},
        {"beta", Rank::from_value(5)}
    });

    auto result = most_normal(rf);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, "alpha");
}

TEST(NormalExceptionalOperationsTest, MostNormalEmptyReturnsNullopt) {
    auto empty_rf = make_empty_ranking<int>();
    auto result = most_normal(empty_rf);
    EXPECT_FALSE(result.has_value());
}

TEST(NormalExceptionalOperationsTest, TakeNZeroReturnsEmptyVector) {
    auto rf = from_values_sequential<int>({1, 2, 3, 4});
    auto pairs = take_n(rf, 0);
    EXPECT_TRUE(pairs.empty());
}

TEST(NormalExceptionalOperationsTest, TakeNLessThanAvailable) {
    auto rf = from_values_sequential<int>({1, 2, 3, 4, 5});
    auto pairs = take_n(rf, 3);

    std::vector<int> expected_values = {1, 2, 3};
    EXPECT_EQ(collect_values(pairs), expected_values);
    EXPECT_EQ(pairs.size(), 3u);
}

TEST(NormalExceptionalOperationsTest, TakeNMoreThanAvailableReturnsAll) {
    auto rf = from_values_sequential<int>({1, 2, 3});
    auto pairs = take_n(rf, 10);

    std::vector<int> expected_values = {1, 2, 3};
    EXPECT_EQ(collect_values(pairs), expected_values);
    EXPECT_EQ(pairs.size(), 3u);
}

TEST(NormalExceptionalOperationsTest, TakeNPreservesRanks) {
    auto rf = from_list<int>({
        {10, Rank::from_value(0)},
        {20, Rank::from_value(3)},
        {30, Rank::from_value(7)},
        {40, Rank::from_value(8)}
    });

    auto pairs = take_n(rf, 3);

    std::vector<int> expected_values = {10, 20, 30};
    std::vector<Rank> expected_ranks = {
        Rank::from_value(0),
        Rank::from_value(3),
        Rank::from_value(7)
    };

    EXPECT_EQ(collect_values(pairs), expected_values);
    EXPECT_EQ(collect_ranks(pairs), expected_ranks);
}

TEST(NormalExceptionalOperationsTest, TakeNForcesOnlyRequiredPrefix) {
    std::size_t generator_calls = 0;
    auto rf = from_generator<int>([&generator_calls](std::size_t index) {
        ++generator_calls;
        return std::make_pair(static_cast<int>(index), Rank::from_value(index));
    });

    EXPECT_EQ(generator_calls, 1u);

    auto pairs = take_n(rf, 5);
    EXPECT_EQ(pairs.size(), 5u);

    EXPECT_LE(generator_calls, 6u);
    std::vector<int> expected_values = {0, 1, 2, 3, 4};
    EXPECT_EQ(collect_values(pairs), expected_values);
}
