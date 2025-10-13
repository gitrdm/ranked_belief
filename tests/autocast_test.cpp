#include <gtest/gtest.h>

#include "ranked_belief/autocast.hpp"
#include "ranked_belief/constructors.hpp"

#include <string>
#include <type_traits>

using namespace ranked_belief;

namespace {

static_assert(IsRankingFunction<RankingFunction<int>>);
static_assert(IsRankingFunction<const RankingFunction<double>&>);
static_assert(!IsRankingFunction<int>);
static_assert(!IsRankingFunction<std::string>);

} // namespace

TEST(AutocastTest, ScalarProducesSingletonRanking) {
    auto rf = autocast(42);

    ASSERT_FALSE(rf.is_empty());
    auto first = rf.first();
    ASSERT_TRUE(first.has_value());
    EXPECT_EQ(first->first, 42);
    EXPECT_EQ(first->second, Rank::zero());
}

TEST(AutocastTest, StringValueIsLifted) {
    std::string word = "hello";
    auto rf = autocast(word);

    auto first = rf.first();
    ASSERT_TRUE(first.has_value());
    EXPECT_EQ(first->first, "hello");
    EXPECT_EQ(first->second, Rank::zero());
}

TEST(AutocastTest, RankingFunctionLvalueIsForwarded) {
    auto rf = from_values_sequential<int>({1, 2, 3});

    auto& forwarded = autocast(rf);
    EXPECT_EQ(forwarded.head(), rf.head());
    EXPECT_TRUE(std::is_reference_v<decltype(forwarded)>);
}

TEST(AutocastTest, RankingFunctionRvalueIsForwarded) {
    auto rf = from_values_sequential<int>({4, 5});

    auto moved = autocast(std::move(rf));
    auto first = moved.first();
    ASSERT_TRUE(first.has_value());
    EXPECT_EQ(first->first, 4);
}

TEST(AutocastTest, ForwardingDoesNotForceAdditionalEvaluation) {
    std::size_t generator_calls = 0;
    auto base = from_generator<int>([&generator_calls](std::size_t index) {
        ++generator_calls;
        return std::make_pair(static_cast<int>(index), Rank::from_value(index));
    });

    EXPECT_EQ(generator_calls, 1u);

    auto& forwarded = autocast(base);
    EXPECT_EQ(generator_calls, 1u);

    auto first = forwarded.first();
    ASSERT_TRUE(first.has_value());
    EXPECT_EQ(first->first, 0);
    EXPECT_LE(generator_calls, 2u);
}
