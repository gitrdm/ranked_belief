"""Ranked let binding example translated from the Racket code."""

from __future__ import annotations

from typing import Dict, List, Mapping, Tuple

from .. import Rank, RankingFunctionAny
from ..dsl import normal_exceptional, rlet_star, take_n

__all__ = ["consumption_preferences"]


def consumption_preferences() -> RankingFunctionAny:
    """Model the mutually dependent beer/peanut preferences."""

    def body(env: Dict[str, bool]) -> Dict[str, bool]:
        return {"beer": env["b"], "peanuts": env["p"]}

    def bind_b(_env: Mapping[str, bool]) -> RankingFunctionAny:
        return normal_exceptional(False, True)

    def bind_p(env: Mapping[str, bool]) -> RankingFunctionAny:
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


def materialised_preferences(limit: int = 6) -> List[Tuple[Dict[str, bool], Rank]]:
    """Convenience helper returning the most plausible preference combinations."""

    return take_n(consumption_preferences(), limit)
