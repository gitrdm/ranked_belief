"""Ranked application examples mirroring the original Racket snippets."""

from __future__ import annotations

import operator

from .. import Rank, RankingFunctionAny
from ..dsl import normal_exceptional, observe, ranked_apply, take_n

__all__ = [
    "deterministic_sum",
    "uncertain_argument_sum",
    "uncertain_operator_sum",
]


def deterministic_sum() -> RankingFunctionAny:
    """Compute 10 + 5 without uncertainty.
    
    Returns:
        A ranking with a single value: 15 at rank 0.
    """
    return ranked_apply(operator.add, 10, 5)


def uncertain_argument_sum() -> list[tuple[int, Rank]]:
    """Return the ranking when the first addend is uncertain.
    
    Returns:
        Ranked sums where the first argument is normally 10 or exceptionally 20.
        Results: [(15, rank=0), (25, rank=1)].
    """
    ranking = ranked_apply(operator.add, normal_exceptional(10, 20), 5)
    return take_n(ranking, 4)


def uncertain_operator_sum() -> list[tuple[int, Rank]]:
    """Apply an uncertain operator (addition or subtraction).
    
    Returns:
        Filtered ranking of results where both operator and operand are uncertain.
        Only returns values in {15, -5}.
    """
    operator_ranking = normal_exceptional(operator.add, operator.sub)
    argument_ranking = normal_exceptional(10, 20)
    ranking = ranked_apply(operator_ranking, argument_ranking, 5)
    filtered = observe(ranking, lambda value: value in {15, -5})
    return take_n(filtered, 6)
