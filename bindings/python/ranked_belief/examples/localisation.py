"""Robot localisation example adapted from the ranked-programming package."""

from __future__ import annotations

from typing import Iterable, List, Sequence, Tuple

from .. import Rank, RankingFunctionAny
from ..dsl import either_of, merge_apply, normal_exceptional, observe, take_n

__all__ = ["hmm", "path_likelihoods"]

Coordinate = Tuple[int, int]
Path = Tuple[Tuple[Coordinate, Coordinate | None], ...]


def _neighbour_positions(position: Coordinate) -> Tuple[Coordinate, ...]:
    x, y = position
    return ((x - 1, y), (x + 1, y), (x, y - 1), (x, y + 1))


def _surrounding_positions(position: Coordinate) -> Tuple[Coordinate, ...]:
    x, y = position
    return (
        (x - 1, y - 1),
        (x, y - 1),
        (x + 1, y - 1),
        (x + 1, y),
        (x + 1, y + 1),
        (x, y + 1),
        (x - 1, y + 1),
        (x - 1, y),
    )


def init() -> RankingFunctionAny:
    return RankingFunctionAny.singleton((0, 0))


def next_state(position: Coordinate) -> RankingFunctionAny:
    return either_of(_neighbour_positions(position))


def observable(position: Coordinate) -> RankingFunctionAny:
    return normal_exceptional(position, either_of(_surrounding_positions(position)))


def _prepend(path: Path, state: Coordinate, observation: Coordinate) -> RankingFunctionAny:
    updated = path + ((state, observation),)
    return RankingFunctionAny.singleton(updated)


def _advance(path: Path, observation: Coordinate) -> RankingFunctionAny:
    current = path[-1][0]

    def bind_state(next_state: Coordinate) -> RankingFunctionAny:
        consistent = observe(
            observable(next_state),
            lambda seen: seen == observation,
        )
        return merge_apply(
            consistent,
            lambda _matched: _prepend(path, next_state, observation),
        )

    return merge_apply(next_state(current), bind_state)


def hmm(observations: Sequence[Coordinate]) -> RankingFunctionAny:
    def initialise(state: Coordinate) -> RankingFunctionAny:
        start: Path = ((state, None),)
        return RankingFunctionAny.singleton(start)

    ranking = merge_apply(init(), initialise)
    for obs in observations:
        ranking = merge_apply(ranking, lambda path, obs=obs: _advance(path, obs))
    return ranking


def path_likelihoods(
    observations: Iterable[Coordinate], *, limit: int = 5
) -> List[Tuple[Path, Rank]]:
    return take_n(hmm(tuple(observations)), limit)
