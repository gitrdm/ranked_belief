#include <gtest/gtest.h>

#include "ranked_belief/constructors.hpp"
#include "ranked_belief/type_erasure.hpp"

#include <functional>
#include <optional>
#include <string>
#include <vector>

using namespace ranked_belief;

TEST(TypeErasureTest, WrapsConcreteRanking)
{
	auto rf = from_list<int>({
		{1, Rank::zero()},
		{2, Rank::from_value(1)}
	});

	RankingFunctionAny erased{rf};

	EXPECT_FALSE(erased.is_empty());
	EXPECT_EQ(std::any_cast<int>(erased.first_value()), 1);
	ASSERT_TRUE(erased.first_rank().has_value());
	EXPECT_EQ(erased.first_rank()->value(), 0u);
}

TEST(TypeErasureTest, MapProducesAnyValues)
{
	auto rf = from_values_sequential<int>({1, 2, 3}, Rank::zero(), Deduplication::Enabled);
	RankingFunctionAny erased{rf};

	auto mapped = erased.map([](const std::any& value) {
		return std::any{std::to_string(std::any_cast<int>(value))};
	});

	auto mapped_rf = mapped.cast<std::any>();
	auto first = mapped_rf.first();
	ASSERT_TRUE(first.has_value());
	EXPECT_EQ(std::any_cast<std::string>(first->first), "1");
	EXPECT_EQ(first->second.value(), 0u);
}

TEST(TypeErasureTest, FilterMaintainsUnderlyingType)
{
	auto rf = from_values_sequential<int>({1, 2, 3, 4}, Rank::zero(), Deduplication::Enabled);
	RankingFunctionAny erased{rf};

	auto evens = erased.filter([](const std::any& value) {
		return std::any_cast<int>(value) % 2 == 0;
	});

	auto typed = evens.cast<int>();
	std::vector<int> values;
	for (auto [value, rank] : typed) {
		(void)rank;
		values.push_back(value);
	}

	std::vector<int> expected{2, 4};
	EXPECT_EQ(values, expected);
}

TEST(TypeErasureTest, ObserveValuePreservesLazySemantics)
{
	auto rf = from_values_sequential<int>({5, 6, 7}, Rank::zero(), Deduplication::Enabled);
	RankingFunctionAny erased{rf};

	auto conditioned = erased.observe_value(std::any{6});
	auto typed = conditioned.cast<int>();
	auto first = typed.first();
	ASSERT_TRUE(first.has_value());
	EXPECT_EQ(first->first, 6);
	EXPECT_EQ(first->second.value(), 0u);
}

TEST(TypeErasureTest, MergeDifferentTypesFallsBackToAny)
{
	RankingFunctionAny ints{from_values_sequential<int>({1, 3}, Rank::zero(), Deduplication::Enabled)};
	RankingFunctionAny strings{from_list<std::string>({
		{"two", Rank::from_value(1)}
	})};

	auto merged = ints.merge(strings, Deduplication::Disabled);

	auto first_value = merged.first_value();
	ASSERT_EQ(first_value.type(), typeid(int));
	EXPECT_EQ(std::any_cast<int>(first_value), 1);
	ASSERT_TRUE(merged.first_rank().has_value());
	EXPECT_EQ(merged.first_rank()->value(), 0u);

	auto values = merged.take_n(3);
	ASSERT_EQ(values.size(), 3u);

	std::optional<Rank> rank_one;
	std::optional<Rank> rank_three;
	std::optional<Rank> rank_two_string;

	for (const auto& [payload, rank] : values) {
		if (payload.type() == typeid(int)) {
			const auto value = std::any_cast<int>(payload);
			if (value == 1) {
				rank_one = rank;
			} else if (value == 3) {
				rank_three = rank;
			} else {
				FAIL() << "Unexpected integer in merged ranking";
			}
		} else if (payload.type() == typeid(std::string)) {
			EXPECT_EQ(std::any_cast<std::string>(payload), "two");
			rank_two_string = rank;
		} else {
			FAIL() << "Unexpected type in merged ranking";
		}
	}

	ASSERT_TRUE(rank_one.has_value());
	EXPECT_EQ(rank_one->value(), 0u);
	ASSERT_TRUE(rank_two_string.has_value());
	EXPECT_EQ(rank_two_string->value(), 1u);
	ASSERT_TRUE(rank_three.has_value());
	EXPECT_EQ(rank_three->value(), 1u);
}

TEST(TypeErasureTest, MergeApplyWithFunctionRanking)
{
	RankingFunctionAny values{from_values_sequential<int>({1, 2}, Rank::zero(), Deduplication::Enabled)};

	using AnyFn = std::function<RankingFunctionAny(const std::any&)>;
	auto function_rf = from_list<AnyFn>({
		{AnyFn{[](const std::any& value) {
			int v = std::any_cast<int>(value);
			auto rf = from_list<int>({{v * 10, Rank::zero()}});
			return RankingFunctionAny{rf};
		}}, Rank::zero()}
	}, Deduplication::Disabled);

	RankingFunctionAny functions{function_rf};

	auto applied = values.merge_apply(functions);
	auto results = applied.take_n(2);
	ASSERT_EQ(results.size(), 2u);
	EXPECT_EQ(std::any_cast<int>(results[0].first), 10);
	EXPECT_EQ(results[0].second.value(), 0u);
	EXPECT_EQ(std::any_cast<int>(results[1].first), 20);
	EXPECT_EQ(results[1].second.value(), 1u);
}

TEST(TypeErasureTest, LazyOperationsDeferEvaluation)
{
	std::size_t forced = 0;
	auto generator = from_generator<int>(
		[&forced](std::size_t index) {
			++forced;
			return std::pair{static_cast<int>(index), Rank::from_value(index)};
		},
		0,
		Deduplication::Enabled);

	RankingFunctionAny erased{generator};
	EXPECT_EQ(forced, 1u);

	auto mapped = erased.map([](const std::any& value) {
		int v = std::any_cast<int>(value);
		return std::any{v * 2};
	});
	EXPECT_EQ(forced, 1u);

	auto prefix = mapped.take_n(3);
	EXPECT_EQ(prefix.size(), 3u);
	EXPECT_EQ(forced, 4u);
}



