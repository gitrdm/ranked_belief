/**
 * @file merge_apply_test.cpp
 * @brief Tests for merge_apply operations.
 *
 * Tests the applicative merge operation that applies a function returning
 * ranking functions to each element, then merges all results with shifted ranks.
 */

#include <gtest/gtest.h>
#include <ranked_belief/constructors.hpp>
#include <ranked_belief/operations/merge_apply.hpp>
#include <ranked_belief/ranking_function.hpp>
#include <vector>
#include <string>

using namespace ranked_belief;

class MergeApplyTest : public ::testing::Test {
protected:
    // Helper to collect values from a ranking function
    template<typename T>
    std::vector<T> collect_values(const RankingFunction<T>& rf, size_t max_count = 100) {
        std::vector<T> result;
        size_t count = 0;
        for (const auto& [value, rank] : rf) {
            result.push_back(value);
            if (++count >= max_count) break;
        }
        return result;
    }
    
    // Helper to collect value-rank pairs
    template<typename T>
    std::vector<std::pair<T, Rank>> collect_pairs(const RankingFunction<T>& rf, size_t max_count = 100) {
        std::vector<std::pair<T, Rank>> result;
        size_t count = 0;
        for (const auto& [value, rank] : rf) {
            result.push_back({value, rank});
            if (++count >= max_count) break;
        }
        return result;
    }
};

// ============================================================================
// shift_ranks Tests
// ============================================================================

TEST_F(MergeApplyTest, ShiftRanksEmpty) {
    auto rf = make_empty_ranking<int>();
    auto shifted = shift_ranks(rf, Rank::from_value(5));
    
    EXPECT_TRUE(shifted.is_empty());
}

TEST_F(MergeApplyTest, ShiftRanksByZero) {
    std::vector<int> values = {1, 2, 3};
    auto rf = from_values_sequential(values);
    auto shifted = shift_ranks(rf, Rank::zero());
    
    auto pairs = collect_pairs(shifted);
    ASSERT_EQ(pairs.size(), 3);
    EXPECT_EQ(pairs[0].first, 1);
    EXPECT_EQ(pairs[0].second, Rank::from_value(0));
    EXPECT_EQ(pairs[1].first, 2);
    EXPECT_EQ(pairs[1].second, Rank::from_value(1));
    EXPECT_EQ(pairs[2].first, 3);
    EXPECT_EQ(pairs[2].second, Rank::from_value(2));
}

TEST_F(MergeApplyTest, ShiftRanksBasic) {
    std::vector<int> values = {1, 2, 3};
    auto rf = from_values_sequential(values);
    auto shifted = shift_ranks(rf, Rank::from_value(10));
    
    auto pairs = collect_pairs(shifted);
    ASSERT_EQ(pairs.size(), 3);
    EXPECT_EQ(pairs[0].first, 1);
    EXPECT_EQ(pairs[0].second, Rank::from_value(10));
    EXPECT_EQ(pairs[1].first, 2);
    EXPECT_EQ(pairs[1].second, Rank::from_value(11));
    EXPECT_EQ(pairs[2].first, 3);
    EXPECT_EQ(pairs[2].second, Rank::from_value(12));
}

TEST_F(MergeApplyTest, ShiftRanksPreservesValues) {
    std::vector<std::string> values = {"a", "b", "c"};
    auto rf = from_values_uniform(values, Rank::from_value(5));
    auto shifted = shift_ranks(rf, Rank::from_value(3));
    
    auto result = collect_values(shifted);
    EXPECT_EQ(result, values);
    
    // All should be at rank 8 (5 + 3)
    auto pairs = collect_pairs(shifted);
    for (const auto& [val, rank] : pairs) {
        EXPECT_EQ(rank, Rank::from_value(8));
    }
}

TEST_F(MergeApplyTest, ShiftRanksIsLazy) {
    int call_count = 0;
    auto rf = from_generator<int>([&call_count](size_t i) {
        call_count++;
        return std::make_pair(static_cast<int>(i), Rank::from_value(i));
    });
    
    auto shifted = shift_ranks(rf, Rank::from_value(100));
    // Evaluates head when constructing RankingFunction
    EXPECT_EQ(call_count, 1);
    
    auto pairs = collect_pairs(shifted, 3);
    EXPECT_EQ(call_count, 3);  // Only evaluated what we accessed
    EXPECT_EQ(pairs[0].second, Rank::from_value(100));
    EXPECT_EQ(pairs[1].second, Rank::from_value(101));
    EXPECT_EQ(pairs[2].second, Rank::from_value(102));
}

// ============================================================================
// merge_apply Basic Tests
// ============================================================================

TEST_F(MergeApplyTest, MergeApplyEmpty) {
    auto rf = make_empty_ranking<int>();
    auto result = merge_apply(rf, [](int) {
        return from_values_sequential(std::vector<int>{1, 2, 3});
    });
    
    EXPECT_TRUE(result.is_empty());
}

TEST_F(MergeApplyTest, MergeApplySingleElement) {
    auto rf = singleton(5, Rank::zero());
    
    // For value 5, return ranking: 50@0, 51@1
    auto result = merge_apply(rf, [](int n) {
        return from_list<int>({{n*10, Rank::zero()}, {n*10+1, Rank::from_value(1)}});
    });
    
    auto pairs = collect_pairs(result);
    ASSERT_EQ(pairs.size(), 2);
    EXPECT_EQ(pairs[0].first, 50);
    EXPECT_EQ(pairs[0].second, Rank::from_value(0));  // 0 (from 5) + 0 (from result)
    EXPECT_EQ(pairs[1].first, 51);
    EXPECT_EQ(pairs[1].second, Rank::from_value(1));  // 0 (from 5) + 1 (from result)
}

TEST_F(MergeApplyTest, MergeApplyWithRankShift) {
    // Create ranking: 1@0, 2@1, 3@2
    auto rf = from_values_sequential(std::vector<int>{1, 2, 3});
    
    // For each value n, return: n@0, n*10@1
    auto result = merge_apply(rf, [](int n) {
        return from_list<int>({{n, Rank::zero()}, {n*10, Rank::from_value(1)}});
    });
    
    auto pairs = collect_pairs(result);
    ASSERT_EQ(pairs.size(), 6);
    
    // From 1@0: 1@0, 10@1
    EXPECT_EQ(pairs[0].first, 1);
    EXPECT_EQ(pairs[0].second, Rank::from_value(0));
    EXPECT_EQ(pairs[1].first, 10);
    EXPECT_EQ(pairs[1].second, Rank::from_value(1));
    
    // From 2@1: 2@1, 20@2
    EXPECT_EQ(pairs[2].first, 2);
    EXPECT_EQ(pairs[2].second, Rank::from_value(1));
    EXPECT_EQ(pairs[3].first, 20);
    EXPECT_EQ(pairs[3].second, Rank::from_value(2));
    
    // From 3@2: 3@2, 30@3
    EXPECT_EQ(pairs[4].first, 3);
    EXPECT_EQ(pairs[4].second, Rank::from_value(2));
    EXPECT_EQ(pairs[5].first, 30);
    EXPECT_EQ(pairs[5].second, Rank::from_value(3));
}

TEST_F(MergeApplyTest, MergeApplyRankAddition) {
    // Element at rank R produces results at ranks r1, r2, ...
    // Final ranks should be R + r1, R + r2, ...
    
    auto rf = from_list<int>({{1, Rank::from_value(5)}, {2, Rank::from_value(10)}});
    
    auto result = merge_apply(rf, [](int n) {
        // Each produces: n@0, n*10@2
        return from_list<int>({{n, Rank::zero()}, {n*10, Rank::from_value(2)}});
    });
    
    auto pairs = collect_pairs(result);
    ASSERT_EQ(pairs.size(), 4);
    
    // From 1@5: 1@5, 10@7
    EXPECT_EQ(pairs[0].first, 1);
    EXPECT_EQ(pairs[0].second, Rank::from_value(5));
    EXPECT_EQ(pairs[1].first, 10);
    EXPECT_EQ(pairs[1].second, Rank::from_value(7));
    
    // From 2@10: 2@10, 20@12
    EXPECT_EQ(pairs[2].first, 2);
    EXPECT_EQ(pairs[2].second, Rank::from_value(10));
    EXPECT_EQ(pairs[3].first, 20);
    EXPECT_EQ(pairs[3].second, Rank::from_value(12));
}

TEST_F(MergeApplyTest, MergeApplyMergesInRankOrder) {
    // Create: A@0, B@5
    auto rf = from_list<std::string>({{"A", Rank::zero()}, {"B", Rank::from_value(5)}});
    
    // Each produces: value@0, value@3
    auto result = merge_apply(rf, [](const std::string& s) {
        return from_list<std::string>({{s, Rank::zero()}, {s + s, Rank::from_value(3)}});
    });
    
    auto pairs = collect_pairs(result);
    ASSERT_EQ(pairs.size(), 4);
    
    // Results should be merged by rank:
    // A@0 (from A), AAA@3 (from A), B@5 (from B), BB@8 (from B)
    EXPECT_EQ(pairs[0].first, "A");
    EXPECT_EQ(pairs[0].second, Rank::from_value(0));
    EXPECT_EQ(pairs[1].first, "AA");
    EXPECT_EQ(pairs[1].second, Rank::from_value(3));
    EXPECT_EQ(pairs[2].first, "B");
    EXPECT_EQ(pairs[2].second, Rank::from_value(5));
    EXPECT_EQ(pairs[3].first, "BB");
    EXPECT_EQ(pairs[3].second, Rank::from_value(8));
}

// ============================================================================
// Lazy Evaluation Tests
// ============================================================================

TEST_F(MergeApplyTest, MergeApplyIsLazy) {
    int generator_calls = 0;
    int func_calls = 0;
    
    auto rf = from_generator<int>([&generator_calls](size_t i) {
        generator_calls++;
        return std::make_pair(static_cast<int>(i), Rank::from_value(i));
    });
    
    auto result = merge_apply(rf, [&func_calls](int n) {
        func_calls++;
        return from_list<int>({{n, Rank::zero()}, {n*10, Rank::from_value(1)}});
    });
    
    EXPECT_EQ(generator_calls, 2);
    EXPECT_EQ(func_calls, 1);
    
    // Access first 4 result elements
    // This should come from: 0 (produces 0@0, 0@1), 1 (produces 1@1, 10@2)
    auto values = collect_values(result, 4);
    
    EXPECT_EQ(generator_calls, 4);
    EXPECT_EQ(func_calls, 3);
}

TEST_F(MergeApplyTest, MergeApplyDoesNotComputeAhead) {
    int func_calls = 0;
    
    auto rf = from_values_sequential(std::vector<int>{1, 2, 3, 4, 5});
    
    auto result = merge_apply(rf, [&func_calls](int n) {
        func_calls++;
        return singleton(n * 10);
    });
    
    // Take only first 3 elements
    auto values = collect_values(result, 3);
    
    EXPECT_EQ(values.size(), 3);
    EXPECT_LE(func_calls, 3);  // Should not process more than needed
}

// ============================================================================
// Deduplication Tests
// ============================================================================

TEST_F(MergeApplyTest, MergeApplyWithDeduplication) {
    // Create: 1@0, 2@0
    auto rf = from_values_uniform(std::vector<int>{1, 2});
    
    // Each produces same result: 5@0
    auto result = merge_apply(rf, [](int) {
        return singleton(5);
    });
    
    auto values = collect_values(result);
    // With deduplication (default), should get 5 only once
    EXPECT_EQ(values.size(), 1);
    EXPECT_EQ(values[0], 5);
}

TEST_F(MergeApplyTest, MergeApplyWithoutDeduplication) {
    // Create: 1@0, 2@0
    auto rf = from_values_uniform(std::vector<int>{1, 2});
    
    // Each produces same result: 5@0
    auto result = merge_apply(rf, [](int) {
        return singleton(5);
    }, Deduplication::Disabled);  // No deduplication
    
    auto values = collect_values(result);
    // Without deduplication, should get 5 twice
    EXPECT_EQ(values.size(), 2);
    EXPECT_EQ(values[0], 5);
    EXPECT_EQ(values[1], 5);
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(MergeApplyTest, MergeApplyFunctionReturnsEmpty) {
    auto rf = from_values_sequential(std::vector<int>{1, 2, 3});
    
    // Function returns empty ranking for all values
    auto result = merge_apply(rf, [](int) {
        return make_empty_ranking<int>();
    });
    
    EXPECT_TRUE(result.is_empty());
}

TEST_F(MergeApplyTest, MergeApplyMixedEmptyAndNonEmpty) {
    auto rf = from_values_sequential(std::vector<int>{1, 2, 3});
    
    // Even values return empty, odd values return singleton
    auto result = merge_apply(rf, [](int n) {
        if (n % 2 == 0) {
            return make_empty_ranking<int>();
        } else {
            return singleton(n * 10);
        }
    });
    
    auto values = collect_values(result);
    ASSERT_EQ(values.size(), 2);
    EXPECT_EQ(values[0], 10);  // From 1@0
    EXPECT_EQ(values[1], 30);  // From 3@2
}

TEST_F(MergeApplyTest, MergeApplyWithLargeRanks) {
    auto rf = from_list<int>({{1, Rank::from_value(1000)}, {2, Rank::from_value(2000)}});
    
    auto result = merge_apply(rf, [](int n) {
        return from_list<int>({{n, Rank::from_value(100)}, {n*10, Rank::from_value(200)}});
    });
    
    auto pairs = collect_pairs(result);
    ASSERT_EQ(pairs.size(), 4);
    
    EXPECT_EQ(pairs[0].first, 1);
    EXPECT_EQ(pairs[0].second, Rank::from_value(1100));
    EXPECT_EQ(pairs[1].first, 10);
    EXPECT_EQ(pairs[1].second, Rank::from_value(1200));
    EXPECT_EQ(pairs[2].first, 2);
    EXPECT_EQ(pairs[2].second, Rank::from_value(2100));
    EXPECT_EQ(pairs[3].first, 20);
    EXPECT_EQ(pairs[3].second, Rank::from_value(2200));
}

// ============================================================================
// Type Transformation Tests
// ============================================================================

TEST_F(MergeApplyTest, MergeApplyIntToString) {
    auto rf = from_values_sequential(std::vector<int>{1, 2, 3});
    
    auto result = merge_apply(rf, [](int n) {
        return from_list<std::string>({
            {std::to_string(n), Rank::zero()},
            {std::to_string(n*10), Rank::from_value(1)}
        });
    });
    
    auto values = collect_values(result);
    ASSERT_EQ(values.size(), 6);
    EXPECT_EQ(values[0], "1");
    EXPECT_EQ(values[1], "10");
    EXPECT_EQ(values[2], "2");
    EXPECT_EQ(values[3], "20");
    EXPECT_EQ(values[4], "3");
    EXPECT_EQ(values[5], "30");
}

TEST_F(MergeApplyTest, MergeApplyToComplexType) {
    struct Result {
        int original;
        int doubled;
        bool operator==(const Result& other) const {
            return original == other.original && doubled == other.doubled;
        }
    };
    
    auto rf = from_values_sequential(std::vector<int>{5, 10});
    
    auto result = merge_apply(rf, [](int n) {
        return singleton(Result{n, n*2});
    });
    
    auto values = collect_values(result);
    ASSERT_EQ(values.size(), 2);
    EXPECT_EQ(values[0].original, 5);
    EXPECT_EQ(values[0].doubled, 10);
    EXPECT_EQ(values[1].original, 10);
    EXPECT_EQ(values[1].doubled, 20);
}

// ============================================================================
// Chaining Tests
// ============================================================================

TEST_F(MergeApplyTest, ChainMultipleMergeApplies) {
    // Start with 1@0, 2@1
    auto rf = from_values_sequential(std::vector<int>{1, 2});
    
    // First merge_apply: each n produces n@0, n+1@1
    auto result1 = merge_apply(rf, [](int n) {
        return from_list<int>({{n, Rank::zero()}, {n+1, Rank::from_value(1)}});
    });
    
    // Second merge_apply: each n produces n*10@0
    auto result2 = merge_apply(result1, [](int n) {
        return singleton(n * 10);
    });
    
    auto pairs = collect_pairs(result2);
    // From 1@0: 1@0 -> 10@0, 2@1 -> 20@1
    // From 2@1: 2@1 -> 20@1, 3@2 -> 30@2
    // After deduplication: 10@0, 20@1, 30@2
    
    EXPECT_EQ(pairs[0].first, 10);
    EXPECT_EQ(pairs[0].second, Rank::from_value(0));
    EXPECT_EQ(pairs[1].first, 20);
    EXPECT_EQ(pairs[1].second, Rank::from_value(1));
    EXPECT_EQ(pairs[2].first, 30);
    EXPECT_EQ(pairs[2].second, Rank::from_value(2));
}

// ============================================================================
// Performance/Stress Tests
// ============================================================================

TEST_F(MergeApplyTest, MergeApplyManyElements) {
    // Create ranking with 100 elements
    std::vector<int> values(100);
    for (int i = 0; i < 100; ++i) {
        values[i] = i;
    }
    auto rf = from_values_sequential(values);
    
    // Each element produces 2 results
    auto result = merge_apply(rf, [](int n) {
        return from_list<int>({{n, Rank::zero()}, {n+1000, Rank::from_value(1)}});
    });
    
    // Should have 200 elements total
    auto result_values = collect_values(result, 300);
    EXPECT_EQ(result_values.size(), 200);
    
    // Verify first few elements
    EXPECT_EQ(result_values[0], 0);
    EXPECT_EQ(result_values[1], 1000);
    EXPECT_EQ(result_values[2], 1);
    EXPECT_EQ(result_values[3], 1001);
}

TEST_F(MergeApplyTest, MergeApplyWithVariableSizeResults) {
    auto rf = from_values_sequential(std::vector<int>{1, 2, 3, 4, 5});
    
    // Each n produces n results
    auto result = merge_apply(rf, [](int n) {
        std::vector<std::pair<int, Rank>> pairs;
        for (int i = 0; i < n; ++i) {
            pairs.push_back({n * 10 + i, Rank::from_value(i)});
        }
        return from_list<int>(pairs);
    });
    
    // Should have 1 + 2 + 3 + 4 + 5 = 15 elements
    auto values = collect_values(result, 20);
    EXPECT_EQ(values.size(), 15);
}
