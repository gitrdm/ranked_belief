"""Ranked let binding example translated from the Racket code."""

from __future__ import annotations

from collections.abc import Mapping

from .. import Rank, RankingFunctionAny
from ..dsl import normal_exceptional, rlet_star, take_n

__all__ = ["consumption_preferences"]


def consumption_preferences() -> RankingFunctionAny:
    """Model the mutually dependent beer/peanut preferences.
    
    Returns:
        A ranking function over preference combinations ordered by plausibility.
        Each value is a dict with 'beer' and 'peanuts' boolean preferences.
    """

    def body(env: dict[str, bool]) -> dict[str, bool]:
        """Extract the final preference combination."""
        return {"beer": env["b"], "peanuts": env["p"]}

    def bind_b(_env: Mapping[str, bool]) -> RankingFunctionAny:
        """Beer preference: normally False, exceptionally True."""
        return normal_exceptional(False, True)

    def bind_p(env: Mapping[str, bool]) -> RankingFunctionAny:
        """Peanut preference: depends on beer choice."""
        if env["b"]:
            return normal_exceptional(True, False)
        return RankingFunctionAny.singleton(False)

    return rlet_star(
        [
            ("b", bind_b),
            ("p", bind_p),
        ],
        body,
    )


def materialised_preferences(limit: int = 6) -> list[tuple[dict[str, bool], Rank]]:
    """Return the most plausible preference combinations.
    
    Args:
        limit: Maximum number of combinations to return.
        
    Returns:
        List of (preference_dict, rank) tuples ordered by plausibility.
    """
    return take_n(consumption_preferences(), limit)
