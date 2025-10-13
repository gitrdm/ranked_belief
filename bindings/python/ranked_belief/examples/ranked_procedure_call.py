"""Ranked application examples mirroring the original Racket snippets."""

from __future__ import annotations

import operator
from typing import List, Tuple

from .. import Rank, RankingFunctionAny
from ..dsl import normal_exceptional, ranked_apply, take_n

__all__ = [
    "deterministic_sum",
    "uncertain_argument_sum",
    "uncertain_operator_sum",
]


def deterministic_sum() -> RankingFunctionAny:
    """Compute 10 + 5 without uncertainty."""

    return ranked_apply(operator.add, 10, 5)


def uncertain_argument_sum() -> List[Tuple[int, Rank]]:
    """Return the ranking when the first addend is uncertain."""

    ranking = ranked_apply(operator.add, normal_exceptional(10, 20), 5)
    return take_n(ranking, 4)


def uncertain_operator_sum() -> List[Tuple[int, Rank]]:
    """Apply an uncertain operator (addition or subtraction)."""

    operator_ranking = normal_exceptional(operator.add, operator.sub)
    argument_ranking = normal_exceptional(10, -10)
    ranking = ranked_apply(operator_ranking, 5, argument_ranking)
    return take_n(ranking, 4)
