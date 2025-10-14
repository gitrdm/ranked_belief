/**
 * @file merge_test.cpp
 * @brief Comprehensive tests for merge operations.
 */

#include "ranked_belief/operations/merge.hpp"
#include "ranked_belief/operations/map.hpp"
#include "ranked_belief/operations/filter.hpp"
#include "ranked_belief/constructors.hpp"
#include <gtest/gtest.h>
#include <string>
#include <vector>

using namespace ranked_belief;

// ============================================================================
// Test Fixture
// ============================================================================

class MergeTest : public ::testing::Test {
protected:
    // Helper to collect values from a ranking function
    template<typename T>
    std::vector<T> collect_values(const RankingFunction<T>& rf, std::size_t max_count = 100) {
        std::vector<T> result;
        auto it = rf.begin();
        auto end = rf.end();
        
        for (std::size_t i = 0; i < max_count && it != end; ++i, ++it) {
            result.push_back((*it).first);
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
            result.push_back((*it).second);
        }
        
        return result;
    }
    
    // Helper to collect value-rank pairs
    template<typename T>
    std::vector<std::pair<T, Rank>> collect_pairs(const RankingFunction<T>& rf, std::size_t max_count = 100) {
        std::vector<std::pair<T, Rank>> result;
        auto it = rf.begin();
        auto end = rf.end();
        
        for (std::size_t i = 0; i < max_count && it != end; ++i, ++it) {
            result.push_back({(*it).first, (*it).second});
        }
        
        return result;
    }
};

// ============================================================================
// merge() - Basic Tests
// ============================================================================

TEST_F(MergeTest, MergeTwoEmptySequences) {
    auto rf1 = empty<int>();
    auto rf2 = empty<int>();
    auto merged = merge(rf1, rf2);
    
    EXPECT_TRUE(merged.is_empty());
}

TEST_F(MergeTest, MergeWithFirstEmpty) {
    auto rf1 = empty<int>();
    auto rf2 = from_list<int>({{1, Rank::zero()}, {2, Rank::from_value(1)}});
    auto merged = merge(rf1, rf2);
    
    auto values = collect_values(merged);
    std::vector<int> expected = {1, 2};
    EXPECT_EQ(values, expected);
}

TEST_F(MergeTest, MergeWithSecondEmpty) {
    auto rf1 = from_list<int>({{1, Rank::zero()}, {2, Rank::from_value(1)}});
    auto rf2 = empty<int>();
    auto merged = merge(rf1, rf2);
    
    auto values = collect_values(merged);
    std::vector<int> expected = {1, 2};
    EXPECT_EQ(values, expected);
}

TEST_F(MergeTest, MergeTwoSingletons) {
    auto rf1 = singleton(10, Rank::from_value(1));
    auto rf2 = singleton(20, Rank::from_value(2));
    auto merged = merge(rf1, rf2);
    
    auto pairs = collect_pairs(merged);
    std::vector<std::pair<int, Rank>> expected = {
        {10, Rank::from_value(1)},
        {20, Rank::from_value(2)}
    };
    EXPECT_EQ(pairs, expected);
}

TEST_F(MergeTest, MergeSingletonsReverseOrder) {
    auto rf1 = singleton(10, Rank::from_value(2));
    auto rf2 = singleton(20, Rank::from_value(1));
    auto merged = merge(rf1, rf2);
    
    auto pairs = collect_pairs(merged);
    std::vector<std::pair<int, Rank>> expected = {
        {20, Rank::from_value(1)},
        {10, Rank::from_value(2)}
    };
    EXPECT_EQ(pairs, expected);
}

// ============================================================================
// merge() - Interleaving
// ============================================================================

TEST_F(MergeTest, MergeInterleavesByRank) {
    auto rf1 = from_list<int>({
        {1, Rank::from_value(0)},
        {3, Rank::from_value(2)},
        {5, Rank::from_value(4)}
    });
    auto rf2 = from_list<int>({
        {2, Rank::from_value(1)},
        {4, Rank::from_value(3)},
        {6, Rank::from_value(5)}
    });
    
    auto merged = merge(rf1, rf2);
    
    auto values = collect_values(merged);
    std::vector<int> expected = {1, 2, 3, 4, 5, 6};
    EXPECT_EQ(values, expected);
    
    auto ranks = collect_ranks(merged);
    std::vector<Rank> expected_ranks = {
        Rank::from_value(0), Rank::from_value(1), Rank::from_value(2),
        Rank::from_value(3), Rank::from_value(4), Rank::from_value(5)
    };
    EXPECT_EQ(ranks, expected_ranks);
}

TEST_F(MergeTest, MergeWithSameRanksFirstTakesPrecedence) {
    auto rf1 = from_list<int>({
        {1, Rank::zero()},
        {3, Rank::from_value(1)}
    });
    auto rf2 = from_list<int>({
        {2, Rank::zero()},
        {4, Rank::from_value(1)}
    });
    
    auto merged = merge(rf1, rf2);
    
    auto values = collect_values(merged);
    // When ranks are equal, first function takes precedence
    std::vector<int> expected = {1, 2, 3, 4};
    EXPECT_EQ(values, expected);
}

TEST_F(MergeTest, MergePreservesOriginalRanks) {
    auto rf1 = from_list<int>({
        {10, Rank::from_value(5)},
        {20, Rank::from_value(10)}
    });
    auto rf2 = from_list<int>({
        {30, Rank::from_value(7)},
        {40, Rank::from_value(15)}
    });
    
    auto merged = merge(rf1, rf2);
    
    auto pairs = collect_pairs(merged);
    std::vector<std::pair<int, Rank>> expected = {
        {10, Rank::from_value(5)},
        {30, Rank::from_value(7)},
        {20, Rank::from_value(10)},
        {40, Rank::from_value(15)}
    };
    EXPECT_EQ(pairs, expected);
}

// ============================================================================
// merge() - Lazy Evaluation
// ============================================================================

TEST_F(MergeTest, MergeIsLazy) {
    int call_count1 = 0;
    int call_count2 = 0;
    
    auto rf1 = from_generator<int>([&call_count1](std::size_t i) {
        ++call_count1;
        return std::make_pair(static_cast<int>(i * 2), Rank::from_value(i * 2));
    });
    
    auto rf2 = from_generator<int>([&call_count2](std::size_t i) {
        ++call_count2;
        return std::make_pair(static_cast<int>(i * 2 + 1), Rank::from_value(i * 2 + 1));
    });
    
    // Generators create first element immediately
    EXPECT_EQ(call_count1, 1);
    EXPECT_EQ(call_count2, 1);
    
    // Creating merge should not evaluate more elements
    auto merged = merge(rf1, rf2);
    EXPECT_EQ(call_count1, 1);
    EXPECT_EQ(call_count2, 1);
    
    // Accessing first element (rank 0 from rf1)
    auto it = merged.begin();
    EXPECT_EQ(call_count1, 1);  // Already had it
    EXPECT_EQ(call_count2, 1);  // Didn't need it yet
}

TEST_F(MergeTest, MergeWithInfiniteSequences) {
    auto rf1 = from_generator<int>([](std::size_t i) {
        return std::make_pair(static_cast<int>(i * 2), Rank::from_value(i * 2));
    });
    
    auto rf2 = from_generator<int>([](std::size_t i) {
        return std::make_pair(static_cast<int>(i * 2 + 1), Rank::from_value(i * 2 + 1));
    });
    
    auto merged = merge(rf1, rf2);
    
    // Take first 10 elements
    auto values = collect_values(merged, 10);
    std::vector<int> expected = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    EXPECT_EQ(values, expected);
}

// ============================================================================
// merge() - Different Sequence Lengths
// ============================================================================

TEST_F(MergeTest, MergeFirstLonger) {
    auto rf1 = from_values_sequential(std::vector<int>{1, 2, 3, 4, 5});
    auto rf2 = from_values_sequential(std::vector<int>{10, 20});
    
    auto merged = merge(rf1, rf2);
    
    auto values = collect_values(merged);
    // rf1 ranks: 0,1,2,3,4; rf2 ranks: 0,1
    // Merged by rank, rf1 at same rank takes precedence
    std::vector<int> expected = {1, 10, 2, 20, 3, 4, 5};
    EXPECT_EQ(values, expected);
}

TEST_F(MergeTest, MergeSecondLonger) {
    auto rf1 = from_values_sequential(std::vector<int>{1, 2});
    auto rf2 = from_values_sequential(std::vector<int>{10, 20, 30, 40, 50});
    
    auto merged = merge(rf1, rf2);
    
    auto values = collect_values(merged);
    std::vector<int> expected = {1, 10, 2, 20, 30, 40, 50};
    EXPECT_EQ(values, expected);
}

// ============================================================================
// merge() - Type Tests
// ============================================================================

TEST_F(MergeTest, MergeStrings) {
    auto rf1 = from_list<std::string>({
        {"apple", Rank::from_value(1)},
        {"cherry", Rank::from_value(3)}
    });
    auto rf2 = from_list<std::string>({
        {"banana", Rank::from_value(2)},
        {"date", Rank::from_value(4)}
    });
    
    auto merged = merge(rf1, rf2);
    
    auto values = collect_values(merged);
    std::vector<std::string> expected = {"apple", "banana", "cherry", "date"};
    EXPECT_EQ(values, expected);
}

// ============================================================================
// merge() - Deduplication
// ============================================================================

TEST_F(MergeTest, MergeWithDeduplication) {
    auto rf1 = from_list<int>({
        {1, Rank::zero()},
        {2, Rank::from_value(1)}
    });
    auto rf2 = from_list<int>({
        {2, Rank::zero()},
        {3, Rank::from_value(1)}
    });
    
    auto merged = merge(rf1, rf2, Deduplication::Enabled);
    
    auto values = collect_values(merged);
    // With deduplication, consecutive 2's should be deduplicated
    std::vector<int> expected = {1, 2, 3};
    EXPECT_EQ(values, expected);
}

TEST_F(MergeTest, MergeWithoutDeduplication) {
    auto rf1 = from_list<int>({
        {1, Rank::zero()},
        {2, Rank::from_value(1)}
    });
    auto rf2 = from_list<int>({
        {2, Rank::zero()},
        {3, Rank::from_value(1)}
    });
    
    auto merged = merge(rf1, rf2, Deduplication::Disabled);
    
    auto values = collect_values(merged);
    // Without deduplication, both 2's should appear
    std::vector<int> expected = {1, 2, 2, 3};
    EXPECT_EQ(values, expected);
}

// ============================================================================
// merge_all() - Basic Tests
// ============================================================================

TEST_F(MergeTest, MergeAllEmpty) {
    std::vector<RankingFunction<int>> rankings;
    auto merged = merge_all(rankings);
    
    EXPECT_TRUE(merged.is_empty());
}

TEST_F(MergeTest, MergeAllSingle) {
    std::vector<int> values = {1, 2, 3};
    auto rf = from_values_sequential(values);
    
    auto merged = merge_all(std::vector<RankingFunction<int>>{rf});
    
    auto result = collect_values(merged);
    std::vector<int> expected = {1, 2, 3};
    EXPECT_EQ(result, expected);
}

TEST_F(MergeTest, MergeAllThreeSequences) {
    auto rf1 = from_list<int>({
        {1, Rank::zero()},
        {4, Rank::from_value(3)}
    });
    auto rf2 = from_list<int>({
        {2, Rank::from_value(1)}
    });
    auto rf3 = from_list<int>({
        {3, Rank::from_value(2)},
        {5, Rank::from_value(4)}
    });
    
    auto merged = merge_all(std::vector<RankingFunction<int>>{rf1, rf2, rf3});
    
    auto values = collect_values(merged);
    std::vector<int> expected = {1, 2, 3, 4, 5};
    EXPECT_EQ(values, expected);
}

TEST_F(MergeTest, MergeAllPreservesRankOrder) {
    auto rf1 = from_list<int>({{10, Rank::from_value(5)}});
    auto rf2 = from_list<int>({{20, Rank::from_value(2)}});
    auto rf3 = from_list<int>({{30, Rank::from_value(8)}});
    auto rf4 = from_list<int>({{40, Rank::from_value(3)}});
    
    auto merged = merge_all(std::vector<RankingFunction<int>>{rf1, rf2, rf3, rf4});
    
    auto values = collect_values(merged);
    std::vector<int> expected = {20, 40, 10, 30};  // Sorted by rank: 2,3,5,8
    EXPECT_EQ(values, expected);
}

TEST_F(MergeTest, MergeAllManySequences) {
    std::vector<RankingFunction<int>> rankings;
    
    // Create 10 ranking functions, each with values at specific ranks
    for (int i = 0; i < 10; ++i) {
        auto rf = from_list<int>({
            {i * 10, Rank::from_value(i)},
            {i * 10 + 1, Rank::from_value(10 + i)}
        });
        rankings.push_back(rf);
    }
    
    auto merged = merge_all(rankings);
    
    // Should have 20 elements (2 per sequence)
    auto values = collect_values(merged, 30);
    EXPECT_EQ(values.size(), 20);
    
    // First 10 should be the first elements (ranks 0-9)
    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(values[i], i * 10);
    }
}

// ============================================================================
// Chaining with Other Operations
// ============================================================================

TEST_F(MergeTest, MergeThenFilter) {
    auto rf1 = from_values_sequential(std::vector<int>{1, 2, 3});
    auto rf2 = from_values_sequential(std::vector<int>{4, 5, 6});
    
    auto merged = merge(rf1, rf2);
    auto evens = filter(merged, [](int x) { return x % 2 == 0; });
    
    auto values = collect_values(evens);
    // rf1 ranks: 0,1,2; rf2 ranks: 0,1,2
    // Merged (rf1 takes precedence at same rank): 1@0, 4@0, 2@1, 5@1, 3@2, 6@2
    // Filtered evens: 4@0, 2@1, 6@2
    std::vector<int> expected = {4, 2, 6};
    EXPECT_EQ(values, expected);
}

TEST_F(MergeTest, MergeThenMap) {
    auto rf1 = from_list<int>({{1, Rank::zero()}, {3, Rank::from_value(2)}});
    auto rf2 = from_list<int>({{2, Rank::from_value(1)}, {4, Rank::from_value(3)}});
    
    auto merged = merge(rf1, rf2);
    auto doubled = map(merged, [](int x) { return x * 2; });
    
    auto values = collect_values(doubled);
    std::vector<int> expected = {2, 4, 6, 8};
    EXPECT_EQ(values, expected);
}

TEST_F(MergeTest, FilterThenMerge) {
    std::vector<int> values1 = {1, 2, 3, 4, 5};
    std::vector<int> values2 = {6, 7, 8, 9, 10};
    
    auto rf1 = from_values_sequential(values1);
    auto rf2 = from_values_sequential(values2);
    
    auto evens1 = filter(rf1, [](int x) { return x % 2 == 0; });
    auto evens2 = filter(rf2, [](int x) { return x % 2 == 0; });
    
    auto merged = merge(evens1, evens2);
    
    auto result = collect_values(merged);
    // rf1: 1@0, 2@1, 3@2, 4@3, 5@4
    // rf2: 6@0, 7@1, 8@2, 9@3, 10@4
    // evens1: 2@1, 4@3
    // evens2: 6@0, 8@2, 10@4
    // Merged by rank: 6@0, 2@1, 8@2, 4@3, 10@4
    std::vector<int> expected = {6, 2, 8, 4, 10};
    EXPECT_EQ(result, expected);
}

TEST_F(MergeTest, MapThenMerge) {
    auto rf1 = from_values_sequential(std::vector<int>{1, 2});
    auto rf2 = from_values_sequential(std::vector<int>{3, 4});
    
    auto squared1 = map(rf1, [](int x) { return x * x; });
    auto squared2 = map(rf2, [](int x) { return x * x; });
    
    auto merged = merge(squared1, squared2);
    
    auto values = collect_values(merged);
    std::vector<int> expected = {1, 9, 4, 16};  // 1@0, 9@0, 4@1, 16@1
    EXPECT_EQ(values, expected);
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(MergeTest, MergeSameSequenceTwice) {
    auto rf = from_values_sequential(std::vector<int>{1, 2, 3});
    auto merged = merge(rf, rf);
    
    // When merging a sequence with itself (same shared_ptr), both arguments
    // point to the same data structure. As the merge traverses, it consumes
    // elements from both sides simultaneously, resulting in the original sequence.
    // To actually duplicate, create two separate instances.
    auto values = collect_values(merged);
    std::vector<int> expected = {1, 2, 3};
    EXPECT_EQ(values, expected);
}

TEST_F(MergeTest, MergeTwoIdenticalSequences) {
    // Create two separate instances with same values and ranks
    std::vector<int> values = {1, 2, 3};
    auto rf1 = from_values_sequential(values);
    auto rf2 = from_values_sequential(values);
    
    // With deduplicate=true (default), identical values at same ranks are deduplicated
    auto merged_dedup = merge(rf1, rf2);
    auto result_dedup = collect_values(merged_dedup);
    std::vector<int> expected_dedup = {1, 2, 3};
    EXPECT_EQ(result_dedup, expected_dedup);
    
    // With deduplicate=false, we get all values from both sequences
    auto merged_no_dedup = merge(rf1, rf2, Deduplication::Disabled);
    auto result_no_dedup = collect_values(merged_no_dedup);
    std::vector<int> expected_no_dedup = {1, 1, 2, 2, 3, 3};
    EXPECT_EQ(result_no_dedup, expected_no_dedup);
}

TEST_F(MergeTest, MergeWithUniformRanks) {
    std::vector<int> values1 = {1, 2, 3};
    std::vector<int> values2 = {4, 5, 6};
    
    auto rf1 = from_values_uniform(values1);  // All rank 0
    auto rf2 = from_values_uniform(values2);  // All rank 0
    
    auto merged = merge(rf1, rf2);
    
    auto values = collect_values(merged);
    // All at rank 0, first takes precedence
    std::vector<int> expected = {1, 2, 3, 4, 5, 6};
    EXPECT_EQ(values, expected);
}

TEST_F(MergeTest, MergeDisjointRanks) {
    auto rf1 = from_list<int>({
        {1, Rank::from_value(0)},
        {2, Rank::from_value(1)},
        {3, Rank::from_value(2)}
    });
    auto rf2 = from_list<int>({
        {10, Rank::from_value(10)},
        {20, Rank::from_value(20)},
        {30, Rank::from_value(30)}
    });
    
    auto merged = merge(rf1, rf2);
    
    auto values = collect_values(merged);
    std::vector<int> expected = {1, 2, 3, 10, 20, 30};
    EXPECT_EQ(values, expected);
}

// ============================================================================
// Performance Tests
// ============================================================================

TEST_F(MergeTest, MergeLargeSequences) {
    const std::size_t N = 5000;
    
    auto rf1 = from_generator<int>([](std::size_t i) {
        return std::make_pair(static_cast<int>(i * 2), Rank::from_value(i * 2));
    });
    
    auto rf2 = from_generator<int>([](std::size_t i) {
        return std::make_pair(static_cast<int>(i * 2 + 1), Rank::from_value(i * 2 + 1));
    });
    
    auto merged = merge(rf1, rf2);
    
    auto values = collect_values(merged, N);
    EXPECT_EQ(values.size(), N);
    
    // Verify proper interleaving
    for (std::size_t i = 0; i < 100; ++i) {
        EXPECT_EQ(values[i], static_cast<int>(i));
    }
}

TEST_F(MergeTest, MergeAllManySmallSequences) {
    std::vector<RankingFunction<int>> rankings;
    
    // Create 100 singleton ranking functions
    for (int i = 0; i < 100; ++i) {
        rankings.push_back(singleton(i, Rank::from_value(i)));
    }
    
    auto merged = merge_all(rankings);
    
    auto values = collect_values(merged, 150);
    EXPECT_EQ(values.size(), 100);
    
    // Should be in order
    for (int i = 0; i < 100; ++i) {
        EXPECT_EQ(values[i], i);
    }
}
