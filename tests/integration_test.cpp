#include <gtest/gtest.h>

#include "ranked_belief/constructors.hpp"
#include "ranked_belief/operations/map.hpp"
#include "ranked_belief/operations/merge_apply.hpp"
#include "ranked_belief/operations/nrm_exc.hpp"
#include "ranked_belief/operations/observe.hpp"
#include "ranked_belief/promise.hpp"
#include "ranked_belief/rank.hpp"
#include "ranked_belief/ranking_element.hpp"

#include <array>
#include <functional>
#include <initializer_list>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace {

using ranked_belief::Rank;
using ranked_belief::RankingElement;
using ranked_belief::RankingFunction;

/// Scenario describing a full Monty Hall play-through.
struct MontyHallScenario {
    int prize;
    int pick;
    int host;

    [[maybe_unused]] friend bool operator==(const MontyHallScenario&, const MontyHallScenario&) = default;
};

/// Helper to materialise door labels once.
[[nodiscard]] const std::vector<int>& all_doors() {
    static const std::vector<int> doors{0, 1, 2};
    return doors;
}

} // namespace

TEST(IntegrationTest, MontyHallSwitchingMorePlausibleThanStaying) {
    const auto& doors = all_doors();

    auto prize_distribution = ranked_belief::from_values_uniform(doors, Rank::zero(), false);
    auto initial_pick_distribution = ranked_belief::from_values_uniform(doors, Rank::zero(), false);

    struct WorldState {
        int prize;
        int pick;
    };

    auto world_states = ranked_belief::merge_apply(
        prize_distribution,
        [initial_pick_distribution](int prize) {
            return ranked_belief::merge_apply(
                initial_pick_distribution,
                [prize](int pick) {
                    return ranked_belief::make_singleton_ranking(WorldState{prize, pick});
                },
                false);
        },
        false);

    auto plays = ranked_belief::merge_apply(
        world_states,
        [](const WorldState& world) {
            std::vector<int> candidate_hosts;
            candidate_hosts.reserve(2);
            for (int door : all_doors()) {
                if (door != world.prize && door != world.pick) {
                    candidate_hosts.push_back(door);
                }
            }

            const Rank host_rank = candidate_hosts.size() == 1 ? Rank::zero() : Rank::from_value(1);
            std::vector<std::pair<MontyHallScenario, Rank>> scored_scenarios;
            scored_scenarios.reserve(candidate_hosts.size());
            for (int host : candidate_hosts) {
                scored_scenarios.emplace_back(MontyHallScenario{world.prize, world.pick, host}, host_rank);
            }

            return ranked_belief::from_list(scored_scenarios, false);
        },
        false);

    auto observed = ranked_belief::observe(
        plays,
        [](const MontyHallScenario& scenario) { return scenario.host == 1; },
        false);

    auto stay_evaluations = std::make_shared<int>(0);
    auto stay_results = ranked_belief::map(
        observed,
        [stay_evaluations](const MontyHallScenario& scenario) {
            ++(*stay_evaluations);
            return scenario.pick == scenario.prize;
        },
        false);

    auto switch_evaluations = std::make_shared<int>(0);
    auto switch_results = ranked_belief::map(
        observed,
        [switch_evaluations](const MontyHallScenario& scenario) {
            ++(*switch_evaluations);
            int alternative = -1;
            for (int door : all_doors()) {
                if (door != scenario.pick && door != scenario.host) {
                    alternative = door;
                    break;
                }
            }
            return alternative == scenario.prize;
        },
        false);

    EXPECT_EQ(*stay_evaluations, 0);
    EXPECT_EQ(*switch_evaluations, 0);

    const auto stay_pairs = ranked_belief::take_n(stay_results, 4);
    const auto switch_pairs = ranked_belief::take_n(switch_results, 4);

    EXPECT_EQ(stay_pairs.size(), 4);
    EXPECT_EQ(switch_pairs.size(), 4);

    EXPECT_EQ(*stay_evaluations, static_cast<int>(stay_pairs.size()));
    EXPECT_EQ(*switch_evaluations, static_cast<int>(switch_pairs.size()));

    const auto min_rank_for = [](const std::vector<std::pair<bool, Rank>>& entries, bool target)
        -> std::optional<Rank> {
        std::optional<Rank> best;
        for (const auto& [value, rank] : entries) {
            if (value != target) {
                continue;
            }
            if (!best || rank < *best) {
                best = rank;
            }
        }
        return best;
    };

    const auto stay_true_rank = min_rank_for(stay_pairs, true);
    const auto stay_false_rank = min_rank_for(stay_pairs, false);
    const auto switch_true_rank = min_rank_for(switch_pairs, true);
    const auto switch_false_rank = min_rank_for(switch_pairs, false);

    ASSERT_TRUE(stay_true_rank.has_value());
    ASSERT_TRUE(stay_false_rank.has_value());
    ASSERT_TRUE(switch_true_rank.has_value());
    ASSERT_TRUE(switch_false_rank.has_value());

    EXPECT_GT(*stay_true_rank, *stay_false_rank);
    EXPECT_LT(*switch_true_rank, *switch_false_rank);
}

TEST(IntegrationTest, DiceSumDistributionMatchesCombinatorics) {
    auto value_forces = std::make_shared<int>(0);
    auto next_forces = std::make_shared<int>(0);

    using Element = RankingElement<int>;
    using Builder = std::function<std::shared_ptr<Element>(int)>;

    auto builder = std::make_shared<Builder>();
    std::weak_ptr<Builder> weak_builder = builder;
    *builder = [weak_builder, value_forces, next_forces](int face) -> std::shared_ptr<Element> {
        if (face > 6) {
            return nullptr;
        }

        auto value_promise = ranked_belief::make_promise([value_forces, face]() {
            ++(*value_forces);
            return face;
        });

        auto compute_next = [weak_builder, next_forces, face]() -> std::shared_ptr<Element> {
            ++(*next_forces);
            if (auto locked = weak_builder.lock()) {
                return (*locked)(face + 1);
            }
            return nullptr;
        };

        auto next_promise = ranked_belief::make_promise(std::move(compute_next));
        return std::make_shared<Element>(std::move(value_promise), Rank::zero(), std::move(next_promise));
    };

    auto die = RankingFunction<int>((*builder)(1), false);

    EXPECT_EQ(*value_forces, 0);
    EXPECT_EQ(*next_forces, 0);

    auto sum_distribution = ranked_belief::merge_apply(
        die,
        [die](int first) {
            return ranked_belief::map(
                die,
                [first](int second) { return first + second; },
                false);
        },
        false);

    const auto samples = ranked_belief::take_n(sum_distribution, 36);
    ASSERT_EQ(samples.size(), 36);

    EXPECT_EQ(*value_forces, 6);
    EXPECT_EQ(*next_forces, 6);

    std::array<int, 13> counts{};
    for (const auto& [sum, rank] : samples) {
        EXPECT_EQ(rank, Rank::zero());
        ++counts[sum];
    }

    const std::array<int, 13> expected = [] {
        std::array<int, 13> distribution{};
        distribution[2] = 1;
        distribution[3] = 2;
        distribution[4] = 3;
        distribution[5] = 4;
        distribution[6] = 5;
        distribution[7] = 6;
        distribution[8] = 5;
        distribution[9] = 4;
        distribution[10] = 3;
        distribution[11] = 2;
        distribution[12] = 1;
        return distribution;
    }();

    for (int sum = 2; sum <= 12; ++sum) {
        EXPECT_EQ(counts[sum], expected[sum]) << "Mismatch for sum=" << sum;
    }
}

TEST(IntegrationTest, FibonacciSequenceRespectsLazyGeneration) {
    auto generator_calls = std::make_shared<int>(0);

    auto cache = std::make_shared<std::vector<long long>>(std::initializer_list<long long>{0, 1});

    auto fibonacci = ranked_belief::from_generator<int>(
        [generator_calls, cache](std::size_t index) {
            ++(*generator_calls);
            if (index >= cache->size()) {
                cache->push_back(cache->at(cache->size() - 1) + cache->at(cache->size() - 2));
            }
            return std::make_pair(static_cast<int>(cache->at(index)), Rank::from_value(index));
        },
        0,
        false);

    EXPECT_EQ(*generator_calls, 1);

    const auto first_value = fibonacci.first();
    ASSERT_TRUE(first_value.has_value());
    EXPECT_EQ(first_value->first, 0);
    EXPECT_EQ(first_value->second, Rank::zero());
    EXPECT_EQ(*generator_calls, 1);

    const auto first_seven = ranked_belief::take_n(fibonacci, 7);
    ASSERT_EQ(first_seven.size(), 7);
    EXPECT_EQ(*generator_calls, 8); // Includes sentinel lookahead for the seventh element

    const std::array<int, 7> expected_values{0, 1, 1, 2, 3, 5, 8};
    for (std::size_t i = 0; i < expected_values.size(); ++i) {
        EXPECT_EQ(first_seven[i].first, expected_values[i]);
        EXPECT_EQ(first_seven[i].second, Rank::from_value(i));
        if (i >= 2) {
            EXPECT_EQ(first_seven[i].first, first_seven[i - 1].first + first_seven[i - 2].first);
        }
    }

    const auto first_ten = ranked_belief::take_n(fibonacci, 10);
    ASSERT_EQ(first_ten.size(), 10);
    EXPECT_EQ(*generator_calls, 11); // Includes sentinel lookahead for the tenth element
    EXPECT_EQ(first_ten[9].first, first_ten[8].first + first_ten[7].first);
    EXPECT_EQ(first_ten[9].second, Rank::from_value(9));
}
