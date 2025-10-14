#include "ranked_belief/constructors.hpp"
#include "ranked_belief/operations/merge_apply.hpp"
#include "ranked_belief/operations/nrm_exc.hpp"
#include "ranked_belief/operations/observe.hpp"
#include "ranked_belief/rank.hpp"
#include "ranked_belief/ranking_function.hpp"

#include <iomanip>
#include <iostream>
#include <tuple>
#include <vector>

namespace rb = ranked_belief;

/**
 * @brief Captures a single explanation for the boolean circuit fault analysis.
 *
 * The circuit contains three boolean gates (N, O1, O2) whose unusual behaviour
 * can explain a surprising output. Each gate is normally active and only
 * occasionally fails. When a gate is inactive it short-circuits to @c false.
 */
struct CircuitOutcome {
    bool output;
    bool n_gate;
    bool o1_gate;
    bool o2_gate;

    friend bool operator==(const CircuitOutcome&, const CircuitOutcome&) = default;
};

/**
 * @brief Format a circuit outcome for human-readable logging.
 */
std::ostream& operator<<(std::ostream& os, const CircuitOutcome& outcome) {
    os << "out=" << std::boolalpha << outcome.output
       << " (N=" << outcome.n_gate
       << ", O1=" << outcome.o1_gate
       << ", O2=" << outcome.o2_gate << ")";
    return os;
}

/**
 * @brief Create a boolean ranking with a normal and exceptional value.
 *
 * This mirrors the Racket @c nrm/exc helper: the first value receives rank 0
 * (completely normal) while the second value receives rank 1 (slightly
 * exceptional). Deduplication is disabled so that subsequent operations can
 * observe both outcomes independently.
 */
[[nodiscard]] rb::RankingFunction<bool> normal_exceptional(
    bool normal_value,
    bool exceptional_value,
    rb::Rank exceptional_rank = rb::Rank::from_value(1))
{
    return rb::from_list<bool>({
        {normal_value, rb::Rank::zero()},
        {exceptional_value, exceptional_rank}
    },
    Deduplication::Disabled);
}

/**
 * @brief Enumerate circuit outcomes conditioned on three boolean inputs.
 *
 * The circuit matches the ranked-programming paper example. Each internal gate
 * either behaves normally (passing its intended logic) or—at a slightly higher
 * rank—fails closed, propagating @c false. The resulting ranking is produced via
 * nested lazy merge-apply calls so that only the required combinations are
 * generated when observations are imposed downstream.
 */
[[nodiscard]] rb::RankingFunction<CircuitOutcome> circuit(
    bool input1,
    bool input2,
    bool input3)
{
    return rb::merge_apply(
        normal_exceptional(true, false),
        [=](bool n_gate) {
            const bool l1 = n_gate ? (!input1) : false;

            return rb::merge_apply(
                normal_exceptional(true, false),
                [=](bool o1_gate) {
                    const bool l2 = o1_gate ? (l1 || input2) : false;

                    return rb::merge_apply(
                        normal_exceptional(true, false),
                        [=](bool o2_gate) {
                            const bool output = o2_gate ? (l2 || input3) : false;
                            return rb::make_singleton_ranking(
                                CircuitOutcome{output, n_gate, o1_gate, o2_gate});
                        },
                        Deduplication::Disabled);
                },
                Deduplication::Disabled);
        },
        Deduplication::Disabled);
}

/**
 * @brief Entry point showcasing boolean circuit diagnosis with ranked belief.
 */
int main() {
    std::cout << std::boolalpha;

    const bool input1 = false;
    const bool input2 = false;
    const bool input3 = true;
    const bool observed_output = false;

    const auto prior = circuit(input1, input2, input3);
    const auto posterior = rb::observe(
        prior,
        [observed_output](const CircuitOutcome& outcome) {
            return outcome.output == observed_output;
        },
        Deduplication::Disabled);

    const auto explanations = rb::take_n(posterior, 6);

    std::cout << "Top-ranked explanations for output=" << observed_output << "\n";
    for (std::size_t index = 0; index < explanations.size(); ++index) {
        const auto& [scenario, rank] = explanations[index];
        std::cout << "  " << (index + 1) << ". rank=" << rank
                  << " -> " << scenario << '\n';
    }

    if (explanations.empty()) {
        std::cout << "No explanations within finite rank." << std::endl;
    }

    return 0;
}
