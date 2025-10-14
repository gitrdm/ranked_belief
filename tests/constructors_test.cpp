/**
 * @file constructors_test.cpp
 * @brief Comprehensive tests for ranking function construction functions.
 *
 * Tests cover:
 * - from_list: Construct from value-rank pairs
 * - from_values_uniform: All values at same rank
 * - from_values_sequential: Sequential rank assignment
 * - from_values_with_ranker: Custom rank function
 * - from_generator: Infinite sequences
 * - from_range: C++20 range conversion
 * - from_pair_range: Range of pairs
 * - singleton, empty: Convenience aliases
 */

#include "ranked_belief/constructors.hpp"
#include <gtest/gtest.h>
#include <map>
#include <numeric>
#include <ranges>
#include <string>
#include <vector>

using namespace ranked_belief;

/// Test fixture for constructor tests
class ConstructorsTest : public ::testing::Test {};

// ============================================================================
// from_list Tests
// ============================================================================

TEST_F(ConstructorsTest, FromListWithEmptyVector) {
    auto rf = from_list<int>({});
    
    EXPECT_TRUE(rf.is_empty());
    EXPECT_EQ(rf.size(), 0);
}

TEST_F(ConstructorsTest, FromListWithSinglePair) {
    auto rf = from_list<int>({{42, Rank::from_value(5)}});
    
    EXPECT_FALSE(rf.is_empty());
    EXPECT_EQ(rf.size(), 1);
    
    auto first = rf.first();
    ASSERT_TRUE(first.has_value());
    EXPECT_EQ(first->first, 42);
    EXPECT_EQ(first->second, Rank::from_value(5));
}

TEST_F(ConstructorsTest, FromListWithMultiplePairs) {
    auto rf = from_list<int>({
        {1, Rank::zero()},
        {2, Rank::from_value(1)},
        {3, Rank::from_value(2)}
    });
    
    EXPECT_EQ(rf.size(), 3);
    
    std::vector<int> values;
    for (auto [value, rank] : rf) {
        values.push_back(value);
    }
    
    EXPECT_EQ(values, (std::vector<int>{1, 2, 3}));
}

TEST_F(ConstructorsTest, FromListPreservesRanks) {
    auto rf = from_list<int>({
        {10, Rank::zero()},
        {20, Rank::from_value(3)},
        {30, Rank::from_value(7)}
    });
    
    std::vector<uint64_t> ranks;
    for (auto [value, rank] : rf) {
        ranks.push_back(rank.value());
    }
    
    EXPECT_EQ(ranks, (std::vector<uint64_t>{0, 3, 7}));
}

TEST_F(ConstructorsTest, FromListWithStrings) {
    auto rf = from_list<std::string>({
        {"alpha", Rank::zero()},
        {"beta", Rank::from_value(1)}
    });
    
    std::vector<std::string> values;
    for (auto [value, rank] : rf) {
        values.push_back(value);
    }
    
    EXPECT_EQ(values, (std::vector<std::string>{"alpha", "beta"}));
}

TEST_F(ConstructorsTest, FromListWithDeduplication) {
    auto rf = from_list<int>({
        {1, Rank::zero()},
        {1, Rank::from_value(1)},
        {2, Rank::from_value(2)}
    }, Deduplication::Enabled);
    
    EXPECT_EQ(rf.size(), 2);  // 1 and 2 after dedup
}

TEST_F(ConstructorsTest, FromListWithoutDeduplication) {
    auto rf = from_list<int>({
        {1, Rank::zero()},
        {1, Rank::from_value(1)},
        {2, Rank::from_value(2)}
    }, Deduplication::Disabled);
    
    EXPECT_EQ(rf.size(), 3);  // All kept
}

// ============================================================================
// from_values_uniform Tests
// ============================================================================

TEST_F(ConstructorsTest, FromValuesUniformEmpty) {
    auto rf = from_values_uniform<int>({});
    
    EXPECT_TRUE(rf.is_empty());
}

// Test uniform rank with default rank (zero)
TEST_F(ConstructorsTest, FromValuesUniformDefaultRank) {
    std::vector<int> values{1, 2, 3};
    auto rf = from_values_uniform(values);
    
    for (auto [value, rank] : rf) {
        EXPECT_EQ(rank, Rank::zero());
    }
}

TEST_F(ConstructorsTest, FromValuesUniformCustomRank) {
    std::vector<int> values{10, 20, 30};
    auto rf = from_values_uniform(values, Rank::from_value(5));
    
    for (auto [value, rank] : rf) {
        EXPECT_EQ(rank, Rank::from_value(5));
    }
}

TEST_F(ConstructorsTest, FromValuesUniformPreservesOrder) {
    std::vector<int> input{1, 2, 3, 4, 5};
    auto rf = from_values_uniform(input);
    
    std::vector<int> values;
    for (auto [value, rank] : rf) {
        values.push_back(value);
    }
    
    EXPECT_EQ(values, (std::vector<int>{1, 2, 3, 4, 5}));
}

TEST_F(ConstructorsTest, FromValuesUniformWithStrings) {
    auto rf = from_values_uniform<std::string>({"a", "b", "c"});
    
    std::vector<std::string> values;
    for (auto [value, rank] : rf) {
        values.push_back(value);
    }
    
    EXPECT_EQ(values, (std::vector<std::string>{"a", "b", "c"}));
}

// ============================================================================
// from_values_sequential Tests
// ============================================================================

TEST_F(ConstructorsTest, FromValuesSequentialEmpty) {
    auto rf = from_values_sequential<int>({});
    
    EXPECT_TRUE(rf.is_empty());
}

// Test with default start rank
TEST_F(ConstructorsTest, FromValuesSequentialDefaultStart) {
    std::vector<int> values{1, 2, 3};
    auto rf = from_values_sequential(values);
    
    std::vector<std::pair<int, uint64_t>> pairs;
    for (auto [value, rank] : rf) {
        pairs.emplace_back(value, rank.value());
    }
    
    std::vector<std::pair<int, uint64_t>> expected = {
        {1, 0}, {2, 1}, {3, 2}
    };
    EXPECT_EQ(pairs, expected);
}

TEST_F(ConstructorsTest, FromValuesSequentialCustomStart) {
    std::vector<int> values{10, 20, 30};
    auto rf = from_values_sequential(values, Rank::from_value(5));
    
    std::vector<uint64_t> ranks;
    for (auto [value, rank] : rf) {
        ranks.push_back(rank.value());
    }
    
    EXPECT_EQ(ranks, (std::vector<uint64_t>{5, 6, 7}));
}

TEST_F(ConstructorsTest, FromValuesSequentialSingleElement) {
    std::vector<int> values{42};
    auto rf = from_values_sequential(values);
    
    EXPECT_EQ(rf.size(), 1);
    auto first = rf.first();
    ASSERT_TRUE(first.has_value());
    EXPECT_EQ(first->first, 42);
    EXPECT_EQ(first->second, Rank::zero());
}

// ============================================================================
// from_values_with_ranker Tests
// ============================================================================

TEST_F(ConstructorsTest, FromValuesWithRankerEmpty) {
    auto rf = from_values_with_ranker<int>({}, 
        [](const int&, size_t) { return Rank::zero(); });
    
    EXPECT_TRUE(rf.is_empty());
}

TEST_F(ConstructorsTest, FromValuesWithRankerValueBased) {
    std::vector<int> values{1, 2, 3, 4, 5};
    auto rf = from_values_with_ranker(values,
        [](const int& v, size_t) { return Rank::from_value(v * v); });
    
    std::vector<std::pair<int, uint64_t>> pairs;
    for (auto [value, rank] : rf) {
        pairs.emplace_back(value, rank.value());
    }
    
    std::vector<std::pair<int, uint64_t>> expected = {
        {1, 1}, {2, 4}, {3, 9}, {4, 16}, {5, 25}
    };
    EXPECT_EQ(pairs, expected);
}

// Test with index-based ranker
TEST_F(ConstructorsTest, FromValuesWithRankerIndexBased) {
    std::vector<int> values{10, 20, 30};
    auto rf = from_values_with_ranker(values,
        [](const int&, size_t idx) { return Rank::from_value(idx * 10); });
    
    std::vector<uint64_t> ranks;
    for (auto [value, rank] : rf) {
        ranks.push_back(rank.value());
    }
    
    EXPECT_EQ(ranks, (std::vector<uint64_t>{0, 10, 20}));
}

// Test with combined value and index ranker
TEST_F(ConstructorsTest, FromValuesWithRankerCombined) {
    std::vector<int> values{1, 2, 3};
    auto rf = from_values_with_ranker(values,
        [](const int& v, size_t idx) {
            return Rank::from_value(v + idx);
        });
    
    std::vector<uint64_t> ranks;
    for (auto [value, rank] : rf) {
        ranks.push_back(rank.value());
    }
    
    EXPECT_EQ(ranks, (std::vector<uint64_t>{1, 3, 5}));  // 1+0, 2+1, 3+2
}

// ============================================================================
// from_generator Tests
// ============================================================================

TEST_F(ConstructorsTest, FromGeneratorBasic) {
    auto rf = from_generator<int>([](size_t i) {
        return std::make_pair(static_cast<int>(i), Rank::from_value(i));
    });
    
    EXPECT_FALSE(rf.is_empty());
    
    auto first = rf.first();
    ASSERT_TRUE(first.has_value());
    EXPECT_EQ(first->first, 0);
    EXPECT_EQ(first->second, Rank::zero());
}

TEST_F(ConstructorsTest, FromGeneratorCustomStartIndex) {
    auto rf = from_generator<int>([](size_t i) {
        return std::make_pair(static_cast<int>(i), Rank::from_value(i));
    }, 10);
    
    auto first = rf.first();
    ASSERT_TRUE(first.has_value());
    EXPECT_EQ(first->first, 10);
    EXPECT_EQ(first->second, Rank::from_value(10));
}

TEST_F(ConstructorsTest, FromGeneratorFirstNElements) {
    auto rf = from_generator<int>([](size_t i) {
        return std::make_pair(static_cast<int>(i * 2), Rank::from_value(i));
    });
    
    // Get first 5 elements
    std::vector<int> values;
    auto it = rf.begin();
    for (int count = 0; count < 5 && it != rf.end(); ++count, ++it) {
        values.push_back((*it).first);
    }
    
    EXPECT_EQ(values, (std::vector<int>{0, 2, 4, 6, 8}));
}

TEST_F(ConstructorsTest, FromGeneratorLazyEvaluation) {
    int computation_count = 0;
    
    auto rf = from_generator<int>([&computation_count](size_t i) {
        ++computation_count;
        return std::make_pair(static_cast<int>(i), Rank::from_value(i));
    });
    
    EXPECT_EQ(computation_count, 1);  // Only first element computed in constructor
    
    auto it = rf.begin();
    ++it;  // Force second element
    EXPECT_EQ(computation_count, 2);
}

// ============================================================================
// from_range Tests
// ============================================================================

TEST_F(ConstructorsTest, FromRangeVector) {
    std::vector<int> vec = {1, 2, 3, 4, 5};
    auto rf = from_range(vec);
    
    std::vector<int> values;
    for (auto [value, rank] : rf) {
        values.push_back(value);
    }
    
    EXPECT_EQ(values, vec);
}

TEST_F(ConstructorsTest, FromRangeWithFilter) {
    std::vector<int> vec = {1, 2, 3, 4, 5, 6};
    auto filtered = vec | std::views::filter([](int x) { return x % 2 == 0; });
    auto rf = from_range(filtered);
    
    std::vector<int> values;
    for (auto [value, rank] : rf) {
        values.push_back(value);
    }
    
    EXPECT_EQ(values, (std::vector<int>{2, 4, 6}));
}

TEST_F(ConstructorsTest, FromRangeWithTransform) {
    std::vector<int> vec = {1, 2, 3};
    auto transformed = vec | std::views::transform([](int x) { return x * 10; });
    auto rf = from_range(transformed);
    
    std::vector<int> values;
    for (auto [value, rank] : rf) {
        values.push_back(value);
    }
    
    EXPECT_EQ(values, (std::vector<int>{10, 20, 30}));
}

TEST_F(ConstructorsTest, FromRangeSequentialRanks) {
    std::vector<int> vec = {1, 2, 3};
    auto rf = from_range(vec);
    
    std::vector<uint64_t> ranks;
    for (auto [value, rank] : rf) {
        ranks.push_back(rank.value());
    }
    
    EXPECT_EQ(ranks, (std::vector<uint64_t>{0, 1, 2}));
}

TEST_F(ConstructorsTest, FromRangeCustomStartRank) {
    std::vector<int> vec = {10, 20};
    auto rf = from_range(vec, Rank::from_value(5));
    
    std::vector<uint64_t> ranks;
    for (auto [value, rank] : rf) {
        ranks.push_back(rank.value());
    }
    
    EXPECT_EQ(ranks, (std::vector<uint64_t>{5, 6}));
}

// ============================================================================
// from_pair_range Tests
// ============================================================================

TEST_F(ConstructorsTest, FromPairRangeMap) {
    std::map<int, Rank> map = {
        {1, Rank::zero()},
        {2, Rank::from_value(1)},
        {3, Rank::from_value(2)}
    };
    
    auto rf = from_pair_range(map);
    
    std::vector<int> values;
    for (auto [value, rank] : rf) {
        values.push_back(value);
    }
    
    EXPECT_EQ(values, (std::vector<int>{1, 2, 3}));
}

TEST_F(ConstructorsTest, FromPairRangeVectorOfPairs) {
    std::vector<std::pair<std::string, Rank>> pairs = {
        {"alpha", Rank::zero()},
        {"beta", Rank::from_value(1)},
        {"gamma", Rank::from_value(2)}
    };
    
    auto rf = from_pair_range(pairs);
    
    std::vector<std::string> values;
    for (auto [value, rank] : rf) {
        values.push_back(value);
    }
    
    EXPECT_EQ(values, (std::vector<std::string>{"alpha", "beta", "gamma"}));
}

TEST_F(ConstructorsTest, FromPairRangePreservesRanks) {
    std::vector<std::pair<int, Rank>> pairs = {
        {100, Rank::from_value(10)},
        {200, Rank::from_value(20)}
    };
    
    auto rf = from_pair_range(pairs);
    
    std::vector<uint64_t> ranks;
    for (auto [value, rank] : rf) {
        ranks.push_back(rank.value());
    }
    
    EXPECT_EQ(ranks, (std::vector<uint64_t>{10, 20}));
}

// ============================================================================
// Convenience Alias Tests
// ============================================================================

TEST_F(ConstructorsTest, SingletonAlias) {
    auto rf = singleton(42, Rank::from_value(7));
    
    EXPECT_FALSE(rf.is_empty());
    EXPECT_EQ(rf.size(), 1);
    
    auto first = rf.first();
    ASSERT_TRUE(first.has_value());
    EXPECT_EQ(first->first, 42);
    EXPECT_EQ(first->second, Rank::from_value(7));
}

TEST_F(ConstructorsTest, SingletonAliasDefaultRank) {
    auto rf = singleton(99);
    
    auto first = rf.first();
    ASSERT_TRUE(first.has_value());
    EXPECT_EQ(first->second, Rank::zero());
}

TEST_F(ConstructorsTest, EmptyAlias) {
    auto rf = empty<int>();
    
    EXPECT_TRUE(rf.is_empty());
    EXPECT_EQ(rf.size(), 0);
}

// ============================================================================
// Edge Case and Integration Tests
// ============================================================================

TEST_F(ConstructorsTest, LargeSequence) {
    std::vector<int> values(100);
    std::iota(values.begin(), values.end(), 0);
    
    auto rf = from_values_sequential(values);
    
    EXPECT_EQ(rf.size(), 100);
}

TEST_F(ConstructorsTest, TypeDeduction) {
    // Test type deduction with explicit container types
    std::vector<int> v1{1, 2, 3};
    std::vector<double> v2{1.0, 2.0, 3.0};
    auto rf1 = from_values_uniform(v1);
    auto rf2 = from_values_sequential(v2);
    auto rf3 = singleton(std::string("test"));
    
    static_assert(std::is_same_v<decltype(rf1), RankingFunction<int>>);
    static_assert(std::is_same_v<decltype(rf2), RankingFunction<double>>);
    static_assert(std::is_same_v<decltype(rf3), RankingFunction<std::string>>);
}

TEST_F(ConstructorsTest, MixedConstructionMethods) {
    // from_list
    auto rf1 = from_list<int>({{1, Rank::zero()}});
    
    // from_values_sequential
    std::vector<int> v2{1};
    auto rf2 = from_values_sequential(v2);
    
    // Both should have size 1 with value 1
    EXPECT_EQ(rf1.size(), 1);
    EXPECT_EQ(rf2.size(), 1);
    EXPECT_EQ(rf1.first()->first, 1);
    EXPECT_EQ(rf2.first()->first, 1);
}

TEST_F(ConstructorsTest, DeduplicationConsistency) {
    std::vector<int> values = {1, 1, 2, 2, 3};
    
    auto rf_dedup = from_values_uniform(values, Rank::zero(), Deduplication::Enabled);
    auto rf_no_dedup = from_values_uniform(values, Rank::zero(), Deduplication::Disabled);
    
    EXPECT_EQ(rf_dedup.size(), 3);    // 1, 2, 3 after dedup
    EXPECT_EQ(rf_no_dedup.size(), 5); // All kept
}

TEST_F(ConstructorsTest, ComplexTypeConstruction) {
    std::vector<std::vector<int>> values = {
        {1, 2}, {3, 4}, {5, 6}
    };
    
    auto rf = from_values_sequential(values);
    
    EXPECT_EQ(rf.size(), 3);
    
    auto first = rf.first();
    ASSERT_TRUE(first.has_value());
    EXPECT_EQ(first->first, (std::vector<int>{1, 2}));
}

TEST_F(ConstructorsTest, InfiniteRankValues) {
    auto rf = from_list<int>({
        {1, Rank::zero()},
        {2, Rank::infinity()}
    });
    
    auto it = rf.begin();
    ++it;
    EXPECT_EQ((*it).second, Rank::infinity());
}
