"""Spelling correction example using trie-based wildcard matching."""

from __future__ import annotations

import os
from functools import lru_cache
from pathlib import Path
from typing import Dict, Iterable, List, Tuple

from .. import Rank, RankingFunctionAny
from ..dsl import merge_apply, normal_exceptional, observe, take_n

__all__ = ["correct", "suggest"]

_DICTIONARY_FILENAME = "google-10000-english-no-swears.txt"


def _resolve_dictionary_path() -> Path:
    """Locate the dictionary file from either source or build trees.

    Searches the directory hierarchy above this module for a ``ranked-programming``
    checkout containing the example dictionary. Falls back to an environment override
    for testing or custom paths.
    """

    override = os.environ.get("RANKED_BELIEF_DICTIONARY")
    if override:
        candidate = Path(override).expanduser().resolve()
        if candidate.is_file():
            return candidate
        raise FileNotFoundError(
            f"Environment variable RANKED_BELIEF_DICTIONARY={candidate} does not point to a file"
        )

    relative = Path("ranked-programming") / "examples" / _DICTIONARY_FILENAME
    for base in Path(__file__).resolve().parents:
        candidate = base / relative
        if candidate.is_file():
            return candidate

    raise FileNotFoundError(
        "Unable to locate dictionary file 'google-10000-english-no-swears.txt'. "
        "Provide it by setting RANKED_BELIEF_DICTIONARY or placing the ranked-programming"
        " checkout alongside this package."
    )


_DICTIONARY_PATH = _resolve_dictionary_path()

Trie = Dict[str, Tuple["Trie", List[str]]]


@lru_cache(maxsize=1)
def _load_dictionary() -> List[str]:
    with _DICTIONARY_PATH.open("r", encoding="utf8") as handle:
        return [line.strip() for line in handle if line.strip()]


@lru_cache(maxsize=1)
def _build_trie() -> Trie:
    root: Trie = {}
    for word in _load_dictionary():
        node = root
        for char in word:
            node = node.setdefault(char, ({}, []))[0]
        terminal_children, terminal_words = node.setdefault("$", ({}, []))
        terminal_words.append(word)
    return root


def _lookup(node: Trie, pattern: List[str], index: int = 0) -> List[str]:
    if index == len(pattern):
        results: List[str] = []
        for child in node.values():
            results.extend(child[1])
        return results

    char = pattern[index]
    results: List[str] = []
    if char == "*":
        for child in node.values():
            results.extend(child[1])
            results.extend(_lookup(child[0], pattern, index + 1))
    elif char in node:
        child_node, words = node[char]
        results.extend(words)
        results.extend(_lookup(child_node, pattern, index + 1))
    return results


def _gen(pattern: List[str]) -> RankingFunctionAny:
    if not pattern:
        return RankingFunctionAny.singleton(tuple())

    head, *tail = pattern
    tail_rf = _gen(tail)

    keep_head = tail_rf.map(lambda rest: (head,) + rest)
    wildcard = tail_rf.map(lambda rest: ("*",) + rest)
    return keep_head.merge(wildcard)


def correct(word: str) -> RankingFunctionAny:
    pattern = list(word)

    def predicate(candidate: Tuple[str, ...]) -> bool:
        return bool(_lookup(_build_trie(), list(candidate)))

    return observe(_gen(pattern), predicate)


def suggest(word: str, limit: int = 5) -> List[Tuple[str, Rank]]:
    candidates = correct(word)
    return [("".join(value), rank) for value, rank in take_n(candidates, limit)]
