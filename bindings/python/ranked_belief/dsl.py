"""Convenience helpers mirroring the ranked-programming DSL in Python.

The utilities in this module build on :class:`ranked_belief.RankingFunctionAny`
so that the original Racket examples can be expressed idiomatically in
Python while retaining full lazy semantics.
"""

from __future__ import annotations

from typing import Any, Callable, Dict, Iterable, Iterator, Mapping, Sequence

from . import Rank, RankingFunctionAny

__all__ = [
    "ensure_ranking",
    "normal_exceptional",
    "either_of",
    "either_or",
    "merge_apply",
    "observe",
    "observe_value",
    "ranked_apply",
    "rlet_star",
    "take_n",
]


def _coerce_rank(value: Rank | int) -> Rank:
    """Coerce integers into :class:`Rank` instances."""

    if isinstance(value, Rank):
        return value
    if isinstance(value, bool):
        # Prevent accidental bool-to-int coercion from flipping the meaning.
        return Rank.from_value(int(value))
    return Rank.from_value(int(value))


def ensure_ranking(value: Any) -> RankingFunctionAny:
    """Autocast values or callables into :class:`RankingFunctionAny`.

    * Plain Python values become singleton rankings at rank 0.
    * Callables are wrapped lazily so that their body is only evaluated if the
      resulting ranking is forced.
    * Existing rankings are returned unchanged.
    """

    if isinstance(value, RankingFunctionAny):
        return value
    return RankingFunctionAny.singleton(value)


def normal_exceptional(
    normal_value: Any,
    exceptional_value: Any,
    exceptional_rank: Rank | int = Rank.from_value(1),
    *,
    deduplicate: bool = False,
) -> RankingFunctionAny:
    """Mirror the ``nrm/exc`` constructor from Racket."""

    offset = _coerce_rank(exceptional_rank)
    normal_rf = ensure_ranking(normal_value)
    exceptional_rf = ensure_ranking(exceptional_value)
    shifted = exceptional_rf.map_with_rank(
        lambda value, rank: (value, rank + offset),
        deduplicate=False,
    )
    return normal_rf.merge(shifted, deduplicate=deduplicate)


def either_of(values: Iterable[Any], *, deduplicate: bool = False) -> RankingFunctionAny:
    """Return a ranking that normally selects any value from ``values``."""

    pairs = [(value, Rank.zero()) for value in values]
    return RankingFunctionAny.from_list(pairs, deduplicate=deduplicate)


def either_or(*values: Any, deduplicate: bool = False) -> RankingFunctionAny:
    """Merge zero or more rankings/values lazily, preferring lower ranks."""

    iterator: Iterator[Any] = iter(values)
    try:
        first = ensure_ranking(next(iterator))
    except StopIteration:
        return RankingFunctionAny()

    result = first
    for value in iterator:
        result = result.merge(ensure_ranking(value), deduplicate=deduplicate)
    return result


def merge_apply(
    ranking: Any,
    func: Callable[[Any], Any],
    *,
    deduplicate: bool = False,
) -> RankingFunctionAny:
    """Monadic bind helper that preserves laziness."""

    ranking_any = ensure_ranking(ranking)
    return ranking_any.merge_apply(
        lambda value: ensure_ranking(func(value)),
        deduplicate=deduplicate,
    )


def observe(
    ranking: Any,
    predicate: Callable[[Any], bool],
    *,
    deduplicate: bool = False,
) -> RankingFunctionAny:
    """Condition ``ranking`` on a Python predicate."""

    ranking_any = ensure_ranking(ranking)
    return ranking_any.observe(predicate, deduplicate=deduplicate)


def observe_value(
    ranking: Any,
    value: Any,
    *,
    deduplicate: bool = False,
) -> RankingFunctionAny:
    """Condition ``ranking`` on equality with ``value``."""

    ranking_any = ensure_ranking(ranking)
    return ranking_any.observe_value(value, deduplicate=deduplicate)


def take_n(ranking: Any, count: int) -> list[tuple[Any, Rank]]:
    """Materialise up to ``count`` elements from ``ranking``."""

    return ensure_ranking(ranking).take_n(count)


def ranked_apply(
    func_or_ranking: Any,
    *args: Any,
    deduplicate: bool = False,
) -> RankingFunctionAny:
    """Apply (possibly ranked) functions to (possibly ranked) arguments lazily."""

    if isinstance(func_or_ranking, RankingFunctionAny):
        return merge_apply(
            func_or_ranking,
            lambda fn: ranked_apply(fn, *args, deduplicate=deduplicate),
            deduplicate=deduplicate,
        )

    if not callable(func_or_ranking):  # pragma: no cover - defensive
        raise TypeError("ranked_apply expects a callable or RankingFunctionAny")

    arg_rankings = [ensure_ranking(arg) for arg in args]

    def loop(index: int, collected: tuple[Any, ...]) -> RankingFunctionAny:
        if index == len(arg_rankings):
            return ensure_ranking(func_or_ranking(*collected))
        return merge_apply(
            arg_rankings[index],
            lambda value: loop(index + 1, collected + (value,)),
            deduplicate=deduplicate,
        )

    return loop(0, tuple())


def rlet_star(
    bindings: Sequence[tuple[str, Callable[[Mapping[str, Any]], Any]]],
    body: Callable[[Dict[str, Any]], Any],
) -> RankingFunctionAny:
    """Sequential ranked let-binding helper."""

    def loop(index: int, env: Dict[str, Any]) -> RankingFunctionAny:
        if index == len(bindings):
            return ensure_ranking(body(env))

        name, builder = bindings[index]
        candidate = ensure_ranking(builder(env))
        return merge_apply(
            candidate,
            lambda value: loop(index + 1, {**env, name: value}),
            deduplicate=False,
        )

    return loop(0, {})