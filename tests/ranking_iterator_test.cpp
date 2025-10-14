/**
 * @file ranking_iterator_test.cpp
 * @brief Comprehensive tests for RankingIterator<T>.
 *
 * Tests cover:
 * - Iterator construction (begin/end sentinels)
 * - Iterator operations (dereference, increment, comparison)
 * - Range-based for loop iteration
 * - Deduplication on/off behavior
 * - Edge cases (empty, single element, all duplicates)
 * - C++20 ranges compatibility
 * - Complex value types
 */

#include "ranked_belief/ranking_iterator.hpp"
#include "ranked_belief/ranking_element.hpp"
#include <gtest/gtest.h>
#include <algorithm>
#include <ranges>
#include <string>
#include <vector>

using namespace ranked_belief;

/// Test fixture for RankingIterator tests
class RankingIteratorTest : public ::testing::Test {
protected:
    /// Helper to create a simple 3-element sequence: [1@0, 2@1, 3@2]
    std::shared_ptr<RankingElement<int>> create_simple_sequence() {
        return make_lazy_element(1, Rank::zero(), []() {
            return make_lazy_element(2, Rank::from_value(1), []() {
                return make_terminal(3, Rank::from_value(2));
            });
        });
    }

    /// Helper to create a sequence with duplicates: [1@0, 1@1, 2@2, 2@3, 3@4]
    std::shared_ptr<RankingElement<int>> create_duplicate_sequence() {
        return make_lazy_element(1, Rank::zero(), []() {
            return make_lazy_element(1, Rank::from_value(1), []() {
                return make_lazy_element(2, Rank::from_value(2), []() {
                    return make_lazy_element(2, Rank::from_value(3), []() {
                        return make_terminal(3, Rank::from_value(4));
                    });
                });
            });
        });
    }

    /// Helper to create a sequence of all duplicates: [5@0, 5@1, 5@2]
    std::shared_ptr<RankingElement<int>> create_all_duplicates() {
        return make_lazy_element(5, Rank::zero(), []() {
            return make_lazy_element(5, Rank::from_value(1), []() {
                return make_terminal(5, Rank::from_value(2));
            });
        });
    }
};

// ============================================================================
// Construction Tests
// ============================================================================

TEST_F(RankingIteratorTest, DefaultConstructorCreatesEndSentinel) {
    RankingIterator<int> it;
    EXPECT_TRUE(it.is_end());
    EXPECT_EQ(it.current(), nullptr);
}

TEST_F(RankingIteratorTest, ConstructWithNullptrCreatesEndSentinel) {
    RankingIterator<int> it(nullptr, Deduplication::Disabled);
    EXPECT_TRUE(it.is_end());
    EXPECT_EQ(it.current(), nullptr);
}

TEST_F(RankingIteratorTest, ConstructWithElementCreatesValidIterator) {
    auto seq = create_simple_sequence();
    RankingIterator<int> it(seq, Deduplication::Disabled);
    
    EXPECT_FALSE(it.is_end());
    EXPECT_EQ(it.current(), seq);
}

TEST_F(RankingIteratorTest, ConstructorPreservesDeduplicationFlag) {
    auto seq = create_simple_sequence();
    
    RankingIterator<int> it_dedup(seq, Deduplication::Enabled);
    EXPECT_TRUE(it_dedup.is_deduplicating());
    
    RankingIterator<int> it_no_dedup(seq, Deduplication::Disabled);
    EXPECT_FALSE(it_no_dedup.is_deduplicating());
}

// ============================================================================
// Dereference Tests
// ============================================================================

TEST_F(RankingIteratorTest, DereferenceReturnsValueRankPair) {
    auto seq = make_terminal(42, Rank::from_value(7));
    RankingIterator<int> it(seq, Deduplication::Disabled);
    
    auto [value, rank] = *it;
    EXPECT_EQ(value, 42);
    EXPECT_EQ(rank, Rank::from_value(7));
}

TEST_F(RankingIteratorTest, DereferenceWithZeroRank) {
    auto seq = make_terminal(100, Rank::zero());
    RankingIterator<int> it(seq, Deduplication::Disabled);
    
    auto [value, rank] = *it;
    EXPECT_EQ(value, 100);
    EXPECT_EQ(rank, Rank::zero());
}

TEST_F(RankingIteratorTest, DereferenceWithString) {
    auto seq = make_terminal(std::string("hello"), Rank::from_value(3));
    RankingIterator<std::string> it(seq, Deduplication::Disabled);
    
    auto [value, rank] = *it;
    EXPECT_EQ(value, "hello");
    EXPECT_EQ(rank, Rank::from_value(3));
}

// ============================================================================
// Increment Tests
// ============================================================================

TEST_F(RankingIteratorTest, PreIncrementAdvancesToNext) {
    auto seq = create_simple_sequence();
    RankingIterator<int> it(seq, Deduplication::Disabled);
    
    EXPECT_EQ((*it).first, 1);
    ++it;
    EXPECT_EQ((*it).first, 2);
    ++it;
    EXPECT_EQ((*it).first, 3);
}

TEST_F(RankingIteratorTest, PreIncrementReturnsReference) {
    auto seq = create_simple_sequence();
    RankingIterator<int> it(seq, Deduplication::Disabled);
    
    auto& result = ++it;
    EXPECT_EQ(&result, &it);  // Same object
}

TEST_F(RankingIteratorTest, PostIncrementAdvancesToNext) {
    auto seq = create_simple_sequence();
    RankingIterator<int> it(seq, Deduplication::Disabled);
    
    EXPECT_EQ((*it).first, 1);
    it++;
    EXPECT_EQ((*it).first, 2);
    it++;
    EXPECT_EQ((*it).first, 3);
}

TEST_F(RankingIteratorTest, PostIncrementReturnsPreviousIterator) {
    auto seq = create_simple_sequence();
    RankingIterator<int> it(seq, Deduplication::Disabled);
    
    auto prev = it++;
    EXPECT_EQ((*prev).first, 1);
    EXPECT_EQ((*it).first, 2);
}

TEST_F(RankingIteratorTest, IncrementPastEndReachesNull) {
    auto seq = make_terminal(1, Rank::zero());
    RankingIterator<int> it(seq, Deduplication::Disabled);
    
    EXPECT_FALSE(it.is_end());
    ++it;
    EXPECT_TRUE(it.is_end());
}

// ============================================================================
// Comparison Tests
// ============================================================================

TEST_F(RankingIteratorTest, EqualityComparison) {
    auto seq = create_simple_sequence();
    RankingIterator<int> it1(seq, Deduplication::Disabled);
    RankingIterator<int> it2(seq, Deduplication::Disabled);
    
    EXPECT_TRUE(it1 == it2);
    EXPECT_FALSE(it1 != it2);
}

TEST_F(RankingIteratorTest, InequalityAfterIncrement) {
    auto seq = create_simple_sequence();
    RankingIterator<int> it1(seq, Deduplication::Disabled);
    RankingIterator<int> it2(seq, Deduplication::Disabled);
    
    ++it1;
    EXPECT_FALSE(it1 == it2);
    EXPECT_TRUE(it1 != it2);
}

TEST_F(RankingIteratorTest, EndSentinelComparison) {
    RankingIterator<int> end1;
    RankingIterator<int> end2;
    
    EXPECT_TRUE(end1 == end2);
    EXPECT_FALSE(end1 != end2);
}

TEST_F(RankingIteratorTest, IteratorNotEqualToEnd) {
    auto seq = create_simple_sequence();
    RankingIterator<int> it(seq, Deduplication::Disabled);
    RankingIterator<int> end;
    
    EXPECT_FALSE(it == end);
    EXPECT_TRUE(it != end);
}

// ============================================================================
// Iteration Tests
// ============================================================================

TEST_F(RankingIteratorTest, IterateOverFiniteSequence) {
    auto seq = create_simple_sequence();
    RankingIterator<int> it(seq, Deduplication::Disabled);
    RankingIterator<int> end;
    
    std::vector<int> values;
    for (; it != end; ++it) {
        values.push_back((*it).first);
    }
    
    EXPECT_EQ(values, (std::vector<int>{1, 2, 3}));
}

TEST_F(RankingIteratorTest, IterateWithRanksPreserved) {
    auto seq = create_simple_sequence();
    RankingIterator<int> it(seq, Deduplication::Disabled);
    RankingIterator<int> end;
    
    std::vector<std::pair<int, uint64_t>> pairs;
    for (; it != end; ++it) {
        auto [value, rank] = *it;
        pairs.push_back({value, rank.value()});
    }
    
    std::vector<std::pair<int, uint64_t>> expected = {
        {1, 0}, {2, 1}, {3, 2}
    };
    EXPECT_EQ(pairs, expected);
}

TEST_F(RankingIteratorTest, SingleElementIteration) {
    auto seq = make_terminal(99, Rank::from_value(5));
    RankingIterator<int> it(seq, Deduplication::Disabled);
    RankingIterator<int> end;
    
    std::vector<int> values;
    for (; it != end; ++it) {
        values.push_back((*it).first);
    }
    
    EXPECT_EQ(values, (std::vector<int>{99}));
}

TEST_F(RankingIteratorTest, EmptySequenceIteration) {
    RankingIterator<int> it;  // End sentinel
    RankingIterator<int> end;
    
    std::vector<int> values;
    for (; it != end; ++it) {
        values.push_back((*it).first);
    }
    
    EXPECT_TRUE(values.empty());
}

// ============================================================================
// Deduplication Tests
// ============================================================================

TEST_F(RankingIteratorTest, DeduplicationRemovesDuplicates) {
    auto seq = create_duplicate_sequence();
    RankingIterator<int> it(seq, Deduplication::Enabled);  // With deduplication
    RankingIterator<int> end;
    
    std::vector<int> values;
    for (; it != end; ++it) {
        values.push_back((*it).first);
    }
    
    EXPECT_EQ(values, (std::vector<int>{1, 2, 3}));
}

TEST_F(RankingIteratorTest, NoDeduplicationKeepsAllElements) {
    auto seq = create_duplicate_sequence();
    RankingIterator<int> it(seq, Deduplication::Disabled);  // Without deduplication
    RankingIterator<int> end;
    
    std::vector<int> values;
    for (; it != end; ++it) {
        values.push_back((*it).first);
    }
    
    EXPECT_EQ(values, (std::vector<int>{1, 1, 2, 2, 3}));
}

TEST_F(RankingIteratorTest, DeduplicationKeepsFirstRank) {
    auto seq = create_duplicate_sequence();  // [1@0, 1@1, 2@2, 2@3, 3@4]
    RankingIterator<int> it(seq, Deduplication::Enabled);
    RankingIterator<int> end;
    
    std::vector<std::pair<int, uint64_t>> pairs;
    for (; it != end; ++it) {
        auto [value, rank] = *it;
        pairs.push_back({value, rank.value()});
    }
    
    // Should keep first occurrence of each value (minimum rank)
    std::vector<std::pair<int, uint64_t>> expected = {
        {1, 0}, {2, 2}, {3, 4}
    };
    EXPECT_EQ(pairs, expected);
}

TEST_F(RankingIteratorTest, AllDuplicatesYieldsSingleElement) {
    auto seq = create_all_duplicates();  // [5@0, 5@1, 5@2]
    RankingIterator<int> it(seq, Deduplication::Enabled);
    RankingIterator<int> end;
    
    std::vector<int> values;
    for (; it != end; ++it) {
        values.push_back((*it).first);
    }
    
    EXPECT_EQ(values, (std::vector<int>{5}));
}

TEST_F(RankingIteratorTest, DeduplicationOnConstructor) {
    // Sequence starts with duplicates: [7@0, 7@1, 8@2]
    auto seq = make_lazy_element(7, Rank::zero(), []() {
        return make_lazy_element(7, Rank::from_value(1), []() {
            return make_terminal(8, Rank::from_value(2));
        });
    });
    
    RankingIterator<int> it(seq, Deduplication::Enabled);
    
    // Constructor should skip the first duplicate
    EXPECT_EQ((*it).first, 7);
    EXPECT_EQ((*it).second, Rank::zero());
}

// ============================================================================
// Complex Type Tests
// ============================================================================

TEST_F(RankingIteratorTest, IterateOverStrings) {
    auto seq = make_lazy_element(std::string("alpha"), Rank::zero(), []() {
        return make_lazy_element(std::string("beta"), Rank::from_value(1), []() {
            return make_terminal(std::string("gamma"), Rank::from_value(2));
        });
    });
    
    RankingIterator<std::string> it(seq, Deduplication::Disabled);
    RankingIterator<std::string> end;
    
    std::vector<std::string> values;
    for (; it != end; ++it) {
        values.push_back((*it).first);
    }
    
    EXPECT_EQ(values, (std::vector<std::string>{"alpha", "beta", "gamma"}));
}

TEST_F(RankingIteratorTest, DeduplicateStrings) {
    auto seq = make_lazy_element(std::string("hello"), Rank::zero(), []() {
        return make_lazy_element(std::string("hello"), Rank::from_value(1), []() {
            return make_terminal(std::string("world"), Rank::from_value(2));
        });
    });
    
    RankingIterator<std::string> it(seq, Deduplication::Enabled);
    RankingIterator<std::string> end;
    
    std::vector<std::string> values;
    for (; it != end; ++it) {
        values.push_back((*it).first);
    }
    
    EXPECT_EQ(values, (std::vector<std::string>{"hello", "world"}));
}

TEST_F(RankingIteratorTest, IterateOverVectors) {
    auto seq = make_lazy_element(std::vector<int>{1, 2}, Rank::zero(), []() {
        return make_terminal(std::vector<int>{3, 4}, Rank::from_value(1));
    });
    
    RankingIterator<std::vector<int>> it(seq, Deduplication::Disabled);
    RankingIterator<std::vector<int>> end;
    
    std::vector<std::vector<int>> values;
    for (; it != end; ++it) {
        values.push_back((*it).first);
    }
    
    EXPECT_EQ(values.size(), 2);
    EXPECT_EQ(values[0], (std::vector<int>{1, 2}));
    EXPECT_EQ(values[1], (std::vector<int>{3, 4}));
}

// ============================================================================
// C++20 Ranges Tests
// ============================================================================

TEST_F(RankingIteratorTest, WorksWithStdRangesAlgorithms) {
    auto seq = create_simple_sequence();
    RankingIterator<int> begin(seq, Deduplication::Disabled);
    RankingIterator<int> end;
    
    // Count elements using std::ranges::distance
    auto count = std::ranges::distance(begin, end);
    EXPECT_EQ(count, 3);
}

TEST_F(RankingIteratorTest, WorksWithStdRangesFind) {
    auto seq = create_simple_sequence();
    RankingIterator<int> begin(seq, Deduplication::Disabled);
    RankingIterator<int> end;
    
    // Find value 2 in sequence
    auto it = std::ranges::find_if(begin, end, [](const auto& pair) {
        return pair.first == 2;
    });
    
    EXPECT_NE(it, end);
    EXPECT_EQ((*it).first, 2);
}

TEST_F(RankingIteratorTest, WorksWithStdRangesTransform) {
    auto seq = create_simple_sequence();
    RankingIterator<int> begin(seq, Deduplication::Disabled);
    RankingIterator<int> end;
    
    // Extract just the values
    std::vector<int> values;
    std::ranges::transform(begin, end, std::back_inserter(values),
                          [](const auto& pair) { return pair.first; });
    
    EXPECT_EQ(values, (std::vector<int>{1, 2, 3}));
}

// ============================================================================
// Edge Case Tests
// ============================================================================

TEST_F(RankingIteratorTest, IteratorCopyIsIndependent) {
    auto seq = create_simple_sequence();
    RankingIterator<int> it1(seq, Deduplication::Disabled);
    RankingIterator<int> it2 = it1;
    
    EXPECT_EQ(it1, it2);
    ++it1;
    EXPECT_NE(it1, it2);
    EXPECT_EQ((*it1).first, 2);
    EXPECT_EQ((*it2).first, 1);
}

TEST_F(RankingIteratorTest, LongSequenceIteration) {
    // Create a longer sequence to test performance
    std::function<std::shared_ptr<RankingElement<int>>(int)> make_seq;
    make_seq = [&make_seq](int n) -> std::shared_ptr<RankingElement<int>> {
        if (n == 0) return nullptr;
        if (n == 1) return make_terminal(n, Rank::from_value(n - 1));
        return make_lazy_element(n, Rank::from_value(n - 1), [n, &make_seq]() {
            return make_seq(n - 1);
        });
    };
    
    auto seq = make_seq(100);
    RankingIterator<int> it(seq, Deduplication::Disabled);
    RankingIterator<int> end;
    
    int count = 0;
    for (; it != end; ++it) {
        ++count;
    }
    
    EXPECT_EQ(count, 100);
}

TEST_F(RankingIteratorTest, DeduplicationWithManyConsecutiveDuplicates) {
    // Create sequence: [1@0, 1@1, ..., 1@99, 2@100]
    std::function<std::shared_ptr<RankingElement<int>>(int, int)> make_seq;
    make_seq = [&make_seq](int duplicates_left, int next_value) 
        -> std::shared_ptr<RankingElement<int>> {
        if (duplicates_left == 0) {
            return make_terminal(next_value, Rank::from_value(100));
        }
        return make_lazy_element(1, Rank::from_value(duplicates_left - 1), 
            [duplicates_left, next_value, &make_seq]() {
                return make_seq(duplicates_left - 1, next_value);
            });
    };
    
    auto seq = make_seq(100, 2);
    RankingIterator<int> it(seq, Deduplication::Enabled);
    RankingIterator<int> end;
    
    std::vector<int> values;
    for (; it != end; ++it) {
        values.push_back((*it).first);
    }
    
    EXPECT_EQ(values, (std::vector<int>{1, 2}));
}

TEST_F(RankingIteratorTest, MixedDeduplicationBehavior) {
    // Sequence: [1@0, 2@1, 2@2, 3@3]
    auto seq = make_lazy_element(1, Rank::zero(), []() {
        return make_lazy_element(2, Rank::from_value(1), []() {
            return make_lazy_element(2, Rank::from_value(2), []() {
                return make_terminal(3, Rank::from_value(3));
            });
        });
    });
    
    RankingIterator<int> it(seq, Deduplication::Enabled);
    RankingIterator<int> end;
    
    std::vector<int> values;
    for (; it != end; ++it) {
        values.push_back((*it).first);
    }
    
    EXPECT_EQ(values, (std::vector<int>{1, 2, 3}));
}
