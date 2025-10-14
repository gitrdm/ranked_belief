"""Recursive ranking example illustrating infinite lazy structures."""

from __future__ import annotations

from .. import Rank, RankingFunctionAny
from ..dsl import normal_exceptional, observe, take_n

__all__ = ["recursive_fun", "first_values", "values_greater_than"]


def recursive_fun(value: int) -> RankingFunctionAny:
    """Port of the Racket ``fun`` definition with full laziness.
    
    Creates an infinite ranking where each level is twice the previous:
    1@0, 2@1, 4@2, 8@3, 16@4, ...
    
    Args:
        value: The starting value for this level of recursion.
        
    Returns:
        A ranking with the current value normally, and recursively
        doubled values exceptionally.
    """
    def exceptional_branch() -> RankingFunctionAny:
        """Lazily compute the exceptional (doubled) branch."""
        return recursive_fun(value * 2)

    return normal_exceptional(value, exceptional_branch)


def first_values(count: int = 10) -> list[tuple[int, Rank]]:
    """Materialise the first ``count`` values of ``recursive_fun(1)``.
    
    Args:
        count: Number of values to retrieve from the infinite ranking.
        
    Returns:
        List of (value, rank) tuples: [(1, 0), (2, 1), (4, 2), ...].
    """
    ranking = recursive_fun(1)
    return take_n(ranking, count)


def values_greater_than(threshold: int, *, limit: int = 5) -> list[tuple[int, Rank]]:
    """Observe ``recursive_fun(1)`` with a predicate and return the top results.
    
    Args:
        threshold: Only return values greater than this.
        limit: Maximum number of results to return.
        
    Returns:
        List of (value, rank) tuples where value > threshold, renormalized
        so the smallest satisfying value has rank 0.
    """
    ranking = recursive_fun(1)
    posterior = observe(ranking, lambda value: value > threshold)
    return take_n(posterior, limit)
