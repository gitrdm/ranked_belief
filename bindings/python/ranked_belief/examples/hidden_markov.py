"""Hidden Markov model example expressed with the Python bindings."""

from __future__ import annotations

from collections.abc import Iterable, Sequence
from functools import lru_cache

from .. import Rank, RankingFunctionAny
from ..dsl import either_or, merge_apply, normal_exceptional, observe, take_n

__all__ = ["hmm", "sequence_likelihoods"]

State = str
Observation = str
Path = tuple[tuple[State, Observation | None], ...]


@lru_cache(maxsize=None)
def init() -> RankingFunctionAny:
    """Initial distribution over weather states."""

    return either_or("rainy", "sunny")


def trans(state: State) -> RankingFunctionAny:
    """Transition model (probabilistic ranking)."""

    if state == "rainy":
        return normal_exceptional("rainy", "sunny", Rank.from_value(2))
    return normal_exceptional("sunny", "rainy", Rank.from_value(2))


def emit(state: State) -> RankingFunctionAny:
    """Emission model linking latent state to umbrella observation."""

    if state == "rainy":
        return normal_exceptional("yes", "no", Rank.from_value(1))
    return normal_exceptional("no", "yes", Rank.from_value(1))


def _prepend_step(path: Path, state: State, observation: Observation) -> RankingFunctionAny:
    new_path = path + ((state, observation),)
    return RankingFunctionAny.singleton(new_path)


def _advance(path: Path, observation: Observation) -> RankingFunctionAny:
    last_state = path[-1][0]

    def bind_state(next_state: State) -> RankingFunctionAny:
        consistent_emissions = observe(
            emit(next_state),
            lambda value: value == observation,
        )
        return merge_apply(
            consistent_emissions,
            lambda _matched: _prepend_step(path, next_state, observation),
        )

    return merge_apply(trans(last_state), bind_state)


def hmm(observations: Sequence[Observation]) -> RankingFunctionAny:
    """Compute the ranking over state/observation paths matching ``observations``."""

    def initialise(state: State) -> RankingFunctionAny:
        start: Path = ((state, None),)
        return RankingFunctionAny.singleton(start)

    ranking = merge_apply(init(), initialise)
    for obs in observations:
        ranking = merge_apply(ranking, lambda path, obs=obs: _advance(path, obs))
    return ranking.map(lambda path: tuple(path[1:]))


def sequence_likelihoods(observations: Iterable[Observation], *, limit: int = 5) -> list[tuple[Path, Rank]]:
    """Return the most plausible latent state sequences for ``observations``."""

    return take_n(hmm(tuple(observations)), limit)
