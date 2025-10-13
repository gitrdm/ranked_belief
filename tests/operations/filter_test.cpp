/**
 * @file filter_test.cpp
 * @brief Comprehensive tests for filter operations.
 */

#include "ranked_belief/operations/filter.hpp"
#include "ranked_belief/operations/map.hpp"
#include "ranked_belief/constructors.hpp"
#include <gtest/gtest.h>
#include <stdexcept>
#include <string>
#include <vector>

using namespace ranked_belief;

// ============================================================================
// Test Fixture
// ============================================================================

class FilterTest : public ::testing::Test {
protected:
    // Helper to collect values from a ranking function
    template<typename T>
    std::vector<T> collect_values(const RankingFunction<T>& rf, std::size_t max_count = 100) {
        std::vector<T> result;
        auto it = rf.begin();
        auto end = rf.end();
        
        for (std::size_t i = 0; i < max_count && it != end; ++i, ++it) {
            result.push_back((*it).first);  // Extract value from pair
        }
        
        return result;
    }
    
    // Helper to collect ranks
    template<typename T>
    std::vector<Rank> collect_ranks(const RankingFunction<T>& rf, std::size_t max_count = 100) {
        std::vector<Rank> result;
        auto it = rf.begin();
        auto end = rf.end();
        
        for (std::size_t i = 0; i < max_count && it != end; ++i, ++it) {
            result.push_back((*it).second);  // Extract rank from pair
        }
        
        return result;
    }
};

// ============================================================================
// filter() - Basic Tests
// ============================================================================

TEST_F(FilterTest, FilterEmptySequence) {
    auto rf = empty<int>();
    auto filtered = filter(rf, [](int x) { return x > 0; });
    
    EXPECT_TRUE(filtered.is_empty());
}

TEST_F(FilterTest, FilterSingleElementMatching) {
    auto rf = singleton(42);
    auto filtered = filter(rf, [](int x) { return x > 0; });
    
    auto values = collect_values(filtered);
    ASSERT_EQ(values.size(), 1);
    EXPECT_EQ(values[0], 42);
}

TEST_F(FilterTest, FilterSingleElementNotMatching) {
    auto rf = singleton(-42);
    auto filtered = filter(rf, [](int x) { return x > 0; });
    
    EXPECT_TRUE(filtered.is_empty());
}

TEST_F(FilterTest, FilterMultipleElementsSomeMatch) {
    std::vector<int> values = {1, 2, 3, 4, 5};
    auto rf = from_values_sequential(values);
    auto evens = filter(rf, [](int x) { return x % 2 == 0; });
    
    auto result = collect_values(evens);
    std::vector<int> expected = {2, 4};
    EXPECT_EQ(result, expected);
}

TEST_F(FilterTest, FilterMultipleElementsAllMatch) {
    std::vector<int> values = {2, 4, 6, 8};
    auto rf = from_values_sequential(values);
    auto evens = filter(rf, [](int x) { return x % 2 == 0; });
    
    auto result = collect_values(evens);
    std::vector<int> expected = {2, 4, 6, 8};
    EXPECT_EQ(result, expected);
}

TEST_F(FilterTest, FilterMultipleElementsNoneMatch) {
    std::vector<int> values = {1, 3, 5, 7};
    auto rf = from_values_sequential(values);
    auto evens = filter(rf, [](int x) { return x % 2 == 0; });
    
    EXPECT_TRUE(evens.is_empty());
}

// ============================================================================
// filter() - Rank Preservation
// ============================================================================

TEST_F(FilterTest, FilterPreservesOriginalRanks) {
    std::vector<int> values = {1, 2, 3, 4, 5};
    auto rf = from_values_sequential(values);  // ranks: 0,1,2,3,4
    auto evens = filter(rf, [](int x) { return x % 2 == 0; });
    
    auto ranks = collect_ranks(evens);
    std::vector<Rank> expected = {Rank::from_value(1), Rank::from_value(3)};
    EXPECT_EQ(ranks, expected);
}

TEST_F(FilterTest, FilterPreservesUniformRank) {
    std::vector<int> values = {1, 2, 3, 4, 5};
    auto rf = from_values_uniform(values);  // all rank 0
    auto evens = filter(rf, [](int x) { return x % 2 == 0; });
    
    auto ranks = collect_ranks(evens);
    for (const auto& rank : ranks) {
        EXPECT_EQ(rank, Rank::from_value(0));
    }
}

TEST_F(FilterTest, FilterPreservesNonSequentialRanks) {
    auto rf = from_list<int>({
        {10, Rank::from_value(0)},
        {20, Rank::from_value(2)},
        {30, Rank::from_value(5)},
        {40, Rank::from_value(10)}
    });
    
    auto filtered = filter(rf, [](int x) { return x >= 20; });
    
    auto values = collect_values(filtered);
    auto ranks = collect_ranks(filtered);
    
    std::vector<int> expected_values = {20, 30, 40};
    std::vector<Rank> expected_ranks = {
        Rank::from_value(2), Rank::from_value(5), Rank::from_value(10)
    };
    
    EXPECT_EQ(values, expected_values);
    EXPECT_EQ(ranks, expected_ranks);
}

// ============================================================================
// filter() - Lazy Evaluation
// ============================================================================

TEST_F(FilterTest, FilterIsLazy) {
    int call_count = 0;
    auto rf = from_generator<int>([&call_count](std::size_t i) {
        ++call_count;
        return std::make_pair(static_cast<int>(i), Rank::from_value(i));
    });
    
    // from_generator evaluates first element immediately
    EXPECT_EQ(call_count, 1);
    
    // Creating the filter should not evaluate more elements
    auto filtered = filter(rf, [](int x) { return x % 2 == 0; });
    EXPECT_EQ(call_count, 1);
    
    // First element (0) matches, so accessing it shouldn't evaluate more
    auto it = filtered.begin();
    EXPECT_EQ(call_count, 1);
    
    // Moving to second element will evaluate elements until finding next match
    ++it;
    EXPECT_GT(call_count, 1);  // Will evaluate more elements
    EXPECT_LE(call_count, 4);  // But not too many (0 matches, skip 1, find 2)
}

TEST_F(FilterTest, FilterWithInfiniteSequence) {
    auto rf = from_generator<int>([](std::size_t i) {
        return std::make_pair(static_cast<int>(i), Rank::from_value(i));
    });
    
    auto evens = filter(rf, [](int x) { return x % 2 == 0; });
    
    // Take only first 5 evens
    auto values = collect_values(evens, 5);
    
    std::vector<int> expected = {0, 2, 4, 6, 8};
    EXPECT_EQ(values, expected);
}

// ============================================================================
// filter() - Complex Predicates
// ============================================================================

TEST_F(FilterTest, FilterWithComplexPredicate) {
    std::vector<int> values = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    auto rf = from_values_sequential(values);
    
    // Filter for multiples of 2 or 3
    auto filtered = filter(rf, [](int x) {
        return (x % 2 == 0) || (x % 3 == 0);
    });
    
    auto result = collect_values(filtered);
    std::vector<int> expected = {2, 3, 4, 6, 8, 9, 10};
    EXPECT_EQ(result, expected);
}

TEST_F(FilterTest, FilterStringsWithPredicate) {
    std::vector<std::string> values = {"apple", "banana", "apricot", "cherry", "avocado"};
    auto rf = from_values_sequential(values);
    
    // Filter for strings starting with 'a'
    auto filtered = filter(rf, [](const std::string& s) {
        return !s.empty() && s[0] == 'a';
    });
    
    auto result = collect_values(filtered);
    std::vector<std::string> expected = {"apple", "apricot", "avocado"};
    EXPECT_EQ(result, expected);
}

// ============================================================================
// filter() - Deduplication
// ============================================================================

TEST_F(FilterTest, FilterWithDeduplication) {
    std::vector<int> values = {1, 2, 2, 3, 3, 3, 4};
    auto rf = from_values_uniform(values);
    auto evens = filter(rf, [](int x) { return x % 2 == 0; }, true);
    
    auto result = collect_values(evens);
    std::vector<int> expected = {2, 4};
    EXPECT_EQ(result, expected);
}

TEST_F(FilterTest, FilterWithoutDeduplication) {
    std::vector<int> values = {1, 2, 2, 3, 4, 4, 4};
    auto rf = from_values_uniform(values);
    auto evens = filter(rf, [](int x) { return x % 2 == 0; }, false);
    
    auto result = collect_values(evens);
    std::vector<int> expected = {2, 2, 4, 4, 4};
    EXPECT_EQ(result, expected);
}

// ============================================================================
// filter() - Chaining Operations
// ============================================================================

TEST_F(FilterTest, ChainMultipleFilters) {
    std::vector<int> values = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    auto rf = from_values_sequential(values);
    
    // First filter evens, then filter those > 5
    auto evens = filter(rf, [](int x) { return x % 2 == 0; });
    auto large_evens = filter(evens, [](int x) { return x > 5; });
    
    auto result = collect_values(large_evens);
    std::vector<int> expected = {6, 8, 10};
    EXPECT_EQ(result, expected);
}

TEST_F(FilterTest, FilterThenMap) {
    std::vector<int> values = {1, 2, 3, 4, 5};
    auto rf = from_values_sequential(values);
    
    // Filter evens, then square them
    auto evens = filter(rf, [](int x) { return x % 2 == 0; });
    auto squared = map(evens, [](int x) { return x * x; });
    
    auto result = collect_values(squared);
    std::vector<int> expected = {4, 16};
    EXPECT_EQ(result, expected);
}

// ============================================================================
// filter() - Exception Handling
// ============================================================================

TEST_F(FilterTest, FilterPredicateThrowsException) {
    std::vector<int> values = {1, 2, 3, 4, 5};
    auto rf = from_values_sequential(values);
    
    auto filtered = filter(rf, [](int x) -> bool {
        if (x == 3) {
            throw std::runtime_error("Predicate error");
        }
        return x % 2 == 0;
    });
    
    // Iterate - should throw when predicate hits 3 during filtering
    EXPECT_THROW({
        for ([[maybe_unused]] auto [value, rank] : filtered) {
            // Will throw when filter evaluates predicate on 3
        }
    }, std::runtime_error);
}

// ============================================================================
// take() - Basic Tests
// ============================================================================

TEST_F(FilterTest, TakeFromEmptySequence) {
    auto rf = empty<int>();
    auto taken = take(rf, 5);
    
    EXPECT_TRUE(taken.is_empty());
}

TEST_F(FilterTest, TakeZeroElements) {
    std::vector<int> values = {1, 2, 3, 4, 5};
    auto rf = from_values_sequential(values);
    auto taken = take(rf, 0);
    
    EXPECT_TRUE(taken.is_empty());
}

TEST_F(FilterTest, TakeLessThanAvailable) {
    std::vector<int> values = {1, 2, 3, 4, 5};
    auto rf = from_values_sequential(values);
    auto taken = take(rf, 3);
    
    auto result = collect_values(taken);
    std::vector<int> expected = {1, 2, 3};
    EXPECT_EQ(result, expected);
}

TEST_F(FilterTest, TakeExactlyAvailable) {
    std::vector<int> values = {1, 2, 3};
    auto rf = from_values_sequential(values);
    auto taken = take(rf, 3);
    
    auto result = collect_values(taken);
    std::vector<int> expected = {1, 2, 3};
    EXPECT_EQ(result, expected);
}

TEST_F(FilterTest, TakeMoreThanAvailable) {
    std::vector<int> values = {1, 2, 3};
    auto rf = from_values_sequential(values);
    auto taken = take(rf, 10);
    
    auto result = collect_values(taken);
    std::vector<int> expected = {1, 2, 3};
    EXPECT_EQ(result, expected);
}

// ============================================================================
// take() - Rank Preservation
// ============================================================================

TEST_F(FilterTest, TakePreservesOriginalRanks) {
    std::vector<int> values = {1, 2, 3, 4, 5};
    auto rf = from_values_sequential(values);  // ranks: 0,1,2,3,4
    auto taken = take(rf, 3);
    
    auto ranks = collect_ranks(taken);
    std::vector<Rank> expected = {
        Rank::from_value(0), Rank::from_value(1), Rank::from_value(2)
    };
    EXPECT_EQ(ranks, expected);
}

TEST_F(FilterTest, TakePreservesNonSequentialRanks) {
    auto rf = from_list<int>({
        {10, Rank::from_value(0)},
        {20, Rank::from_value(2)},
        {30, Rank::from_value(5)},
        {40, Rank::from_value(10)},
        {50, Rank::from_value(20)}
    });
    
    auto taken = take(rf, 3);
    
    auto values = collect_values(taken);
    auto ranks = collect_ranks(taken);
    
    std::vector<int> expected_values = {10, 20, 30};
    std::vector<Rank> expected_ranks = {
        Rank::from_value(0), Rank::from_value(2), Rank::from_value(5)
    };
    
    EXPECT_EQ(values, expected_values);
    EXPECT_EQ(ranks, expected_ranks);
}

// ============================================================================
// take() - Lazy Evaluation
// ============================================================================

TEST_F(FilterTest, TakeIsLazy) {
    int call_count = 0;
    auto rf = from_generator<int>([&call_count](std::size_t i) {
        ++call_count;
        return std::make_pair(static_cast<int>(i), Rank::from_value(i));
    });
    
    // from_generator evaluates first element immediately
    EXPECT_EQ(call_count, 1);
    
    // Creating take should not evaluate more elements
    auto taken = take(rf, 5);
    EXPECT_EQ(call_count, 1);
    
    // Accessing first element shouldn't evaluate more (already done)
    auto it = taken.begin();
    EXPECT_EQ(call_count, 1);
    
    // Moving to second element evaluates it
    ++it;
    EXPECT_EQ(call_count, 2);
}

TEST_F(FilterTest, TakeFromInfiniteSequence) {
    auto rf = from_generator<int>([](std::size_t i) {
        return std::make_pair(static_cast<int>(i), Rank::from_value(i));
    });
    
    auto taken = take(rf, 5);
    
    auto result = collect_values(taken);
    std::vector<int> expected = {0, 1, 2, 3, 4};
    EXPECT_EQ(result, expected);
}

// ============================================================================
// take() - Chaining Operations
// ============================================================================

TEST_F(FilterTest, ChainTakeOperations) {
    std::vector<int> values = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    auto rf = from_values_sequential(values);
    
    // Take 5, then take 3 of those
    auto first_five = take(rf, 5);
    auto first_three = take(first_five, 3);
    
    auto result = collect_values(first_three);
    std::vector<int> expected = {1, 2, 3};
    EXPECT_EQ(result, expected);
}

TEST_F(FilterTest, TakeThenFilter) {
    std::vector<int> values = {1, 2, 3, 4, 5, 6, 7, 8};
    auto rf = from_values_sequential(values);
    
    // Take first 6, then filter evens
    auto first_six = take(rf, 6);
    auto evens = filter(first_six, [](int x) { return x % 2 == 0; });
    
    auto result = collect_values(evens);
    std::vector<int> expected = {2, 4, 6};
    EXPECT_EQ(result, expected);
}

TEST_F(FilterTest, FilterThenTake) {
    std::vector<int> values = {1, 2, 3, 4, 5, 6, 7, 8};
    auto rf = from_values_sequential(values);
    
    // Filter evens, then take first 2
    auto evens = filter(rf, [](int x) { return x % 2 == 0; });
    auto first_two = take(evens, 2);
    
    auto result = collect_values(first_two);
    std::vector<int> expected = {2, 4};
    EXPECT_EQ(result, expected);
}

// ============================================================================
// take_while_rank() - Basic Tests
// ============================================================================

TEST_F(FilterTest, TakeWhileRankFromEmptySequence) {
    auto rf = empty<int>();
    auto taken = take_while_rank(rf, Rank::from_value(5));
    
    EXPECT_TRUE(taken.is_empty());
}

TEST_F(FilterTest, TakeWhileRankBelowAllRanks) {
    std::vector<int> values = {1, 2, 3, 4, 5};
    auto rf = from_values_sequential(values);  // ranks: 0,1,2,3,4
    auto taken = take_while_rank(rf, Rank::from_value(2));
    
    auto result = collect_values(taken);
    std::vector<int> expected = {1, 2, 3};
    EXPECT_EQ(result, expected);
}

TEST_F(FilterTest, TakeWhileRankAboveAllRanks) {
    std::vector<int> values = {1, 2, 3};
    auto rf = from_values_sequential(values);  // ranks: 0,1,2
    auto taken = take_while_rank(rf, Rank::from_value(10));
    
    auto result = collect_values(taken);
    std::vector<int> expected = {1, 2, 3};
    EXPECT_EQ(result, expected);
}

TEST_F(FilterTest, TakeWhileRankExactMatch) {
    std::vector<int> values = {1, 2, 3, 4, 5};
    auto rf = from_values_sequential(values);  // ranks: 0,1,2,3,4
    auto taken = take_while_rank(rf, Rank::from_value(3));
    
    auto result = collect_values(taken);
    std::vector<int> expected = {1, 2, 3, 4};
    EXPECT_EQ(result, expected);
}

TEST_F(FilterTest, TakeWhileRankNonSequential) {
    auto rf = from_list<int>({
        {10, Rank::from_value(0)},
        {20, Rank::from_value(2)},
        {30, Rank::from_value(5)},
        {40, Rank::from_value(10)},
        {50, Rank::from_value(20)}
    });
    
    auto taken = take_while_rank(rf, Rank::from_value(5));
    
    auto values = collect_values(taken);
    auto ranks = collect_ranks(taken);
    
    std::vector<int> expected_values = {10, 20, 30};
    std::vector<Rank> expected_ranks = {
        Rank::from_value(0), Rank::from_value(2), Rank::from_value(5)
    };
    
    EXPECT_EQ(values, expected_values);
    EXPECT_EQ(ranks, expected_ranks);
}

// ============================================================================
// take_while_rank() - Lazy Evaluation
// ============================================================================

TEST_F(FilterTest, TakeWhileRankIsLazy) {
    int call_count = 0;
    auto rf = from_generator<int>([&call_count](std::size_t i) {
        ++call_count;
        return std::make_pair(static_cast<int>(i), Rank::from_value(i));
    });
    
    // from_generator evaluates first element immediately
    EXPECT_EQ(call_count, 1);
    
    // Creating take_while_rank should not evaluate more elements
    auto taken = take_while_rank(rf, Rank::from_value(5));
    EXPECT_EQ(call_count, 1);
    
    // Accessing first element shouldn't evaluate more (already done)
    auto it = taken.begin();
    EXPECT_EQ(call_count, 1);
}

TEST_F(FilterTest, TakeWhileRankFromInfiniteSequence) {
    auto rf = from_generator<int>([](std::size_t i) {
        return std::make_pair(static_cast<int>(i), Rank::from_value(i));
    });
    
    auto taken = take_while_rank(rf, Rank::from_value(4));
    
    auto result = collect_values(taken);
    std::vector<int> expected = {0, 1, 2, 3, 4};
    EXPECT_EQ(result, expected);
}

// ============================================================================
// take_while_rank() - Uniform Ranks
// ============================================================================

TEST_F(FilterTest, TakeWhileRankWithUniformRanks) {
    std::vector<int> values = {1, 2, 3, 4, 5};
    auto rf = from_values_uniform(values);  // all rank 0
    auto taken = take_while_rank(rf, Rank::from_value(0));
    
    auto result = collect_values(taken);
    std::vector<int> expected = {1, 2, 3, 4, 5};
    EXPECT_EQ(result, expected);
}

TEST_F(FilterTest, TakeWhileRankBelowUniformRanks) {
    auto rf = from_list<int>({
        {1, Rank::from_value(5)},
        {2, Rank::from_value(5)},
        {3, Rank::from_value(5)},
        {4, Rank::from_value(5)},
        {5, Rank::from_value(5)}
    });
    
    auto taken = take_while_rank(rf, Rank::from_value(3));
    
    EXPECT_TRUE(taken.is_empty());
}

// ============================================================================
// Chaining All Operations
// ============================================================================

TEST_F(FilterTest, ComplexChainFilterMapTake) {
    std::vector<int> values = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    auto rf = from_values_sequential(values);
    
    // Filter evens, square them, take first 3
    auto evens = filter(rf, [](int x) { return x % 2 == 0; });
    auto squared = map(evens, [](int x) { return x * x; });
    auto first_three = take(squared, 3);
    
    auto result = collect_values(first_three);
    std::vector<int> expected = {4, 16, 36};  // 2^2, 4^2, 6^2
    EXPECT_EQ(result, expected);
}

TEST_F(FilterTest, ComplexChainTakeFilterTakeWhileRank) {
    std::vector<int> values = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    auto rf = from_values_sequential(values);
    
    // Take first 8, filter evens, take rank <= 2
    auto first_eight = take(rf, 8);
    auto evens = filter(first_eight, [](int x) { return x % 2 == 0; });
    auto low_rank = take_while_rank(evens, Rank::from_value(2));
    
    auto result = collect_values(low_rank);
    // From first 8: {1,2,3,4,5,6,7,8}, ranks {0,1,2,3,4,5,6,7}
    // After filter evens: {2,4,6,8}, ranks {1,3,5,7}
    // After rank <= 2: {2}, rank {1}
    std::vector<int> expected = {2};
    EXPECT_EQ(result, expected);
}

// ============================================================================
// Large Sequences
// ============================================================================

TEST_F(FilterTest, FilterLargeSequence) {
    const std::size_t N = 10000;
    
    auto rf = from_generator<int>([](std::size_t i) {
        return std::make_pair(static_cast<int>(i + 1), Rank::from_value(i));
    });
    
    // Filter for multiples of 7
    auto multiples_of_7 = filter(rf, [](int x) { return x % 7 == 0; });
    
    auto values = collect_values(multiples_of_7, N / 7 + 10);
    
    // Should have approximately N/7 elements
    EXPECT_GT(values.size(), 1400);
    EXPECT_LT(values.size(), 1500);
    
    // Verify all are multiples of 7
    for (int val : values) {
        EXPECT_EQ(val % 7, 0);
    }
}

TEST_F(FilterTest, TakeLargeCount) {
    const std::size_t N = 10000;
    
    auto rf = from_generator<int>([](std::size_t i) {
        return std::make_pair(static_cast<int>(i), Rank::from_value(i));
    });
    
    auto taken = take(rf, N);
    
    auto values = collect_values(taken, N + 100);
    EXPECT_EQ(values.size(), N);
}
