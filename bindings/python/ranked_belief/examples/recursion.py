"""Recursive ranking example illustrating infinite lazy structures."""

from __future__ import annotations

from typing import List, Tuple

from .. import Rank, RankingFunctionAny
from ..dsl import normal_exceptional, observe, take_n

__all__ = ["recursive_fun", "first_values", "values_greater_than"]


def recursive_fun(value: int) -> RankingFunctionAny:
    """Port of the Racket ``fun`` definition with full laziness."""

    def exceptional_branch() -> RankingFunctionAny:
        return recursive_fun(value * 2)

    return normal_exceptional(value, exceptional_branch)


def first_values(count: int = 10) -> List[Tuple[int, Rank]]:
    """Materialise the first ``count`` values of ``recursive_fun(1)``."""

    ranking = recursive_fun(1)
    return take_n(ranking, count)


def values_greater_than(threshold: int, *, limit: int = 5) -> List[Tuple[int, Rank]]:
    """Observe ``recursive_fun(1)`` with a predicate and return the top results."""

    ranking = recursive_fun(1)
    posterior = observe(ranking, lambda value: value > threshold)
    return take_n(posterior, limit)
