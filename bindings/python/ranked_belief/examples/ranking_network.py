"""Ranking network example translated from Racket."""

from __future__ import annotations

from typing import Dict, List, Mapping, Tuple

from .. import Rank, RankingFunctionAny
from ..dsl import normal_exceptional, observe, rlet_star, take_n

__all__ = ["network", "sensor_given_fire"]


def network() -> RankingFunctionAny:
    """Construct the ranking network with hazard (H), burglary (B), fire (F), and sensor (S)."""

    def body(env: Dict[str, bool]) -> Dict[str, bool]:
        return env

    def bind_h(_env: Mapping[str, bool]) -> RankingFunctionAny:
        return normal_exceptional(False, True, Rank.from_value(15))

    def bind_b(env: Mapping[str, bool]) -> RankingFunctionAny:
        if env["H"]:
            return normal_exceptional(False, True, Rank.from_value(4))
        return normal_exceptional(True, False, Rank.from_value(8))

    def bind_f(_env: Mapping[str, bool]) -> RankingFunctionAny:
        return normal_exceptional(True, False, Rank.from_value(10))

    def bind_s(env: Mapping[str, bool]) -> RankingFunctionAny:
        b, f = env["B"], env["F"]
        if b and f:
            return normal_exceptional(True, False, Rank.from_value(3))
        if b and not f:
            return normal_exceptional(False, True, Rank.from_value(13))
        if not b and f:
            return normal_exceptional(False, True, Rank.from_value(11))
        return normal_exceptional(False, True, Rank.from_value(27))

    return rlet_star(
        [
            ("H", bind_h),
            ("B", bind_b),
            ("F", bind_f),
            ("S", bind_s),
        ],
        body,
    )


def sensor_given_fire(limit: int = 8) -> List[Tuple[bool, Rank]]:
    """Return the ranking over sensor outcomes conditioned on fire being true."""

    posterior = observe(network(), lambda env: env["F"])
    sensor_only = posterior.map(lambda env: env["S"])
    return take_n(sensor_only, limit)
