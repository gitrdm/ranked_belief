/**
 * @file rank_test.cpp
 * @brief Comprehensive tests for the Rank type.
 */

#include "ranked_belief/rank.hpp"

#include <gtest/gtest.h>

#include <limits>
#include <sstream>

using namespace ranked_belief;

// ============================================================================
// Construction Tests
// ============================================================================

TEST(RankTest, DefaultConstructorCreatesZero) {
    Rank r;
    EXPECT_FALSE(r.is_infinity());
    EXPECT_TRUE(r.is_finite());
    EXPECT_EQ(r.value(), 0);
}

TEST(RankTest, ZeroFactoryCreatesZero) {
    Rank r = Rank::zero();
    EXPECT_FALSE(r.is_infinity());
    EXPECT_TRUE(r.is_finite());
    EXPECT_EQ(r.value(), 0);
}

TEST(RankTest, InfinityFactoryCreatesInfinity) {
    Rank r = Rank::infinity();
    EXPECT_TRUE(r.is_infinity());
    EXPECT_FALSE(r.is_finite());
}

TEST(RankTest, FromValueCreatesFiniteRank) {
    Rank r = Rank::from_value(42);
    EXPECT_FALSE(r.is_infinity());
    EXPECT_EQ(r.value(), 42);
}

TEST(RankTest, FromValueWithZeroCreatesZero) {
    Rank r = Rank::from_value(0);
    EXPECT_FALSE(r.is_infinity());
    EXPECT_EQ(r.value(), 0);
}

TEST(RankTest, FromValueWithMaxFiniteCreatesMaxRank) {
    uint64_t max_val = Rank::max_finite_value() - 1;
    Rank r = Rank::from_value(max_val);
    EXPECT_FALSE(r.is_infinity());
    EXPECT_EQ(r.value(), max_val);
}

TEST(RankTest, FromValueThrowsOnMaxFiniteValue) {
    EXPECT_THROW({ [[maybe_unused]] auto r = Rank::from_value(Rank::max_finite_value()); },
                 std::invalid_argument);
}

TEST(RankTest, FromValueThrowsOnExcessiveValue) {
    EXPECT_THROW(
        { [[maybe_unused]] auto r = Rank::from_value(std::numeric_limits<uint64_t>::max()); },
        std::invalid_argument);
}

TEST(RankTest, MaxFiniteValueIs2To63Minus1) {
    EXPECT_EQ(Rank::max_finite_value(), (1ULL << 63) - 1);
    EXPECT_EQ(Rank::max_finite_value(), std::numeric_limits<int64_t>::max());
}

// ============================================================================
// Value Access Tests
// ============================================================================

TEST(RankTest, ValueReturnsCorrectValue) {
    Rank r = Rank::from_value(123);
    EXPECT_EQ(r.value(), 123);
}

TEST(RankTest, ValueThrowsOnInfinity) {
    Rank r = Rank::infinity();
    EXPECT_THROW({ [[maybe_unused]] auto v = r.value(); }, std::logic_error);
}

TEST(RankTest, ValueOrReturnsValueForFinite) {
    Rank r = Rank::from_value(42);
    EXPECT_EQ(r.value_or(999), 42);
}

TEST(RankTest, ValueOrReturnsDefaultForInfinity) {
    Rank r = Rank::infinity();
    EXPECT_EQ(r.value_or(999), 999);
}

// ============================================================================
// Addition Tests
// ============================================================================

TEST(RankTest, AdditionOfFiniteRanks) {
    Rank a = Rank::from_value(10);
    Rank b = Rank::from_value(20);
    Rank c = a + b;
    EXPECT_FALSE(c.is_infinity());
    EXPECT_EQ(c.value(), 30);
}

TEST(RankTest, AdditionWithZero) {
    Rank a = Rank::from_value(42);
    Rank z = Rank::zero();
    Rank c = a + z;
    EXPECT_FALSE(c.is_infinity());
    EXPECT_EQ(c.value(), 42);
}

TEST(RankTest, AdditionIsCommutative) {
    Rank a = Rank::from_value(7);
    Rank b = Rank::from_value(13);
    EXPECT_EQ((a + b).value(), (b + a).value());
}

TEST(RankTest, AdditionWithInfinityYieldsInfinity) {
    Rank finite = Rank::from_value(100);
    Rank inf = Rank::infinity();
    
    Rank result1 = finite + inf;
    Rank result2 = inf + finite;
    
    EXPECT_TRUE(result1.is_infinity());
    EXPECT_TRUE(result2.is_infinity());
}

TEST(RankTest, AdditionOfTwoInfinitiesYieldsInfinity) {
    Rank inf1 = Rank::infinity();
    Rank inf2 = Rank::infinity();
    Rank result = inf1 + inf2;
    EXPECT_TRUE(result.is_infinity());
}

TEST(RankTest, AdditionThrowsOnOverflow) {
    uint64_t max_val = Rank::max_finite_value();
    Rank a = Rank::from_value(max_val - 10);
    Rank b = Rank::from_value(11);
    
    EXPECT_THROW({ [[maybe_unused]] auto c = a + b; }, std::overflow_error);
}

TEST(RankTest, AdditionNearMaxDoesNotOverflow) {
    uint64_t max_val = Rank::max_finite_value();
    Rank a = Rank::from_value(max_val - 100);
    Rank b = Rank::from_value(50);
    
    Rank c = a + b;
    EXPECT_FALSE(c.is_infinity());
    EXPECT_EQ(c.value(), max_val - 50);
}

// ============================================================================
// Subtraction Tests
// ============================================================================

TEST(RankTest, SubtractionOfFiniteRanks) {
    Rank a = Rank::from_value(30);
    Rank b = Rank::from_value(10);
    Rank c = a - b;
    EXPECT_FALSE(c.is_infinity());
    EXPECT_EQ(c.value(), 20);
}

TEST(RankTest, SubtractionToZero) {
    Rank a = Rank::from_value(42);
    Rank b = Rank::from_value(42);
    Rank c = a - b;
    EXPECT_FALSE(c.is_infinity());
    EXPECT_EQ(c.value(), 0);
}

TEST(RankTest, SubtractionFromZeroThrows) {
    Rank z = Rank::zero();
    Rank a = Rank::from_value(1);
    EXPECT_THROW({ [[maybe_unused]] auto c = z - a; }, std::underflow_error);
}

TEST(RankTest, SubtractionThrowsOnUnderflow) {
    Rank a = Rank::from_value(10);
    Rank b = Rank::from_value(20);
    EXPECT_THROW({ [[maybe_unused]] auto c = a - b; }, std::underflow_error);
}

TEST(RankTest, SubtractionFromInfinityThrows) {
    Rank inf = Rank::infinity();
    Rank finite = Rank::from_value(10);
    EXPECT_THROW({ [[maybe_unused]] auto c = inf - finite; }, std::logic_error);
}

TEST(RankTest, SubtractionOfInfinityThrows) {
    Rank finite = Rank::from_value(10);
    Rank inf = Rank::infinity();
    EXPECT_THROW({ [[maybe_unused]] auto c = finite - inf; }, std::logic_error);
}

// ============================================================================
// Min/Max Tests
// ============================================================================

TEST(RankTest, MinOfTwoFiniteRanks) {
    Rank a = Rank::from_value(10);
    Rank b = Rank::from_value(20);
    EXPECT_EQ(a.min(b).value(), 10);
    EXPECT_EQ(b.min(a).value(), 10);
}

TEST(RankTest, MinWithEqualRanks) {
    Rank a = Rank::from_value(15);
    Rank b = Rank::from_value(15);
    EXPECT_EQ(a.min(b).value(), 15);
}

TEST(RankTest, MinWithZero) {
    Rank z = Rank::zero();
    Rank a = Rank::from_value(42);
    EXPECT_EQ(z.min(a).value(), 0);
    EXPECT_EQ(a.min(z).value(), 0);
}

TEST(RankTest, MinWithInfinity) {
    Rank finite = Rank::from_value(100);
    Rank inf = Rank::infinity();
    
    Rank result1 = finite.min(inf);
    Rank result2 = inf.min(finite);
    
    EXPECT_FALSE(result1.is_infinity());
    EXPECT_EQ(result1.value(), 100);
    EXPECT_FALSE(result2.is_infinity());
    EXPECT_EQ(result2.value(), 100);
}

TEST(RankTest, MinOfTwoInfinities) {
    Rank inf1 = Rank::infinity();
    Rank inf2 = Rank::infinity();
    EXPECT_TRUE(inf1.min(inf2).is_infinity());
}

TEST(RankTest, MaxOfTwoFiniteRanks) {
    Rank a = Rank::from_value(10);
    Rank b = Rank::from_value(20);
    EXPECT_EQ(a.max(b).value(), 20);
    EXPECT_EQ(b.max(a).value(), 20);
}

TEST(RankTest, MaxWithEqualRanks) {
    Rank a = Rank::from_value(15);
    Rank b = Rank::from_value(15);
    EXPECT_EQ(a.max(b).value(), 15);
}

TEST(RankTest, MaxWithInfinity) {
    Rank finite = Rank::from_value(100);
    Rank inf = Rank::infinity();
    
    EXPECT_TRUE(finite.max(inf).is_infinity());
    EXPECT_TRUE(inf.max(finite).is_infinity());
}

TEST(RankTest, MaxOfTwoInfinities) {
    Rank inf1 = Rank::infinity();
    Rank inf2 = Rank::infinity();
    EXPECT_TRUE(inf1.max(inf2).is_infinity());
}

// ============================================================================
// Comparison Tests
// ============================================================================

TEST(RankTest, EqualityOfFiniteRanks) {
    Rank a = Rank::from_value(42);
    Rank b = Rank::from_value(42);
    Rank c = Rank::from_value(43);
    
    EXPECT_EQ(a, b);
    EXPECT_NE(a, c);
}

TEST(RankTest, EqualityOfZeros) {
    Rank z1 = Rank::zero();
    Rank z2 = Rank::from_value(0);
    EXPECT_EQ(z1, z2);
}

TEST(RankTest, EqualityOfInfinities) {
    Rank inf1 = Rank::infinity();
    Rank inf2 = Rank::infinity();
    EXPECT_EQ(inf1, inf2);
}

TEST(RankTest, InequalityOfFiniteAndInfinite) {
    Rank finite = Rank::from_value(1000000);
    Rank inf = Rank::infinity();
    EXPECT_NE(finite, inf);
}

TEST(RankTest, LessThanForFiniteRanks) {
    Rank a = Rank::from_value(10);
    Rank b = Rank::from_value(20);
    
    EXPECT_LT(a, b);
    EXPECT_FALSE(b < a);
    EXPECT_FALSE(a < a);
}

TEST(RankTest, LessThanWithInfinity) {
    Rank finite = Rank::from_value(1000000);
    Rank inf = Rank::infinity();
    
    EXPECT_LT(finite, inf);
    EXPECT_FALSE(inf < finite);
}

TEST(RankTest, GreaterThanForFiniteRanks) {
    Rank a = Rank::from_value(10);
    Rank b = Rank::from_value(20);
    
    EXPECT_GT(b, a);
    EXPECT_FALSE(a > b);
    EXPECT_FALSE(a > a);
}

TEST(RankTest, GreaterThanWithInfinity) {
    Rank finite = Rank::from_value(1000000);
    Rank inf = Rank::infinity();
    
    EXPECT_GT(inf, finite);
    EXPECT_FALSE(finite > inf);
}

TEST(RankTest, LessThanOrEqualForFiniteRanks) {
    Rank a = Rank::from_value(10);
    Rank b = Rank::from_value(20);
    Rank c = Rank::from_value(10);
    
    EXPECT_LE(a, b);
    EXPECT_LE(a, c);
    EXPECT_FALSE(b <= a);
}

TEST(RankTest, GreaterThanOrEqualForFiniteRanks) {
    Rank a = Rank::from_value(10);
    Rank b = Rank::from_value(20);
    Rank c = Rank::from_value(20);
    
    EXPECT_GE(b, a);
    EXPECT_GE(b, c);
    EXPECT_FALSE(a >= b);
}

TEST(RankTest, ThreeWayComparisonFiniteRanks) {
    Rank a = Rank::from_value(10);
    Rank b = Rank::from_value(20);
    Rank c = Rank::from_value(10);
    
    EXPECT_EQ(a <=> b, std::strong_ordering::less);
    EXPECT_EQ(b <=> a, std::strong_ordering::greater);
    EXPECT_EQ(a <=> c, std::strong_ordering::equal);
}

TEST(RankTest, ThreeWayComparisonWithInfinity) {
    Rank finite = Rank::from_value(100);
    Rank inf1 = Rank::infinity();
    Rank inf2 = Rank::infinity();
    
    EXPECT_EQ(finite <=> inf1, std::strong_ordering::less);
    EXPECT_EQ(inf1 <=> finite, std::strong_ordering::greater);
    EXPECT_EQ(inf1 <=> inf2, std::strong_ordering::equal);
}

// ============================================================================
// Increment/Decrement Tests
// ============================================================================

TEST(RankTest, PrefixIncrement) {
    Rank r = Rank::from_value(10);
    Rank& result = ++r;
    EXPECT_EQ(r.value(), 11);
    EXPECT_EQ(&result, &r);  // Returns reference to same object
}

TEST(RankTest, PostfixIncrement) {
    Rank r = Rank::from_value(10);
    Rank old = r++;
    EXPECT_EQ(old.value(), 10);
    EXPECT_EQ(r.value(), 11);
}

TEST(RankTest, IncrementFromZero) {
    Rank r = Rank::zero();
    ++r;
    EXPECT_EQ(r.value(), 1);
}

TEST(RankTest, IncrementThrowsAtMaxFinite) {
    Rank r = Rank::from_value(Rank::max_finite_value() - 1);
    EXPECT_THROW(++r, std::overflow_error);
}

TEST(RankTest, IncrementThrowsOnInfinity) {
    Rank r = Rank::infinity();
    EXPECT_THROW(++r, std::logic_error);
}

TEST(RankTest, PrefixDecrement) {
    Rank r = Rank::from_value(10);
    Rank& result = --r;
    EXPECT_EQ(r.value(), 9);
    EXPECT_EQ(&result, &r);
}

TEST(RankTest, PostfixDecrement) {
    Rank r = Rank::from_value(10);
    Rank old = r--;
    EXPECT_EQ(old.value(), 10);
    EXPECT_EQ(r.value(), 9);
}

TEST(RankTest, DecrementToZero) {
    Rank r = Rank::from_value(1);
    --r;
    EXPECT_EQ(r.value(), 0);
}

TEST(RankTest, DecrementThrowsAtZero) {
    Rank r = Rank::zero();
    EXPECT_THROW(--r, std::underflow_error);
}

TEST(RankTest, DecrementThrowsOnInfinity) {
    Rank r = Rank::infinity();
    EXPECT_THROW(--r, std::logic_error);
}

// ============================================================================
// Stream Output Tests
// ============================================================================

TEST(RankTest, StreamOutputForFiniteRank) {
    Rank r = Rank::from_value(42);
    std::ostringstream oss;
    oss << r;
    EXPECT_EQ(oss.str(), "42");
}

TEST(RankTest, StreamOutputForZero) {
    Rank r = Rank::zero();
    std::ostringstream oss;
    oss << r;
    EXPECT_EQ(oss.str(), "0");
}

TEST(RankTest, StreamOutputForInfinity) {
    Rank r = Rank::infinity();
    std::ostringstream oss;
    oss << r;
    EXPECT_EQ(oss.str(), "∞");
}

TEST(RankTest, StreamOutputForLargeFiniteRank) {
    uint64_t large_val = Rank::max_finite_value() - 1;
    Rank r = Rank::from_value(large_val);
    std::ostringstream oss;
    oss << r;
    EXPECT_EQ(oss.str(), std::to_string(large_val));
}

// ============================================================================
// Edge Case and Stress Tests
// ============================================================================

TEST(RankTest, MultipleOperationsChaining) {
    Rank a = Rank::from_value(5);
    Rank b = Rank::from_value(3);
    Rank c = Rank::from_value(2);
    
    Rank result = a + b + c;
    EXPECT_EQ(result.value(), 10);
}

TEST(RankTest, MinMaxChaining) {
    Rank a = Rank::from_value(10);
    Rank b = Rank::from_value(20);
    Rank c = Rank::from_value(15);
    
    Rank min_result = a.min(b).min(c);
    Rank max_result = a.max(b).max(c);
    
    EXPECT_EQ(min_result.value(), 10);
    EXPECT_EQ(max_result.value(), 20);
}

TEST(RankTest, CopySemantics) {
    Rank original = Rank::from_value(42);
    Rank copy = original;
    
    EXPECT_EQ(original, copy);
    EXPECT_EQ(copy.value(), 42);
}

TEST(RankTest, AssignmentSemantics) {
    Rank a = Rank::from_value(10);
    Rank b = Rank::from_value(20);
    
    a = b;
    EXPECT_EQ(a.value(), 20);
    EXPECT_EQ(b.value(), 20);
}

TEST(RankTest, ConstCorrectness) {
    const Rank r = Rank::from_value(42);
    EXPECT_EQ(r.value(), 42);
    EXPECT_FALSE(r.is_infinity());
    
    const Rank inf = Rank::infinity();
    EXPECT_TRUE(inf.is_infinity());
}
