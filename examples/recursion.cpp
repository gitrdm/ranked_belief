#include "ranked_belief/operations/merge.hpp"
#include "ranked_belief/operations/merge_apply.hpp"
#include "ranked_belief/operations/nrm_exc.hpp"
#include "ranked_belief/operations/observe.hpp"
#include "ranked_belief/rank.hpp"
#include "ranked_belief/ranking_element.hpp"
#include "ranked_belief/ranking_function.hpp"

#include <iomanip>
#include <iostream>
#include <memory>
#include <utility>
#include <vector>

namespace rb = ranked_belief;

/**
 * @brief Build the lazy ranking used in the recursive example.
 *
 * The function mirrors the Racket definition
 * @code
 * (define (fun x) (nrm/exc x (fun (* x 2))))
 * @endcode
 * and therefore yields an infinite sequence where each exceptional branch is
 * twice the preceding value and one rank less plausible. The recursive call is
 * wrapped in a promise so that additional values are generated only when the
 * caller forces deeper elements of the ranking.
 */
[[nodiscard]] rb::RankingFunction<int> recursive_fun(
    int value,
    const std::shared_ptr<int>& forced_tails)
{
    auto tail_builder = [value, forced_tails]() -> std::shared_ptr<rb::RankingElement<int>> {
        ++(*forced_tails);
        auto shifted_tail = rb::shift_ranks(
            recursive_fun(value * 2, forced_tails),
            rb::Rank::from_value(1));
        return shifted_tail.head();
    };

    auto head = rb::make_lazy_element<int>(
        value,
        rb::Rank::zero(),
        std::move(tail_builder));

    return rb::RankingFunction<int>(std::move(head));
}

/**
 * @brief Pretty-print a sequence of ranked integers.
 */
void dump_prefix(const std::vector<std::pair<int, rb::Rank>>& entries, std::size_t limit) {
    for (std::size_t index = 0; index < entries.size() && index < limit; ++index) {
        const auto& [value, rank] = entries[index];
        std::cout << "  " << (index + 1) << ". value=" << value << ", rank=" << rank << '\n';
    }
}

/**
 * @brief Execution showcase for the recursive ranked function.
 */
int main() {
    std::cout << std::boolalpha;

    auto primary_forces = std::make_shared<int>(0);
    auto ranking = recursive_fun(1, primary_forces);

    const auto prefix = rb::take_n(ranking, 10);
    std::cout << "First ten outcomes of recursive_fun(1):\n";
    dump_prefix(prefix, prefix.size());
    std::cout << "Lazy tail expansions forced: " << *primary_forces << "\n\n";

    auto observed_forces = std::make_shared<int>(0);
    auto ranking_for_observe = recursive_fun(1, observed_forces);
    const auto observed = rb::observe(
        ranking_for_observe,
        [](int v) { return v > 100; },
        Deduplication::Disabled);
    const auto observed_prefix = rb::take_n(observed, 5);

    std::cout << "Values greater than 100 (first five):\n";
    dump_prefix(observed_prefix, observed_prefix.size());
    std::cout << "Lazy tail expansions forced while observing: "
              << *observed_forces << '\n';

    return 0;
}
