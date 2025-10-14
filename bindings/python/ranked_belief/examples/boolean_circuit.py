"""Boolean circuit diagnosis example using the Python bindings."""

from __future__ import annotations

from typing import Any, Iterable

from .. import Rank, RankingFunctionAny
from ..dsl import normal_exceptional, observe, rlet_star

__all__ = ["circuit", "diagnose", "format_scenarios"]


# Gate behavior: normally active (True), exceptionally fails (False)
GATE_BEHAVIOUR = normal_exceptional(True, False, deduplicate=False)


def circuit(input1: bool, input2: bool, input3: bool) -> RankingFunctionAny:
    """Compose the three-gate boolean circuit from the original paper.
    
    Each gate can either work normally (True) or fail (False) with higher rank.
    The circuit computes: output = ((¬input1 OR input2) OR input3) when all gates work.
    
    Args:
        input1: First circuit input
        input2: Second circuit input
        input3: Third circuit input
        
    Returns:
        A ranking function over circuit outcomes showing gate failure scenarios.
    """
    
    def compute_circuit(bindings: dict[str, Any]) -> dict[str, Any]:
        """Compute circuit output given gate states."""
        n_gate = bindings["N"]
        o1_gate = bindings["O1"]
        o2_gate = bindings["O2"]
        
        # Compute intermediate values and output
        l1 = (not input1) if n_gate else True
        l2 = (l1 or input2) if o1_gate else False
        output = (l2 or input3) if o2_gate else False
        
        return {
            "output": output,
            "N": n_gate,
            "O1": o1_gate,
            "O2": o2_gate,
        }
    
    # Use rlet_star for clean sequential binding of gate behaviors
    return rlet_star(
        [
            ("N", lambda _env: GATE_BEHAVIOUR),
            ("O1", lambda _env: GATE_BEHAVIOUR),
            ("O2", lambda _env: GATE_BEHAVIOUR),
        ],
        compute_circuit,
    )


def diagnose(
    input1: bool,
    input2: bool,
    input3: bool,
    observed_output: bool,
    *,
    limit: int = 6,
) -> list[tuple[dict[str, bool], Rank]]:
    """Enumerate the most plausible gate failures explaining the observed output.
    
    Args:
        input1: First circuit input
        input2: Second circuit input
        input3: Third circuit input
        observed_output: The output value that was actually observed
        limit: Maximum number of scenarios to return
        
    Returns:
        List of (scenario, rank) tuples ordered by increasing rank.
    """
    posterior = observe(
        circuit(input1, input2, input3),
        lambda scenario: scenario["output"] == observed_output,
    )
    return [(scenario, rank) for scenario, rank in posterior.take_n(limit)]


def format_scenarios(scenarios: Iterable[tuple[dict[str, bool], Rank]]) -> str:
    """Render circuit scenarios in a human-readable format.
    
    Args:
        scenarios: Iterable of (scenario_dict, rank) tuples
        
    Returns:
        Multi-line string with numbered scenarios.
    """
    lines = []
    for index, (scenario, rank) in enumerate(scenarios, start=1):
        rank_str = str(int(rank)) if rank.is_finite() else "∞"
        lines.append(
            f"{index:>2}. rank={rank_str} "
            f"output={scenario['output']} "
            f"N={scenario['N']} O1={scenario['O1']} O2={scenario['O2']}"
        )
    return "\n".join(lines)


if __name__ == "__main__":  # pragma: no cover
    results = diagnose(False, False, True, False)
    print("Top-ranked explanations for output=False:")
    print(format_scenarios(results))
