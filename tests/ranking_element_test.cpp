/**
 * @file ranking_element_test.cpp
 * @brief Comprehensive tests for the RankingElement lazy linked list node.
 */

#include "ranked_belief/ranking_element.hpp"

#include <gtest/gtest.h>

#include <string>
#include <thread>
#include <vector>

using namespace ranked_belief;

// ============================================================================
// Construction Tests
// ============================================================================

TEST(RankingElementTest, ConstructWithLazyNext) {
    auto next_elem = make_terminal(2, Rank::from_value(1));
    auto lazy_next = make_promise_value(next_elem);
    
    RankingElement<int> elem(1, Rank::zero(), std::move(lazy_next));
    
    EXPECT_EQ(elem.value(), 1);
    EXPECT_EQ(elem.rank(), Rank::zero());
    EXPECT_FALSE(elem.is_last());
}

TEST(RankingElementTest, ConstructWithKnownNext) {
    auto next_elem = make_terminal(2, Rank::from_value(1));
    RankingElement<int> elem(1, Rank::zero(), next_elem);
    
    EXPECT_EQ(elem.value(), 1);
    EXPECT_EQ(elem.rank(), Rank::zero());
    EXPECT_EQ(elem.next(), next_elem);
}

TEST(RankingElementTest, ConstructTerminalElement) {
    RankingElement<int> elem(42, Rank::from_value(5));
    
    EXPECT_EQ(elem.value(), 42);
    EXPECT_EQ(elem.rank(), Rank::from_value(5));
    EXPECT_TRUE(elem.is_last());
    EXPECT_EQ(elem.next(), nullptr);
}

TEST(RankingElementTest, MakeTerminalHelper) {
    auto elem = make_terminal(100, Rank::from_value(10));
    
    EXPECT_EQ(elem->value(), 100);
    EXPECT_EQ(elem->rank(), Rank::from_value(10));
    EXPECT_TRUE(elem->is_last());
}

TEST(RankingElementTest, MakeElementHelper) {
    auto next = make_terminal(2, Rank::from_value(1));
    auto elem = make_element(1, Rank::zero(), next);
    
    EXPECT_EQ(elem->value(), 1);
    EXPECT_EQ(elem->rank(), Rank::zero());
    EXPECT_EQ(elem->next(), next);
}

TEST(RankingElementTest, MakeLazyElementHelper) {
    int computation_count = 0;
    auto elem = make_lazy_element(1, Rank::zero(), [&computation_count]() {
        ++computation_count;
        return make_terminal(2, Rank::from_value(1));
    });
    
    EXPECT_EQ(computation_count, 0);  // Not computed yet
    auto next = elem->next();
    EXPECT_EQ(computation_count, 1);  // Now computed
    EXPECT_EQ(next->value(), 2);
}

// ============================================================================
// Value and Rank Access Tests
// ============================================================================

TEST(RankingElementTest, ValueAccessForInt) {
    auto elem = make_terminal(42, Rank::zero());
    EXPECT_EQ(elem->value(), 42);
}

TEST(RankingElementTest, ValueAccessForString) {
    auto elem = make_terminal(std::string("hello"), Rank::from_value(3));
    EXPECT_EQ(elem->value(), "hello");
}

TEST(RankingElementTest, RankAccessWithZero) {
    auto elem = make_terminal(1, Rank::zero());
    EXPECT_EQ(elem->rank(), Rank::zero());
}

TEST(RankingElementTest, RankAccessWithNonZero) {
    auto elem = make_terminal(1, Rank::from_value(42));
    EXPECT_EQ(elem->rank(), Rank::from_value(42));
}

TEST(RankingElementTest, RankAccessWithInfinity) {
    auto elem = make_terminal(1, Rank::infinity());
    EXPECT_TRUE(elem->rank().is_infinity());
}

// ============================================================================
// Next Pointer Tests
// ============================================================================

TEST(RankingElementTest, NextOnTerminalReturnsNull) {
    auto elem = make_terminal(1, Rank::zero());
    EXPECT_EQ(elem->next(), nullptr);
}

TEST(RankingElementTest, NextOnNonTerminalReturnsElement) {
    auto next_elem = make_terminal(2, Rank::from_value(1));
    auto elem = make_element(1, Rank::zero(), next_elem);
    
    EXPECT_EQ(elem->next(), next_elem);
}

TEST(RankingElementTest, NextEvaluatesLazyComputation) {
    int computation_count = 0;
    auto elem = make_lazy_element(1, Rank::zero(), [&computation_count]() {
        ++computation_count;
        return make_terminal(2, Rank::from_value(1));
    });
    
    EXPECT_EQ(computation_count, 0);
    [[maybe_unused]] auto result1 = elem->next();
    EXPECT_EQ(computation_count, 1);
}

TEST(RankingElementTest, NextIsMemoized) {
    int computation_count = 0;
    auto elem = make_lazy_element(1, Rank::zero(), [&computation_count]() {
        ++computation_count;
        return make_terminal(2, Rank::from_value(1));
    });
    
    auto next1 = elem->next();
    auto next2 = elem->next();
    auto next3 = elem->next();
    
    EXPECT_EQ(computation_count, 1);  // Computed only once
    EXPECT_EQ(next1, next2);
    EXPECT_EQ(next2, next3);
}

TEST(RankingElementTest, IsLastForTerminalElement) {
    auto elem = make_terminal(1, Rank::zero());
    EXPECT_TRUE(elem->is_last());
}

TEST(RankingElementTest, IsLastForNonTerminalElement) {
    auto next = make_terminal(2, Rank::from_value(1));
    auto elem = make_element(1, Rank::zero(), next);
    EXPECT_FALSE(elem->is_last());
}

TEST(RankingElementTest, NextIsForcedReturnsFalseInitially) {
    auto elem = make_lazy_element(1, Rank::zero(), []() {
        return make_terminal(2, Rank::from_value(1));
    });
    
    EXPECT_FALSE(elem->next_is_forced());
}

TEST(RankingElementTest, NextIsForcedReturnsTrueAfterAccess) {
    auto elem = make_lazy_element(1, Rank::zero(), []() {
        return make_terminal(2, Rank::from_value(1));
    });
    
    [[maybe_unused]] auto result = elem->next();
    EXPECT_TRUE(elem->next_is_forced());
}

// ============================================================================
// Sequence Construction Tests
// ============================================================================

TEST(RankingElementTest, BuildFiniteSequence) {
    // Build sequence: 1@0 -> 2@1 -> 3@2
    auto elem3 = make_terminal(3, Rank::from_value(2));
    auto elem2 = make_element(2, Rank::from_value(1), elem3);
    auto elem1 = make_element(1, Rank::zero(), elem2);
    
    EXPECT_EQ(elem1->value(), 1);
    EXPECT_EQ(elem1->next()->value(), 2);
    EXPECT_EQ(elem1->next()->next()->value(), 3);
    EXPECT_TRUE(elem1->next()->next()->is_last());
}

TEST(RankingElementTest, BuildLazySequence) {
    // Build lazy sequence where each element is computed on demand
    auto elem = make_lazy_element(1, Rank::zero(), []() {
        return make_lazy_element(2, Rank::from_value(1), []() {
            return make_terminal(3, Rank::from_value(2));
        });
    });
    
    EXPECT_EQ(elem->value(), 1);
    
    auto next1 = elem->next();
    EXPECT_EQ(next1->value(), 2);
    
    auto next2 = next1->next();
    EXPECT_EQ(next2->value(), 3);
    EXPECT_TRUE(next2->is_last());
}

TEST(RankingElementTest, TraverseSequence) {
    auto elem3 = make_terminal(3, Rank::from_value(2));
    auto elem2 = make_element(2, Rank::from_value(1), elem3);
    auto elem1 = make_element(1, Rank::zero(), elem2);
    
    std::vector<int> values;
    std::vector<Rank> ranks;
    
    auto current = elem1;
    while (current) {
        values.push_back(current->value());
        ranks.push_back(current->rank());
        current = current->next();
    }
    
    EXPECT_EQ(values.size(), 3);
    EXPECT_EQ(values[0], 1);
    EXPECT_EQ(values[1], 2);
    EXPECT_EQ(values[2], 3);
    EXPECT_EQ(ranks[0], Rank::zero());
    EXPECT_EQ(ranks[1], Rank::from_value(1));
    EXPECT_EQ(ranks[2], Rank::from_value(2));
}

// ============================================================================
// Infinite Sequence Tests
// ============================================================================

TEST(RankingElementTest, MakeInfiniteSequenceBasic) {
    auto infinite = make_infinite_sequence<int>([](size_t i) {
        return std::make_pair(static_cast<int>(i), Rank::from_value(i));
    });
    
    EXPECT_EQ(infinite->value(), 0);
    EXPECT_EQ(infinite->rank(), Rank::zero());
    EXPECT_FALSE(infinite->is_last());
}

TEST(RankingElementTest, InfiniteSequenceFirstElements) {
    auto infinite = make_infinite_sequence<int>([](size_t i) {
        return std::make_pair(static_cast<int>(i * 10), Rank::from_value(i));
    });
    
    EXPECT_EQ(infinite->value(), 0);
    EXPECT_EQ(infinite->next()->value(), 10);
    EXPECT_EQ(infinite->next()->next()->value(), 20);
    EXPECT_EQ(infinite->next()->next()->next()->value(), 30);
}

TEST(RankingElementTest, InfiniteSequenceNeverEnds) {
    auto infinite = make_infinite_sequence<int>([](size_t i) {
        return std::make_pair(static_cast<int>(i), Rank::from_value(i));
    });
    
    auto current = infinite;
    for (int i = 0; i < 100; ++i) {
        EXPECT_FALSE(current->is_last());
        EXPECT_EQ(current->value(), i);
        current = current->next();
    }
}

TEST(RankingElementTest, InfiniteSequenceWithCustomStartIndex) {
    auto infinite = make_infinite_sequence<int>(
        [](size_t i) {
            return std::make_pair(static_cast<int>(i), Rank::from_value(i));
        },
        42);
    
    EXPECT_EQ(infinite->value(), 42);
    EXPECT_EQ(infinite->next()->value(), 43);
    EXPECT_EQ(infinite->next()->next()->value(), 44);
}

TEST(RankingElementTest, InfiniteSequenceLazyEvaluation) {
    int computation_count = 0;
    auto infinite = make_infinite_sequence<int>([&computation_count](size_t i) {
        ++computation_count;
        return std::make_pair(static_cast<int>(i), Rank::from_value(i));
    });
    
    EXPECT_EQ(computation_count, 1);  // Only first element computed
    
    [[maybe_unused]] auto result2 = infinite->next();
    EXPECT_EQ(computation_count, 2);  // Second element now computed
    
    [[maybe_unused]] auto result3 = infinite->next()->next();
    EXPECT_EQ(computation_count, 3);  // Third element now computed
}

// ============================================================================
// Complex Type Tests
// ============================================================================

TEST(RankingElementTest, ElementWithStringValues) {
    auto elem2 = make_terminal(std::string("world"), Rank::from_value(1));
    auto elem1 = make_element(std::string("hello"), Rank::zero(), elem2);
    
    EXPECT_EQ(elem1->value(), "hello");
    EXPECT_EQ(elem1->next()->value(), "world");
}

TEST(RankingElementTest, ElementWithVectorValues) {
    std::vector<int> vec1{1, 2, 3};
    std::vector<int> vec2{4, 5, 6};
    
    auto elem2 = make_terminal(vec2, Rank::from_value(1));
    auto elem1 = make_element(vec1, Rank::zero(), elem2);
    
    EXPECT_EQ(elem1->value(), vec1);
    EXPECT_EQ(elem1->next()->value(), vec2);
}

struct CustomType {
    int x;
    std::string name;
    
    bool operator==(const CustomType& other) const {
        return x == other.x && name == other.name;
    }
};

TEST(RankingElementTest, ElementWithCustomType) {
    CustomType obj1{42, "first"};
    CustomType obj2{99, "second"};
    
    auto elem2 = make_terminal(obj2, Rank::from_value(1));
    auto elem1 = make_element(obj1, Rank::zero(), elem2);
    
    EXPECT_EQ(elem1->value(), obj1);
    EXPECT_EQ(elem1->next()->value(), obj2);
}

// ============================================================================
// Thread Safety Tests
// ============================================================================

TEST(RankingElementTest, ConcurrentNextAccess) {
    std::atomic<int> computation_count{0};
    auto elem = make_lazy_element(1, Rank::zero(), [&computation_count]() {
        ++computation_count;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        return make_terminal(2, Rank::from_value(1));
    });
    
    constexpr int num_threads = 10;
    std::vector<std::thread> threads;
    std::vector<std::shared_ptr<RankingElement<int>>> results(num_threads);
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&elem, &results, i]() {
            results[i] = elem->next();
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_EQ(computation_count.load(), 1);  // Computed only once
    
    // All threads should get the same pointer
    for (int i = 1; i < num_threads; ++i) {
        EXPECT_EQ(results[i], results[0]);
    }
}

TEST(RankingElementTest, ConcurrentTraversal) {
    auto elem3 = make_terminal(3, Rank::from_value(2));
    auto elem2 = make_element(2, Rank::from_value(1), elem3);
    auto elem1 = make_element(1, Rank::zero(), elem2);
    
    constexpr int num_threads = 10;
    std::vector<std::thread> threads;
    std::vector<std::vector<int>> results(num_threads);
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&elem1, &results, i]() {
            auto current = elem1;
            while (current) {
                results[i].push_back(current->value());
                current = current->next();
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    // All threads should see the same sequence
    std::vector<int> expected{1, 2, 3};
    for (int i = 0; i < num_threads; ++i) {
        EXPECT_EQ(results[i], expected);
    }
}

// ============================================================================
// Memory and Performance Tests
// ============================================================================

TEST(RankingElementTest, LongSequenceConstruction) {
    constexpr int length = 1000;
    
    // Build a long sequence
    auto current = make_terminal(length, Rank::from_value(length));
    for (int i = length - 1; i >= 0; --i) {
        current = make_element(i, Rank::from_value(i), current);
    }
    
    // Verify we can traverse it
    int count = 0;
    auto iter = current;
    while (iter) {
        EXPECT_EQ(iter->value(), count);
        ++count;
        iter = iter->next();
    }
    
    EXPECT_EQ(count, length + 1);
}

TEST(RankingElementTest, LazySequenceDoesNotComputeAhead) {
    int max_computed = -1;
    
    std::function<std::shared_ptr<RankingElement<int>>(int)> make_seq;
    make_seq = [&max_computed, &make_seq](int n) {
        max_computed = std::max(max_computed, n);
        if (n >= 100) {
            return make_terminal(n, Rank::from_value(n));
        }
        return make_lazy_element(n, Rank::from_value(n), [&make_seq, n]() {
            return make_seq(n + 1);
        });
    };
    
    auto seq = make_seq(0);
    EXPECT_EQ(max_computed, 0);  // Only first element computed
    
    [[maybe_unused]] auto result4 = seq->next();
    EXPECT_EQ(max_computed, 1);  // Only up to second element
    
    [[maybe_unused]] auto result5 = seq->next()->next();
    EXPECT_EQ(max_computed, 2);  // Only up to third element
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST(RankingElementTest, SingleElementSequence) {
    auto elem = make_terminal(42, Rank::zero());
    
    EXPECT_EQ(elem->value(), 42);
    EXPECT_TRUE(elem->is_last());
    EXPECT_EQ(elem->next(), nullptr);
}

TEST(RankingElementTest, SequenceWithAllSameRanks) {
    auto elem3 = make_terminal(3, Rank::from_value(5));
    auto elem2 = make_element(2, Rank::from_value(5), elem3);
    auto elem1 = make_element(1, Rank::from_value(5), elem2);
    
    EXPECT_EQ(elem1->rank(), Rank::from_value(5));
    EXPECT_EQ(elem1->next()->rank(), Rank::from_value(5));
    EXPECT_EQ(elem1->next()->next()->rank(), Rank::from_value(5));
}

TEST(RankingElementTest, SequenceWithInfiniteRanks) {
    auto elem2 = make_terminal(2, Rank::infinity());
    auto elem1 = make_element(1, Rank::infinity(), elem2);
    
    EXPECT_TRUE(elem1->rank().is_infinity());
    EXPECT_TRUE(elem1->next()->rank().is_infinity());
}

TEST(RankingElementTest, RecursiveLazyConstruction) {
    // Create a recursive structure using lambda capture
    std::function<std::shared_ptr<RankingElement<int>>(int)> make_recursive;
    make_recursive = [&make_recursive](int n) -> std::shared_ptr<RankingElement<int>> {
        if (n > 5) {
            return make_terminal(n, Rank::from_value(n));
        }
        return make_lazy_element(n, Rank::from_value(n), [&make_recursive, n]() {
            return make_recursive(n + 1);
        });
    };
    
    auto seq = make_recursive(1);
    EXPECT_EQ(seq->value(), 1);
    EXPECT_EQ(seq->next()->value(), 2);
    EXPECT_EQ(seq->next()->next()->value(), 3);
}
