/**
 * @file promise_test.cpp
 * @brief Comprehensive tests for the Promise<T> lazy evaluation mechanism.
 */

#include "ranked_belief/promise.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

using namespace ranked_belief;

// ============================================================================
// Construction Tests
// ============================================================================

TEST(PromiseTest, ConstructFromFunction) {
    Promise<int> p([]() { return 42; });
    EXPECT_FALSE(p.is_forced());
    EXPECT_FALSE(p.has_value());
}

TEST(PromiseTest, ConstructFromValue) {
    Promise<int> p(42);
    EXPECT_TRUE(p.has_value());
    EXPECT_EQ(p.force(), 42);
}

TEST(PromiseTest, ConstructFromNullFunctionThrows) {
    std::function<int()> null_func;
    EXPECT_THROW(Promise<int> p(null_func), std::invalid_argument);
}

TEST(PromiseTest, MakePromiseHelperDeducesType) {
    auto p = make_promise([]() { return 42; });
    EXPECT_EQ(p.force(), 42);
}

TEST(PromiseTest, MakePromiseValueHelperDeducesType) {
    auto p = make_promise_value(42);
    EXPECT_EQ(p.force(), 42);
}

TEST(PromiseTest, MakePromiseWithString) {
    auto p = make_promise([]() { return std::string("hello"); });
    EXPECT_EQ(p.force(), "hello");
}

// ============================================================================
// Basic Force Tests
// ============================================================================

TEST(PromiseTest, ForceExecutesComputation) {
    int execution_count = 0;
    Promise<int> p([&execution_count]() {
        ++execution_count;
        return 42;
    });

    EXPECT_EQ(execution_count, 0);
    int result = p.force();
    EXPECT_EQ(result, 42);
    EXPECT_EQ(execution_count, 1);
}

TEST(PromiseTest, ForceReturnsSameValueOnMultipleCalls) {
    int value = 0;
    Promise<int> p([&value]() { return ++value; });

    int first = p.force();
    int second = p.force();
    int third = p.force();

    EXPECT_EQ(first, 1);
    EXPECT_EQ(second, 1);
    EXPECT_EQ(third, 1);
}

TEST(PromiseTest, ForceExecutesOnlyOnce) {
    int execution_count = 0;
    Promise<int> p([&execution_count]() {
        ++execution_count;
        return 42;
    });

    p.force();
    p.force();
    p.force();

    EXPECT_EQ(execution_count, 1);
}

TEST(PromiseTest, ConstForceWorks) {
    const Promise<int> p([]() { return 42; });
    EXPECT_EQ(p.force(), 42);
}

TEST(PromiseTest, ConstForceExecutesOnlyOnce) {
    std::atomic<int> execution_count{0};
    const Promise<int> p([&execution_count]() {
        ++execution_count;
        return 42;
    });

    p.force();
    p.force();

    EXPECT_EQ(execution_count.load(), 1);
}

// ============================================================================
// State Query Tests
// ============================================================================

TEST(PromiseTest, IsForcedReturnsFalseInitially) {
    Promise<int> p([]() { return 42; });
    EXPECT_FALSE(p.is_forced());
}

TEST(PromiseTest, IsForcedReturnsTrueAfterForce) {
    Promise<int> p([]() { return 42; });
    p.force();
    EXPECT_TRUE(p.is_forced());
}

TEST(PromiseTest, HasValueReturnsFalseInitially) {
    Promise<int> p([]() { return 42; });
    EXPECT_FALSE(p.has_value());
}

TEST(PromiseTest, HasValueReturnsTrueAfterSuccessfulForce) {
    Promise<int> p([]() { return 42; });
    p.force();
    EXPECT_TRUE(p.has_value());
}

TEST(PromiseTest, HasExceptionReturnsFalseInitially) {
    Promise<int> p([]() { return 42; });
    EXPECT_FALSE(p.has_exception());
}

TEST(PromiseTest, HasExceptionReturnsFalseAfterSuccessfulForce) {
    Promise<int> p([]() { return 42; });
    p.force();
    EXPECT_FALSE(p.has_exception());
}

// ============================================================================
// Exception Handling Tests
// ============================================================================

TEST(PromiseTest, ForceThrowsExceptionFromComputation) {
    Promise<int> p([]() -> int { throw std::runtime_error("computation failed"); });

    EXPECT_THROW({ [[maybe_unused]] auto v = p.force(); }, std::runtime_error);
}

TEST(PromiseTest, ExceptionIsCachedAndRethrownOnSubsequentForce) {
    int execution_count = 0;
    Promise<int> p([&execution_count]() -> int {
        ++execution_count;
        throw std::runtime_error("computation failed");
    });

    EXPECT_THROW({ [[maybe_unused]] auto v = p.force(); }, std::runtime_error);
    EXPECT_THROW({ [[maybe_unused]] auto v = p.force(); }, std::runtime_error);
    EXPECT_THROW({ [[maybe_unused]] auto v = p.force(); }, std::runtime_error);

    EXPECT_EQ(execution_count, 1);  // Only executed once
}

TEST(PromiseTest, HasExceptionReturnsTrueAfterFailedForce) {
    Promise<int> p([]() -> int { throw std::runtime_error("error"); });

    try {
        p.force();
    } catch (...) {
        // Ignore
    }

    EXPECT_TRUE(p.has_exception());
    EXPECT_FALSE(p.has_value());
}

TEST(PromiseTest, ExceptionTypeIsPreserved) {
    Promise<int> p([]() -> int { throw std::invalid_argument("specific error"); });

    EXPECT_THROW({ [[maybe_unused]] auto v = p.force(); }, std::invalid_argument);
}

// ============================================================================
// Move Semantics Tests
// ============================================================================

TEST(PromiseTest, MoveConstruction) {
    Promise<int> p1([]() { return 42; });
    Promise<int> p2(std::move(p1));

    EXPECT_EQ(p2.force(), 42);
}

TEST(PromiseTest, MoveConstructionPreservesValue) {
    Promise<int> p1([]() { return 42; });
    p1.force();  // Force before moving

    Promise<int> p2(std::move(p1));
    EXPECT_EQ(p2.force(), 42);
}

TEST(PromiseTest, MoveAssignment) {
    Promise<int> p1([]() { return 42; });
    Promise<int> p2([]() { return 99; });

    p2 = std::move(p1);
    EXPECT_EQ(p2.force(), 42);
}

TEST(PromiseTest, MoveAssignmentPreservesValue) {
    Promise<int> p1([]() { return 42; });
    p1.force();  // Force before moving

    Promise<int> p2([]() { return 99; });
    p2 = std::move(p1);

    EXPECT_EQ(p2.force(), 42);
}

TEST(PromiseTest, SelfMoveAssignment) {
    Promise<int> p([]() { return 42; });
    
    // Self-assignment (explicitly suppress the warning)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wself-move"
    p = std::move(p);
    #pragma GCC diagnostic pop

    EXPECT_EQ(p.force(), 42);
}

TEST(PromiseTest, MoveToVectorWorks) {
    std::vector<Promise<int>> promises;
    promises.push_back(Promise<int>([]() { return 1; }));
    promises.push_back(Promise<int>([]() { return 2; }));
    promises.push_back(Promise<int>([]() { return 3; }));

    EXPECT_EQ(promises[0].force(), 1);
    EXPECT_EQ(promises[1].force(), 2);
    EXPECT_EQ(promises[2].force(), 3);
}

// ============================================================================
// Thread Safety Tests
// ============================================================================

TEST(PromiseTest, ConcurrentForceExecutesOnlyOnce) {
    std::atomic<int> execution_count{0};
    Promise<int> p([&execution_count]() {
        ++execution_count;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        return 42;
    });

    constexpr int num_threads = 10;
    std::vector<std::thread> threads;
    std::vector<int> results(num_threads);

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&p, &results, i]() { results[i] = p.force(); });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(execution_count.load(), 1);
    for (int i = 0; i < num_threads; ++i) {
        EXPECT_EQ(results[i], 42);
    }
}

TEST(PromiseTest, ConcurrentForceWithException) {
    std::atomic<int> execution_count{0};
    Promise<int> p([&execution_count]() -> int {
        ++execution_count;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        throw std::runtime_error("error");
    });

    constexpr int num_threads = 10;
    std::vector<std::thread> threads;
    std::atomic<int> exception_count{0};

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&p, &exception_count]() {
            try {
                p.force();
            } catch (const std::runtime_error&) {
                ++exception_count;
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(execution_count.load(), 1);
    EXPECT_EQ(exception_count.load(), num_threads);
}

TEST(PromiseTest, ConcurrentConstForce) {
    std::atomic<int> execution_count{0};
    const Promise<int> p([&execution_count]() {
        ++execution_count;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        return 42;
    });

    constexpr int num_threads = 10;
    std::vector<std::thread> threads;
    std::vector<int> results(num_threads);

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&p, &results, i]() { results[i] = p.force(); });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(execution_count.load(), 1);
    for (int i = 0; i < num_threads; ++i) {
        EXPECT_EQ(results[i], 42);
    }
}

// ============================================================================
// Complex Type Tests
// ============================================================================

TEST(PromiseTest, PromiseOfString) {
    Promise<std::string> p([]() { return std::string("hello world"); });
    EXPECT_EQ(p.force(), "hello world");
}

TEST(PromiseTest, PromiseOfVector) {
    Promise<std::vector<int>> p([]() { return std::vector<int>{1, 2, 3, 4, 5}; });
    auto result = p.force();
    EXPECT_EQ(result.size(), 5);
    EXPECT_EQ(result[0], 1);
    EXPECT_EQ(result[4], 5);
}

TEST(PromiseTest, PromiseOfUniquePtr) {
    Promise<std::unique_ptr<int>> p([]() { return std::make_unique<int>(42); });
    auto& result = p.force();
    EXPECT_NE(result, nullptr);
    EXPECT_EQ(*result, 42);
}

struct CustomType {
    int value;
    std::string name;

    bool operator==(const CustomType& other) const {
        return value == other.value && name == other.name;
    }
};

TEST(PromiseTest, PromiseOfCustomType) {
    Promise<CustomType> p([]() { return CustomType{42, "test"}; });
    auto result = p.force();
    EXPECT_EQ(result.value, 42);
    EXPECT_EQ(result.name, "test");
}

// ============================================================================
// Reference Return Tests
// ============================================================================

TEST(PromiseTest, ForceReturnsReference) {
    Promise<std::vector<int>> p([]() { return std::vector<int>{1, 2, 3}; });

    std::vector<int>& ref1 = p.force();
    std::vector<int>& ref2 = p.force();

    // Should be the same object
    EXPECT_EQ(&ref1, &ref2);
}

TEST(PromiseTest, CanModifyThroughReference) {
    Promise<std::vector<int>> p([]() { return std::vector<int>{1, 2, 3}; });

    std::vector<int>& vec = p.force();
    vec.push_back(4);

    // Subsequent force returns modified vector
    std::vector<int>& vec2 = p.force();
    EXPECT_EQ(vec2.size(), 4);
    EXPECT_EQ(vec2[3], 4);
}

TEST(PromiseTest, ConstForceReturnsConstReference) {
    const Promise<int> p([]() { return 42; });
    const int& ref = p.force();
    EXPECT_EQ(ref, 42);
}

// ============================================================================
// Edge Cases and Stress Tests
// ============================================================================

TEST(PromiseTest, PromiseOfPromise) {
    // Create inner promise on heap to allow moving out of lambda
    auto inner_ptr = std::make_shared<Promise<int>>(make_promise([]() { return 42; }));
    
    Promise<int> outer([inner_ptr]() { return inner_ptr->force(); });

    EXPECT_EQ(outer.force(), 42);
}

TEST(PromiseTest, LargeNumberOfPromises) {
    constexpr int num_promises = 1000;
    std::vector<Promise<int>> promises;

    for (int i = 0; i < num_promises; ++i) {
        promises.push_back(Promise<int>([i]() { return i * 2; }));
    }

    for (int i = 0; i < num_promises; ++i) {
        EXPECT_EQ(promises[i].force(), i * 2);
    }
}

TEST(PromiseTest, PromiseWithExpensiveComputation) {
    int computation_count = 0;
    Promise<int> p([&computation_count]() {
        ++computation_count;
        // Simulate expensive computation
        int sum = 0;
        for (int i = 0; i < 1000; ++i) {
            sum += i;
        }
        return sum;
    });

    int result1 = p.force();
    int result2 = p.force();

    EXPECT_EQ(computation_count, 1);
    EXPECT_EQ(result1, result2);
}

TEST(PromiseTest, ChainedPromiseEvaluation) {
    Promise<int> p1([]() { return 10; });
    Promise<int> p2([&p1]() { return p1.force() * 2; });
    Promise<int> p3([&p2]() { return p2.force() + 5; });

    EXPECT_EQ(p3.force(), 25);  // (10 * 2) + 5
}

TEST(PromiseTest, PromiseWithCapturedState) {
    std::string captured_state = "initial";
    Promise<std::string> p([captured_state]() { return captured_state + " computed"; });

    captured_state = "modified";  // Modify after promise creation

    // Promise should use captured state at creation time
    EXPECT_EQ(p.force(), "initial computed");
}

TEST(PromiseTest, MultipleExceptionTypes) {
    Promise<int> p1([]() -> int { throw std::runtime_error("runtime"); });
    Promise<int> p2([]() -> int { throw std::logic_error("logic"); });
    Promise<int> p3([]() -> int { throw std::invalid_argument("invalid"); });

    EXPECT_THROW({ [[maybe_unused]] auto v = p1.force(); }, std::runtime_error);
    EXPECT_THROW({ [[maybe_unused]] auto v = p2.force(); }, std::logic_error);
    EXPECT_THROW({ [[maybe_unused]] auto v = p3.force(); }, std::invalid_argument);
}

// ============================================================================
// Helper Function Tests
// ============================================================================

TEST(PromiseTest, MakePromiseWithComplexLambda) {
    int x = 10;
    auto p = make_promise([x]() {
        int result = 0;
        for (int i = 0; i < x; ++i) {
            result += i;
        }
        return result;
    });

    EXPECT_EQ(p.force(), 45);  // 0+1+2+...+9
}

TEST(PromiseTest, MakePromiseValueWithVariousTypes) {
    auto p_int = make_promise_value(42);
    auto p_string = make_promise_value(std::string("test"));
    auto p_double = make_promise_value(3.14);

    EXPECT_EQ(p_int.force(), 42);
    EXPECT_EQ(p_string.force(), "test");
    EXPECT_DOUBLE_EQ(p_double.force(), 3.14);
}
