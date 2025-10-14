/**
 * @file ranking_function_test.cpp
 * @brief Comprehensive tests for RankingFunction<T>.
 *
 * Tests cover:
 * - Construction (empty, from head, factory functions)
 * - Range interface (begin/end, iteration)
 * - Query operations (first, is_empty, size)
 * - Deduplication behavior
 * - Complex types
 * - C++20 ranges compatibility
 * - Edge cases
 */

#include "ranked_belief/ranking_function.hpp"
#include <gtest/gtest.h>
#include <algorithm>
#include <ranges>
#include <string>
#include <vector>

using namespace ranked_belief;

/// Test fixture for RankingFunction tests
class RankingFunctionTest : public ::testing::Test {
protected:
    /// Helper to create a simple 3-element sequence: [1@0, 2@1, 3@2]
    std::shared_ptr<RankingElement<int>> create_simple_sequence() {
        return make_lazy_element(1, Rank::zero(), []() {
            return make_lazy_element(2, Rank::from_value(1), []() {
                return make_terminal(3, Rank::from_value(2));
            });
        });
    }

    /// Helper to create a sequence with duplicates: [1@0, 1@1, 2@2]
    std::shared_ptr<RankingElement<int>> create_duplicate_sequence() {
        return make_lazy_element(1, Rank::zero(), []() {
            return make_lazy_element(1, Rank::from_value(1), []() {
                return make_terminal(2, Rank::from_value(2));
            });
        });
    }
};

// ============================================================================
// Construction Tests
// ============================================================================

TEST_F(RankingFunctionTest, DefaultConstructorCreatesEmpty) {
    RankingFunction<int> rf;
    
    EXPECT_TRUE(rf.is_empty());
    EXPECT_EQ(rf.head(), nullptr);
    EXPECT_EQ(rf.first(), std::nullopt);
}

TEST_F(RankingFunctionTest, ConstructFromNullptrCreatesEmpty) {
    RankingFunction<int> rf(nullptr, Deduplication::Enabled);
    
    EXPECT_TRUE(rf.is_empty());
    EXPECT_EQ(rf.head(), nullptr);
}

TEST_F(RankingFunctionTest, ConstructFromHeadElement) {
    auto head = make_terminal(42, Rank::from_value(5));
    RankingFunction<int> rf(head);
    
    EXPECT_FALSE(rf.is_empty());
    EXPECT_EQ(rf.head(), head);
}

TEST_F(RankingFunctionTest, ConstructorPreservesDeduplicationFlag) {
    auto head = create_simple_sequence();
    
    RankingFunction<int> rf_dedup(head, Deduplication::Enabled);
    EXPECT_TRUE(rf_dedup.is_deduplicating());
    
    RankingFunction<int> rf_no_dedup(head, Deduplication::Disabled);
    EXPECT_FALSE(rf_no_dedup.is_deduplicating());
}

TEST_F(RankingFunctionTest, MakeEmptyRankingFactory) {
    auto rf = make_empty_ranking<int>();
    
    EXPECT_TRUE(rf.is_empty());
    EXPECT_EQ(rf.first(), std::nullopt);
}

TEST_F(RankingFunctionTest, MakeRankingFunctionFactory) {
    auto head = make_terminal(99, Rank::zero());
    auto rf = make_ranking_function(head);
    
    EXPECT_FALSE(rf.is_empty());
    EXPECT_EQ(rf.head(), head);
}

TEST_F(RankingFunctionTest, MakeSingletonRankingFactory) {
    auto rf = make_singleton_ranking(42, Rank::from_value(7));
    
    EXPECT_FALSE(rf.is_empty());
    auto first = rf.first();
    ASSERT_TRUE(first.has_value());
    EXPECT_EQ(first->first, 42);
    EXPECT_EQ(first->second, Rank::from_value(7));
}

TEST_F(RankingFunctionTest, MakeSingletonWithDefaultRank) {
    auto rf = make_singleton_ranking(100);
    
    auto first = rf.first();
    ASSERT_TRUE(first.has_value());
    EXPECT_EQ(first->first, 100);
    EXPECT_EQ(first->second, Rank::zero());
}

// ============================================================================
// Range Interface Tests
// ============================================================================

TEST_F(RankingFunctionTest, BeginEndForEmptySequence) {
    RankingFunction<int> rf;
    
    EXPECT_EQ(rf.begin(), rf.end());
}

TEST_F(RankingFunctionTest, BeginReturnsIteratorToFirst) {
    auto rf = make_singleton_ranking(42, Rank::from_value(5));
    auto it = rf.begin();
    
    EXPECT_NE(it, rf.end());
    auto [value, rank] = *it;
    EXPECT_EQ(value, 42);
    EXPECT_EQ(rank, Rank::from_value(5));
}

TEST_F(RankingFunctionTest, MultipleBeginCallsReturnIndependentIterators) {
    auto head = create_simple_sequence();
    RankingFunction<int> rf(head);
    
    auto it1 = rf.begin();
    auto it2 = rf.begin();
    
    EXPECT_EQ(it1, it2);  // Initially equal
    ++it1;
    EXPECT_NE(it1, it2);  // After advance, independent
}

TEST_F(RankingFunctionTest, RangeBasedForLoopIteration) {
    auto head = create_simple_sequence();
    RankingFunction<int> rf(head, Deduplication::Disabled);
    
    std::vector<int> values;
    for (auto [value, rank] : rf) {
        values.push_back(value);
    }
    
    EXPECT_EQ(values, (std::vector<int>{1, 2, 3}));
}

TEST_F(RankingFunctionTest, RangeBasedForLoopWithDeduplication) {
    auto head = create_duplicate_sequence();
    RankingFunction<int> rf(head, Deduplication::Enabled);
    
    std::vector<int> values;
    for (auto [value, rank] : rf) {
        values.push_back(value);
    }
    
    EXPECT_EQ(values, (std::vector<int>{1, 2}));
}

TEST_F(RankingFunctionTest, RangeBasedForLoopWithoutDeduplication) {
    auto head = create_duplicate_sequence();
    RankingFunction<int> rf(head, Deduplication::Disabled);
    
    std::vector<int> values;
    for (auto [value, rank] : rf) {
        values.push_back(value);
    }
    
    EXPECT_EQ(values, (std::vector<int>{1, 1, 2}));
}

// ============================================================================
// Query Operation Tests
// ============================================================================

TEST_F(RankingFunctionTest, FirstOnEmptyReturnsNullopt) {
    RankingFunction<int> rf;
    
    EXPECT_EQ(rf.first(), std::nullopt);
}

TEST_F(RankingFunctionTest, FirstReturnsPairOfValueAndRank) {
    auto rf = make_singleton_ranking(42, Rank::from_value(7));
    auto first = rf.first();
    
    ASSERT_TRUE(first.has_value());
    EXPECT_EQ(first->first, 42);
    EXPECT_EQ(first->second, Rank::from_value(7));
}

TEST_F(RankingFunctionTest, FirstWithMultipleElements) {
    auto head = create_simple_sequence();
    RankingFunction<int> rf(head);
    
    auto first = rf.first();
    ASSERT_TRUE(first.has_value());
    EXPECT_EQ(first->first, 1);
    EXPECT_EQ(first->second, Rank::zero());
}

TEST_F(RankingFunctionTest, IsEmptyTrueForDefault) {
    RankingFunction<int> rf;
    EXPECT_TRUE(rf.is_empty());
}

TEST_F(RankingFunctionTest, IsEmptyFalseForNonEmpty) {
    auto rf = make_singleton_ranking(1);
    EXPECT_FALSE(rf.is_empty());
}

TEST_F(RankingFunctionTest, SizeOfEmptyIsZero) {
    RankingFunction<int> rf;
    EXPECT_EQ(rf.size(), 0);
}

TEST_F(RankingFunctionTest, SizeOfSingletonIsOne) {
    auto rf = make_singleton_ranking(42);
    EXPECT_EQ(rf.size(), 1);
}

TEST_F(RankingFunctionTest, SizeCountsAllElements) {
    auto head = create_simple_sequence();
    RankingFunction<int> rf(head, Deduplication::Disabled);
    
    EXPECT_EQ(rf.size(), 3);
}

TEST_F(RankingFunctionTest, SizeRespectsDeduplication) {
    auto head = create_duplicate_sequence();
    
    RankingFunction<int> rf_dedup(head, Deduplication::Enabled);
    EXPECT_EQ(rf_dedup.size(), 2);  // [1, 2] after dedup
    
    RankingFunction<int> rf_no_dedup(head, Deduplication::Disabled);
    EXPECT_EQ(rf_no_dedup.size(), 3);  // [1, 1, 2] without dedup
}

// ============================================================================
// Comparison Tests
// ============================================================================

TEST_F(RankingFunctionTest, EqualityWithSameHead) {
    auto head = create_simple_sequence();
    RankingFunction<int> rf1(head);
    RankingFunction<int> rf2(head);
    
    EXPECT_TRUE(rf1 == rf2);
    EXPECT_FALSE(rf1 != rf2);
}

TEST_F(RankingFunctionTest, InequalityWithDifferentHeads) {
    auto head1 = make_terminal(1, Rank::zero());
    auto head2 = make_terminal(2, Rank::zero());
    
    RankingFunction<int> rf1(head1);
    RankingFunction<int> rf2(head2);
    
    EXPECT_FALSE(rf1 == rf2);
    EXPECT_TRUE(rf1 != rf2);
}

TEST_F(RankingFunctionTest, InequalityWithDifferentDeduplicationFlags) {
    auto head = create_simple_sequence();
    RankingFunction<int> rf1(head, Deduplication::Enabled);
    RankingFunction<int> rf2(head, Deduplication::Disabled);
    
    EXPECT_FALSE(rf1 == rf2);
    EXPECT_TRUE(rf1 != rf2);
}

TEST_F(RankingFunctionTest, EmptyRankingFunctionsAreEqual) {
    RankingFunction<int> rf1;
    RankingFunction<int> rf2;
    
    EXPECT_TRUE(rf1 == rf2);
}

// ============================================================================
// Complex Type Tests
// ============================================================================

TEST_F(RankingFunctionTest, RankingFunctionOfStrings) {
    auto head = make_lazy_element(std::string("alpha"), Rank::zero(), []() {
        return make_terminal(std::string("beta"), Rank::from_value(1));
    });
    
    RankingFunction<std::string> rf(head);
    
    std::vector<std::string> values;
    for (auto [value, rank] : rf) {
        values.push_back(value);
    }
    
    EXPECT_EQ(values, (std::vector<std::string>{"alpha", "beta"}));
}

TEST_F(RankingFunctionTest, RankingFunctionOfVectors) {
    auto head = make_terminal(std::vector<int>{1, 2, 3}, Rank::zero());
    RankingFunction<std::vector<int>> rf(head);
    
    auto first = rf.first();
    ASSERT_TRUE(first.has_value());
    EXPECT_EQ(first->first, (std::vector<int>{1, 2, 3}));
}

TEST_F(RankingFunctionTest, RankingFunctionOfPairs) {
    using Pair = std::pair<int, std::string>;
    auto head = make_terminal(Pair{1, "one"}, Rank::zero());
    RankingFunction<Pair> rf(head);
    
    auto first = rf.first();
    ASSERT_TRUE(first.has_value());
    EXPECT_EQ(first->first.first, 1);
    EXPECT_EQ(first->first.second, "one");
}

// ============================================================================
// C++20 Ranges Tests
// ============================================================================

TEST_F(RankingFunctionTest, WorksWithStdRangesDistance) {
    auto head = create_simple_sequence();
    RankingFunction<int> rf(head, Deduplication::Disabled);
    
    auto dist = std::ranges::distance(rf);
    EXPECT_EQ(dist, 3);
}

TEST_F(RankingFunctionTest, WorksWithStdRangesFind) {
    auto head = create_simple_sequence();
    RankingFunction<int> rf(head, Deduplication::Disabled);
    
    auto it = std::ranges::find_if(rf, [](const auto& pair) {
        return pair.first == 2;
    });
    
    EXPECT_NE(it, rf.end());
    EXPECT_EQ((*it).first, 2);
}

TEST_F(RankingFunctionTest, WorksWithStdRangesTransform) {
    auto head = create_simple_sequence();
    RankingFunction<int> rf(head, Deduplication::Disabled);
    
    std::vector<int> values;
    std::ranges::transform(rf, std::back_inserter(values),
                          [](const auto& pair) { return pair.first; });
    
    EXPECT_EQ(values, (std::vector<int>{1, 2, 3}));
}

TEST_F(RankingFunctionTest, WorksWithStdRangesCount) {
    auto head = create_duplicate_sequence();
    RankingFunction<int> rf(head, Deduplication::Disabled);
    
    auto count = std::ranges::count_if(rf, [](const auto& pair) {
        return pair.first == 1;
    });
    
    EXPECT_EQ(count, 2);
}

TEST_F(RankingFunctionTest, WorksWithStdRangesAnyOf) {
    auto head = create_simple_sequence();
    RankingFunction<int> rf(head, Deduplication::Disabled);
    
    bool has_two = std::ranges::any_of(rf, [](const auto& pair) {
        return pair.first == 2;
    });
    
    EXPECT_TRUE(has_two);
}

// ============================================================================
// Edge Case Tests
// ============================================================================

TEST_F(RankingFunctionTest, SingleElementSequence) {
    auto rf = make_singleton_ranking(99, Rank::from_value(3));
    
    EXPECT_FALSE(rf.is_empty());
    EXPECT_EQ(rf.size(), 1);
    
    auto first = rf.first();
    ASSERT_TRUE(first.has_value());
    EXPECT_EQ(first->first, 99);
    EXPECT_EQ(first->second, Rank::from_value(3));
}

TEST_F(RankingFunctionTest, LongSequenceSize) {
    // Create a sequence of 50 elements
    std::function<std::shared_ptr<RankingElement<int>>(int)> make_seq;
    make_seq = [&make_seq](int n) -> std::shared_ptr<RankingElement<int>> {
        if (n == 0) return nullptr;
        if (n == 1) return make_terminal(n, Rank::from_value(n - 1));
        return make_lazy_element(n, Rank::from_value(n - 1), [n, &make_seq]() {
            return make_seq(n - 1);
        });
    };
    
    auto head = make_seq(50);
    RankingFunction<int> rf(head, Deduplication::Disabled);
    
    EXPECT_EQ(rf.size(), 50);
}

TEST_F(RankingFunctionTest, IteratorIndependenceAcrossRankingFunctions) {
    auto head = create_simple_sequence();
    RankingFunction<int> rf1(head);
    RankingFunction<int> rf2(head);
    
    auto it1 = rf1.begin();
    auto it2 = rf2.begin();
    
    ++it1;
    EXPECT_EQ((*it1).first, 2);
    EXPECT_EQ((*it2).first, 1);  // it2 not affected
}

TEST_F(RankingFunctionTest, MultipleCallsToFirstReturnSameValue) {
    auto rf = make_singleton_ranking(42, Rank::from_value(7));
    
    auto first1 = rf.first();
    auto first2 = rf.first();
    
    ASSERT_TRUE(first1.has_value());
    ASSERT_TRUE(first2.has_value());
    EXPECT_EQ(first1->first, first2->first);
    EXPECT_EQ(first1->second, first2->second);
}

TEST_F(RankingFunctionTest, HeadAccessorReturnsCorrectPointer) {
    auto head = make_terminal(42, Rank::zero());
    RankingFunction<int> rf(head);
    
    EXPECT_EQ(rf.head(), head);
}

TEST_F(RankingFunctionTest, ConstRankingFunctionIteration) {
    auto head = create_simple_sequence();
    const RankingFunction<int> rf(head, Deduplication::Disabled);
    
    std::vector<int> values;
    for (auto [value, rank] : rf) {
        values.push_back(value);
    }
    
    EXPECT_EQ(values, (std::vector<int>{1, 2, 3}));
}

TEST_F(RankingFunctionTest, CopyConstructorSharesHead) {
    auto head = create_simple_sequence();
    RankingFunction<int> rf1(head);
    RankingFunction<int> rf2 = rf1;
    
    EXPECT_EQ(rf1.head(), rf2.head());
    EXPECT_TRUE(rf1 == rf2);
}

TEST_F(RankingFunctionTest, AssignmentSharesHead) {
    auto head1 = make_terminal(1, Rank::zero());
    auto head2 = make_terminal(2, Rank::zero());
    
    RankingFunction<int> rf1(head1);
    RankingFunction<int> rf2(head2);
    
    rf2 = rf1;
    
    EXPECT_EQ(rf1.head(), rf2.head());
    EXPECT_TRUE(rf1 == rf2);
}

TEST_F(RankingFunctionTest, AllDuplicatesWithDeduplication) {
    // Sequence: [5@0, 5@1, 5@2]
    auto head = make_lazy_element(5, Rank::zero(), []() {
        return make_lazy_element(5, Rank::from_value(1), []() {
            return make_terminal(5, Rank::from_value(2));
        });
    });
    
    RankingFunction<int> rf(head, Deduplication::Enabled);
    
    EXPECT_EQ(rf.size(), 1);
    auto first = rf.first();
    ASSERT_TRUE(first.has_value());
    EXPECT_EQ(first->first, 5);
}

TEST_F(RankingFunctionTest, ZeroRankElements) {
    auto head = make_lazy_element(1, Rank::zero(), []() {
        return make_lazy_element(2, Rank::zero(), []() {
            return make_terminal(3, Rank::zero());
        });
    });
    
    RankingFunction<int> rf(head, Deduplication::Disabled);
    
    for (auto [value, rank] : rf) {
        EXPECT_EQ(rank, Rank::zero());
    }
}

TEST_F(RankingFunctionTest, InfinityRankElement) {
    auto rf = make_singleton_ranking(999, Rank::infinity());
    
    auto first = rf.first();
    ASSERT_TRUE(first.has_value());
    EXPECT_EQ(first->second, Rank::infinity());
}
