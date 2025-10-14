#include <gtest/gtest.h>

#include "ranked_belief/constructors.hpp"
#include "ranked_belief/operations/nrm_exc.hpp"

using namespace ranked_belief;

TEST(NormalExceptionalRegression, ExceptionalEarlierIsHead)
{
    // Normal ranking: values 100@17, 101@18 (non-decreasing ranks)
    std::vector<std::pair<int, Rank>> normal_pairs = {
        {100, Rank::from_value(17)},
        {101, Rank::from_value(18)}
    };
    auto normal = from_list<int>(normal_pairs, Deduplication::Enabled);

    // Exceptional ranking (before shift): single value 42@1
    auto exceptional_thunk = []() {
        std::vector<int> vals = {42};
        return from_values_uniform<int>(vals, Rank::from_value(1), Deduplication::Disabled);
    };

    // Shift exceptional by 2 -> exceptional element becomes rank 3
    auto combined = normal_exceptional(normal, exceptional_thunk, Rank::from_value(2), Deduplication::Enabled);

    auto first = combined.first();
    ASSERT_TRUE(first.has_value());

    // Expect the exceptional element (42@3) to come before normal head 100@17
    EXPECT_EQ(first->first, 42);
    EXPECT_EQ(first->second, Rank::from_value(3));
}
