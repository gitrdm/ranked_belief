#include <gtest/gtest.h>

#include "ranked_belief/constructors.hpp"
#include "ranked_belief/operations/observe.hpp"

#include <string>
#include <vector>

using namespace ranked_belief;

namespace {

template<typename T>
std::vector<std::pair<T, Rank>> collect_pairs(const RankingFunction<T>& rf, std::size_t limit = 100) {
    std::vector<std::pair<T, Rank>> result;
    auto it = rf.begin();
    auto end = rf.end();
    for (std::size_t i = 0; i < limit && it != end; ++i, ++it) {
        result.push_back(*it);
    }
    return result;
}

} // namespace

// =============================================================================
// Basic behaviour
// =============================================================================

TEST(ObserveTest, ObserveEmptyRankingYieldsEmpty) {
    auto empty = make_empty_ranking<int>();
    auto observed = observe(empty, 42);

    EXPECT_TRUE(observed.is_empty());
    EXPECT_EQ(observed.size(), 0);
}

TEST(ObserveTest, ObserveExactMatchNormalizesToZero) {
    auto rf = singleton(std::string{"cat"}, Rank::from_value(5));
    auto observed = observe(rf, std::string{"cat"});

    auto items = collect_pairs(observed);
    ASSERT_EQ(items.size(), 1);
    EXPECT_EQ(items[0].first, "cat");
    EXPECT_EQ(items[0].second, Rank::zero());
}

TEST(ObserveTest, ObserveNoMatchRemainsEmpty) {
    auto rf = from_list<int>({
        {1, Rank::from_value(1)},
        {2, Rank::from_value(3)}
    });

    auto observed = observe(rf, [](int value) { return value > 10; });

    EXPECT_TRUE(observed.is_empty());
    EXPECT_EQ(observed.size(), 0);
}

// =============================================================================
// Rank normalization
// =============================================================================

TEST(ObserveTest, ObserveShiftsRanksRelativeToFirstMatch) {
    auto rf = from_list<int>({
        {1, Rank::from_value(2)},
        {2, Rank::from_value(5)},
        {3, Rank::from_value(9)}
    });

    auto observed = observe(rf, [](int value) { return value >= 2; });
    auto items = collect_pairs(observed);

    ASSERT_EQ(items.size(), 2);
    EXPECT_EQ(items[0].first, 2);
    EXPECT_EQ(items[0].second, Rank::zero());
    EXPECT_EQ(items[1].first, 3);
    EXPECT_EQ(items[1].second, Rank::from_value(4));
}

TEST(ObserveTest, ObserveLeavesRanksWhenAlreadyZero) {
    auto rf = from_list<int>({
        {1, Rank::zero()},
        {2, Rank::from_value(3)}
    });

    auto observed = observe(rf, 1);

    auto first = observed.first();
    ASSERT_TRUE(first.has_value());
    EXPECT_EQ(first->first, 1);
    EXPECT_EQ(first->second, Rank::zero());
    EXPECT_TRUE(observed.is_deduplicating());
}

TEST(ObserveTest, ObserveHandlesInfiniteRank) {
    auto rf = from_list<int>({
        {1, Rank::infinity()},
        {2, Rank::infinity()}
    });

    auto observed = observe(rf, 1);

    EXPECT_TRUE(observed.is_empty());
    EXPECT_EQ(observed.size(), 0);
}

// =============================================================================
// Deduplication semantics
// =============================================================================

TEST(ObserveTest, ObserveRespectsDeduplicationEnabled) {
    auto rf = from_list<int>({
        {2, Rank::from_value(1)},
        {2, Rank::from_value(3)},
        {3, Rank::from_value(5)}
    }, false);

    auto observed = observe(rf, 2, true);
    auto items = collect_pairs(observed);

    ASSERT_EQ(items.size(), 1);
    EXPECT_EQ(items[0].first, 2);
    EXPECT_EQ(items[0].second, Rank::zero());
    EXPECT_TRUE(observed.is_deduplicating());
}

TEST(ObserveTest, ObservePreservesDuplicatesWhenDisabled) {
    auto rf = from_list<int>({
        {2, Rank::from_value(1)},
        {2, Rank::from_value(4)},
        {3, Rank::from_value(6)}
    }, false);

    auto observed = observe(rf, 2, false);
    auto items = collect_pairs(observed);

    ASSERT_EQ(items.size(), 2);
    EXPECT_EQ(items[0].first, 2);
    EXPECT_EQ(items[0].second, Rank::zero());
    EXPECT_EQ(items[1].first, 2);
    EXPECT_EQ(items[1].second, Rank::from_value(3));
    EXPECT_FALSE(observed.is_deduplicating());
}

// =============================================================================
// Laziness
// =============================================================================

TEST(ObserveTest, ObserveIsLazyUntilForced) {
    std::size_t generator_calls = 0;
    std::size_t predicate_calls = 0;

    auto rf = from_generator<int>(
        [&generator_calls](std::size_t index) {
            ++generator_calls;
            return std::make_pair(static_cast<int>(index), Rank::from_value(index));
        }
    );

    const auto generator_calls_before_observe = generator_calls;
    const auto predicate_calls_before_observe = predicate_calls;

    auto observed = observe(
        rf,
        [&predicate_calls](int value) {
            ++predicate_calls;
            return value % 7 == 0;
        }
    );

    EXPECT_LE(generator_calls - generator_calls_before_observe, 1u);
    EXPECT_LE(predicate_calls - predicate_calls_before_observe, 1u);

    const auto generator_calls_before_force = generator_calls;
    const auto predicate_calls_before_force = predicate_calls;

    auto first = observed.first();
    ASSERT_TRUE(first.has_value());
    EXPECT_EQ(first->first, 0);
    EXPECT_EQ(first->second, Rank::zero());

    EXPECT_EQ(generator_calls, generator_calls_before_force);
    EXPECT_EQ(predicate_calls, predicate_calls_before_force);
}

TEST(ObserveTest, ObserveDoesNotComputeBeyondNeed) {
    std::size_t generator_calls = 0;

    auto rf = from_generator<int>(
        [&generator_calls](std::size_t index) {
            ++generator_calls;
            return std::make_pair(static_cast<int>(index * 2), Rank::from_value(index));
        }
    );

    auto observed = observe(rf, [](int value) { return value > 10; });

    auto it = observed.begin();
    auto end = observed.end();
    int count = 0;
    while (it != end && count < 3) {
        ++count;
        ++it;
    }

    EXPECT_LT(generator_calls, 20u);
}

// =============================================================================
// Sequential observations
// =============================================================================

TEST(ObserveTest, SequentialObservationsReNormalise) {
    auto rf = from_list<int>({
        {1, Rank::from_value(1)},
        {2, Rank::from_value(2)},
        {3, Rank::from_value(4)},
        {4, Rank::from_value(8)}
    });

    auto greater_than_two = observe(rf, [](int value) { return value > 2; });
    auto even_values = observe(greater_than_two, [](int value) { return value % 2 == 0; });

    auto items = collect_pairs(even_values);

    ASSERT_EQ(items.size(), 1);
    EXPECT_EQ(items[0].first, 4);
    EXPECT_EQ(items[0].second, Rank::zero());
}
