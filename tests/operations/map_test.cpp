/**
 * @file map_test.cpp
 * @brief Comprehensive tests for map operations on RankingFunction
 */

#include "ranked_belief/operations/map.hpp"
#include "ranked_belief/constructors.hpp"

#include <gtest/gtest.h>

#include <string>
#include <utility>
#include <vector>

using namespace ranked_belief;

/// Test fixture for map operations
class MapOperationsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Common test data will be set up here if needed
    }
};

// ============================================================================
// Basic map Tests
// ============================================================================

TEST_F(MapOperationsTest, MapEmptyRanking) {
    auto rf = empty<int>();
    auto mapped = map(rf, [](int x) { return x * 2; });
    
    EXPECT_TRUE(mapped.is_empty());
}

TEST_F(MapOperationsTest, MapSingleElement) {
    auto rf = singleton(5, Rank::from_value(2));
    auto mapped = map(rf, [](int x) { return x * 2; });
    
    EXPECT_FALSE(mapped.is_empty());
    EXPECT_EQ(mapped.size(), 1);
    
    auto first = mapped.first();
    ASSERT_TRUE(first.has_value());
    EXPECT_EQ(first->first, 10);
    EXPECT_EQ(first->second, Rank::from_value(2));
}

TEST_F(MapOperationsTest, MapMultipleElements) {
    std::vector<int> values{1, 2, 3, 4, 5};
    auto rf = from_values_sequential(values);
    auto mapped = map(rf, [](int x) { return x * 2; });
    
    std::vector<std::pair<int, uint64_t>> result;
    for (auto [value, rank] : mapped) {
        result.emplace_back(value, rank.value());
    }
    
    std::vector<std::pair<int, uint64_t>> expected = {
        {2, 0}, {4, 1}, {6, 2}, {8, 3}, {10, 4}
    };
    EXPECT_EQ(result, expected);
}

TEST_F(MapOperationsTest, MapPreservesRanks) {
    auto rf = from_list<int>({
        {10, Rank::from_value(5)},
        {20, Rank::from_value(3)},
        {30, Rank::from_value(8)}
    });
    
    auto mapped = map(rf, [](int x) { return x / 10; });
    
    std::vector<std::pair<int, uint64_t>> result;
    for (auto [value, rank] : mapped) {
        result.emplace_back(value, rank.value());
    }
    
    std::vector<std::pair<int, uint64_t>> expected = {
        {1, 5}, {2, 3}, {3, 8}
    };
    EXPECT_EQ(result, expected);
}

TEST_F(MapOperationsTest, MapWithTypeTransformation) {
    std::vector<int> values{1, 2, 3};
    auto rf = from_values_sequential(values);
    auto mapped = map(rf, [](int x) { return std::to_string(x); });
    
    std::vector<std::string> result;
    for (auto [value, rank] : mapped) {
        result.push_back(value);
    }
    
    std::vector<std::string> expected = {"1", "2", "3"};
    EXPECT_EQ(result, expected);
}

TEST_F(MapOperationsTest, MapIntToDouble) {
    std::vector<int> values{1, 2, 3};
    auto rf = from_values_uniform(values);
    auto mapped = map(rf, [](int x) { return x * 0.5; });
    
    std::vector<double> result;
    for (auto [value, rank] : mapped) {
        result.push_back(value);
    }
    
    std::vector<double> expected = {0.5, 1.0, 1.5};
    EXPECT_EQ(result, expected);
}

TEST_F(MapOperationsTest, MapToComplexType) {
    std::vector<int> values{1, 2, 3};
    auto rf = from_values_sequential(values);
    auto mapped = map(rf, [](int x) {
        return std::make_pair(x, x * x);
    });
    
    std::vector<std::pair<int, int>> result;
    for (auto [value, rank] : mapped) {
        result.push_back(value);
    }
    
    std::vector<std::pair<int, int>> expected = {
        {1, 1}, {2, 4}, {3, 9}
    };
    EXPECT_EQ(result, expected);
}

// ============================================================================
// Lazy Evaluation Tests
// ============================================================================

TEST_F(MapOperationsTest, MapIsLazy) {
    int call_count = 0;
    auto rf = from_values_sequential(std::vector<int>{1, 2, 3, 4, 5});
    
    auto mapped = map(rf, [&call_count](int x) {
        ++call_count;
        return x * 2;
    });
    
    // Function should not have been called yet
    EXPECT_EQ(call_count, 0);
    
    // Access first element
    [[maybe_unused]] auto first = mapped.first();
    EXPECT_EQ(call_count, 1);  // Called once for first element
    
    // Access first element again (should be memoized)
    [[maybe_unused]] auto first_again = mapped.first();
    EXPECT_EQ(call_count, 1);  // Still only called once
}

TEST_F(MapOperationsTest, MapComputesOnDemand) {
    int call_count = 0;
    auto rf = from_values_sequential(std::vector<int>{1, 2, 3, 4, 5});
    
    auto mapped = map(rf, [&call_count](int x) {
        ++call_count;
        return x * 2;
    });
    
    // Iterate through first 3 elements
    int count = 0;
    for ([[maybe_unused]] auto [value, rank] : mapped) {
        ++count;
        if (count == 3) break;
    }
    
    // Function should have been called exactly 3 times
    EXPECT_EQ(call_count, 3);
}

TEST_F(MapOperationsTest, MapMemoizesResults) {
    int call_count = 0;
    auto rf = from_values_sequential(std::vector<int>{1, 2, 3});
    
    auto mapped = map(rf, [&call_count](int x) {
        ++call_count;
        return x * 2;
    });
    
    // First iteration
    for ([[maybe_unused]] auto [value, rank] : mapped) {
        // Just iterate
    }
    int first_count = call_count;
    
    // Second iteration (should use memoized values)
    for ([[maybe_unused]] auto [value, rank] : mapped) {
        // Just iterate
    }
    
    // Count should not have increased
    EXPECT_EQ(call_count, first_count);
}

// ============================================================================
// Deduplication Tests
// ============================================================================

TEST_F(MapOperationsTest, MapWithDeduplication) {
    auto rf = from_list<int>({
        {1, Rank::zero()},
        {2, Rank::from_value(1)},
        {2, Rank::from_value(2)},
        {3, Rank::from_value(3)}
    }, false);  // No deduplication in source
    
    // Map that doesn't change values, with deduplication enabled
    auto mapped = map(rf, [](int x) { return x; }, true);
    
    std::vector<int> result;
    for (auto [value, rank] : mapped) {
        result.push_back(value);
    }
    
    std::vector<int> expected = {1, 2, 3};  // 2 appears only once
    EXPECT_EQ(result, expected);
}

TEST_F(MapOperationsTest, MapWithoutDeduplication) {
    auto rf = from_list<int>({
        {1, Rank::zero()},
        {2, Rank::from_value(1)},
        {2, Rank::from_value(2)},
        {3, Rank::from_value(3)}
    }, false);
    
    // Map with deduplication disabled
    auto mapped = map(rf, [](int x) { return x; }, false);
    
    std::vector<int> result;
    for (auto [value, rank] : mapped) {
        result.push_back(value);
    }
    
    std::vector<int> expected = {1, 2, 2, 3};  // 2 appears twice
    EXPECT_EQ(result, expected);
}

TEST_F(MapOperationsTest, MapCreatesNewDuplicates) {
    auto rf = from_values_sequential(std::vector<int>{1, 2, 3, 4});
    
    // Map that creates duplicates (multiple inputs map to same output)
    auto mapped = map(rf, [](int x) { return x / 2; }, true);
    
    std::vector<int> result;
    for (auto [value, rank] : mapped) {
        result.push_back(value);
    }
    
    std::vector<int> expected = {0, 1, 2};  // 1/2=0, 2/2=1, 3/2=1, 4/2=2
                                             // With dedup: 0, 1, 2
    EXPECT_EQ(result, expected);
}

// ============================================================================
// map_with_rank Tests
// ============================================================================

TEST_F(MapOperationsTest, MapWithRankBasic) {
    std::vector<int> values{10, 20, 30};
    auto rf = from_values_sequential(values);
    
    auto mapped = map_with_rank(rf, [](int x, Rank r) {
        return std::make_pair(x + static_cast<int>(r.value()), r + Rank::from_value(5));
    });
    
    std::vector<std::pair<int, uint64_t>> result;
    for (auto [value, rank] : mapped) {
        result.emplace_back(value, rank.value());
    }
    
    std::vector<std::pair<int, uint64_t>> expected = {
        {10, 5},  // 10 + 0, rank 0 + 5
        {21, 6},  // 20 + 1, rank 1 + 5
        {32, 7}   // 30 + 2, rank 2 + 5
    };
    EXPECT_EQ(result, expected);
}

TEST_F(MapOperationsTest, MapWithRankCanChangeRanks) {
    auto rf = from_values_sequential(std::vector<int>{10, 20, 30});
    
    // Map that doubles values and sets rank based on value
    auto mapped = map_with_rank(rf, [](int x, [[maybe_unused]] Rank r) {
        return std::make_pair(x * 2, Rank::from_value(x / 10));
    });
    
    std::vector<std::pair<int, Rank>> result;
    for (auto [value, rank] : mapped) {
        result.emplace_back(value, rank);
    }
    
    ASSERT_EQ(result.size(), 3);
    EXPECT_EQ(result[0].first, 20);
    EXPECT_EQ(result[0].second, Rank::from_value(1));
    EXPECT_EQ(result[1].first, 40);
    EXPECT_EQ(result[1].second, Rank::from_value(2));
    EXPECT_EQ(result[2].first, 60);
    EXPECT_EQ(result[2].second, Rank::from_value(3));
}

TEST_F(MapOperationsTest, MapWithRankEmpty) {
    auto rf = empty<int>();
    auto mapped = map_with_rank(rf, [](int x, Rank r) {
        return std::make_pair(x, r);
    });
    
    EXPECT_TRUE(mapped.is_empty());
}

// ============================================================================
// map_with_index Tests
// ============================================================================

TEST_F(MapOperationsTest, MapWithIndexBasic) {
    std::vector<std::string> values{"a", "b", "c"};
    auto rf = from_values_uniform(values);
    
    auto mapped = map_with_index(rf, [](const std::string& s, size_t idx) {
        return s + std::to_string(idx);
    });
    
    std::vector<std::string> result;
    for (auto [value, rank] : mapped) {
        result.push_back(value);
    }
    
    std::vector<std::string> expected = {"a0", "b1", "c2"};
    EXPECT_EQ(result, expected);
}

TEST_F(MapOperationsTest, MapWithIndexPreservesRanks) {
    auto rf = from_list<int>({
        {10, Rank::from_value(5)},
        {20, Rank::from_value(3)},
        {30, Rank::from_value(8)}
    });
    
    auto mapped = map_with_index(rf, [](int x, size_t idx) {
        return x + static_cast<int>(idx);
    });
    
    std::vector<std::pair<int, uint64_t>> result;
    for (auto [value, rank] : mapped) {
        result.emplace_back(value, rank.value());
    }
    
    std::vector<std::pair<int, uint64_t>> expected = {
        {10, 5},  // 10 + 0, rank 5
        {21, 3},  // 20 + 1, rank 3
        {32, 8}   // 30 + 2, rank 8
    };
    EXPECT_EQ(result, expected);
}

TEST_F(MapOperationsTest, MapWithIndexEmpty) {
    auto rf = empty<int>();
    auto mapped = map_with_index(rf, [](int x, size_t idx) {
        return x + static_cast<int>(idx);
    });
    
    EXPECT_TRUE(mapped.is_empty());
}

TEST_F(MapOperationsTest, MapWithIndexCreatesIndexedPairs) {
    std::vector<int> values{100, 200, 300};
    auto rf = from_values_sequential(values);
    
    auto mapped = map_with_index(rf, [](int x, size_t idx) {
        return std::make_pair(idx, x);
    });
    
    std::vector<std::pair<size_t, int>> result;
    for (auto [value, rank] : mapped) {
        result.push_back(value);
    }
    
    std::vector<std::pair<size_t, int>> expected = {
        {0, 100}, {1, 200}, {2, 300}
    };
    EXPECT_EQ(result, expected);
}

// ============================================================================
// Exception Handling Tests
// ============================================================================

TEST_F(MapOperationsTest, MapFunctionThrows) {
    std::vector<int> values{1, 2, 0, 4};  // 0 will cause division by zero
    auto rf = from_values_sequential(values);
    
    auto mapped = map(rf, [](int x) {
        if (x == 0) {
            throw std::runtime_error("Division by zero");
        }
        return 10 / x;
    });
    
    // Should not throw until we access the problematic element
    [[maybe_unused]] auto first_result = mapped.first();
    EXPECT_TRUE(first_result.has_value());
    
    // Iterate until we hit the exception
    EXPECT_THROW({
        for ([[maybe_unused]] auto [value, rank] : mapped) {
            // Will throw when accessing the third element (0)
        }
    }, std::runtime_error);
}

TEST_F(MapOperationsTest, MapExceptionIsPropagated) {
    auto rf = singleton(5);
    
    auto mapped = map(rf, [](int /*x*/) -> int {
        throw std::invalid_argument("Test exception");
    });
    
    EXPECT_THROW([[maybe_unused]] auto result = mapped.first(), std::invalid_argument);
}

// ============================================================================
// Chaining and Composition Tests
// ============================================================================

TEST_F(MapOperationsTest, ChainMultipleMaps) {
    std::vector<int> values{1, 2, 3};
    auto rf = from_values_sequential(values);
    
    auto mapped1 = map(rf, [](int x) { return x * 2; });
    auto mapped2 = map(mapped1, [](int x) { return x + 10; });
    auto mapped3 = map(mapped2, [](int x) { return std::to_string(x); });
    
    std::vector<std::string> result;
    for (auto [value, rank] : mapped3) {
        result.push_back(value);
    }
    
    std::vector<std::string> expected = {"12", "14", "16"};
    EXPECT_EQ(result, expected);
}

TEST_F(MapOperationsTest, ChainMapWithIndex) {
    std::vector<int> values{10, 20, 30};
    auto rf = from_values_uniform(values);
    
    auto indexed = map_with_index(rf, [](int x, size_t idx) {
        return x + static_cast<int>(idx);
    });
    
    auto doubled = map(indexed, [](int x) { return x * 2; });
    
    std::vector<int> result;
    for (auto [value, rank] : doubled) {
        result.push_back(value);
    }
    
    std::vector<int> expected = {20, 42, 64};  // (10+0)*2, (20+1)*2, (30+2)*2
    EXPECT_EQ(result, expected);
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(MapOperationsTest, MapIdentityFunction) {
    std::vector<int> values{1, 2, 3, 4, 5};
    auto rf = from_values_sequential(values);
    
    auto mapped = map(rf, [](int x) { return x; });
    
    std::vector<int> original;
    for (auto [value, rank] : rf) {
        original.push_back(value);
    }
    
    std::vector<int> result;
    for (auto [value, rank] : mapped) {
        result.push_back(value);
    }
    
    EXPECT_EQ(result, original);
}

TEST_F(MapOperationsTest, MapConstantFunction) {
    std::vector<int> values{1, 2, 3, 4, 5};
    auto rf = from_values_sequential(values);
    
    auto mapped = map(rf, [](int) { return 42; }, true);
    
    // With deduplication, all map to same value
    EXPECT_EQ(mapped.size(), 1);
    EXPECT_EQ(mapped.first()->first, 42);
}

TEST_F(MapOperationsTest, MapLargeSequence) {
    std::vector<int> values(1000);
    for (size_t i = 0; i < values.size(); ++i) {
        values[i] = static_cast<int>(i);
    }
    auto rf = from_values_sequential(values);
    
    auto mapped = map(rf, [](int x) { return x * x; });
    
    EXPECT_EQ(mapped.size(), 1000);
    
    // Check first and last
    auto first = mapped.first();
    EXPECT_EQ(first->first, 0);
    
    // Iterate to get last
    int last_value = 0;
    for (auto [value, rank] : mapped) {
        last_value = value;
    }
    EXPECT_EQ(last_value, 999 * 999);
}

TEST_F(MapOperationsTest, MapWithCapture) {
    int multiplier = 5;
    std::vector<int> values{1, 2, 3};
    auto rf = from_values_sequential(values);
    
    auto mapped = map(rf, [multiplier](int x) { return x * multiplier; });
    
    std::vector<int> result;
    for (auto [value, rank] : mapped) {
        result.push_back(value);
    }
    
    std::vector<int> expected = {5, 10, 15};
    EXPECT_EQ(result, expected);
}

TEST_F(MapOperationsTest, MapWithMutableCapture) {
    std::vector<int> values{1, 2, 3};
    auto rf = from_values_sequential(values);
    
    int sum = 0;
    auto mapped = map(rf, [&sum](int x) {
        sum += x;
        return sum;
    });
    
    std::vector<int> result;
    for (auto [value, rank] : mapped) {
        result.push_back(value);
    }
    
    std::vector<int> expected = {1, 3, 6};  // Running sum
    EXPECT_EQ(result, expected);
}
