"""Boolean circuit diagnosis example using the Python bindings."""

from __future__ import annotations

from typing import Dict, Iterable, List, Tuple

from .. import Rank, RankingFunctionAny
from ..dsl import merge_apply, normal_exceptional, observe

__all__ = ["circuit", "diagnose", "format_scenarios"]


def _gate_behaviour() -> RankingFunctionAny:
    """Ranking describing a gate that normally passes and exceptionally fails."""

    return normal_exceptional(True, False, deduplicate=False)


def circuit(input1: bool, input2: bool, input3: bool) -> RankingFunctionAny:
    """Compose the three-gate boolean circuit from the original paper."""

    def bind_n_gate(n_gate: bool) -> RankingFunctionAny:
        l1 = (not input1) if n_gate else True

        def bind_o1_gate(o1_gate: bool) -> RankingFunctionAny:
            l2 = (l1 or input2) if o1_gate else False

            def bind_o2_gate(o2_gate: bool) -> RankingFunctionAny:
                output = (l2 or input3) if o2_gate else False
                scenario: Dict[str, bool] = {
                    "output": output,
                    "N": bool(n_gate),
                    "O1": bool(o1_gate),
                    "O2": bool(o2_gate),
                }
                return RankingFunctionAny.singleton(scenario)

            return merge_apply(_gate_behaviour(), bind_o2_gate, deduplicate=False)

        return merge_apply(_gate_behaviour(), bind_o1_gate, deduplicate=False)

    return merge_apply(_gate_behaviour(), bind_n_gate, deduplicate=False)


def diagnose(
    input1: bool,
    input2: bool,
    input3: bool,
    observed_output: bool,
    *,
    limit: int = 6,
) -> List[Tuple[Dict[str, bool], Rank]]:
    """Enumerate the most plausible gate failures explaining ``observed_output``."""

    posterior = observe(
        circuit(input1, input2, input3),
        lambda scenario: scenario["output"] == observed_output,
        deduplicate=False,
    )
    return [(scenario, rank) for scenario, rank in posterior.take_n(limit)]


def format_scenarios(scenarios: Iterable[Tuple[Dict[str, bool], Rank]]) -> str:
    """Nicely render a list of ranked circuit scenarios."""

    lines = []
    for index, (scenario, rank) in enumerate(scenarios, start=1):
        lines.append(
            f"{index:>2}. rank={int(rank) if rank.is_finite() else '∞'} "
            f"output={scenario['output']} N={scenario['N']} O1={scenario['O1']} O2={scenario['O2']}"
        )
    return "\n".join(lines)


if __name__ == "__main__":  # pragma: no cover - example driver
    results = diagnose(False, False, True, False)
    print("Top-ranked explanations for output=False:")
    print(format_scenarios(results))
