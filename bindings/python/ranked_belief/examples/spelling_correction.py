"""Spelling correction example using trie-based wildcard matching."""

from __future__ import annotations

import os
from collections.abc import Iterable
from functools import lru_cache
from pathlib import Path

from .. import Rank, RankingFunctionAny
from ..dsl import merge_apply, normal_exceptional, observe, take_n

__all__ = ["correct", "suggest"]

_DICTIONARY_FILENAME = "google-10000-english-no-swears.txt"


def _resolve_dictionary_path() -> Path:
    """Locate the dictionary file from either source or build trees.

    Searches the directory hierarchy above this module for a ``ranked-programming``
    checkout containing the example dictionary. Falls back to an environment override
    for testing or custom paths.
    
    Returns:
        Path to the dictionary file.
        
    Raises:
        FileNotFoundError: If the dictionary cannot be located.
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

# Trie structure: Dict[char, (child_trie, words_ending_here)]
Trie = dict[str, tuple["Trie", list[str]]]


@lru_cache(maxsize=1)
def _load_dictionary() -> list[str]:
    """Load the dictionary file.
    
    Returns:
        List of words from the dictionary file.
    """
    with _DICTIONARY_PATH.open("r", encoding="utf8") as handle:
        return [line.strip() for line in handle if line.strip()]


@lru_cache(maxsize=1)
def _build_trie() -> Trie:
    """Build a trie from the dictionary for efficient prefix/wildcard matching.
    
    Returns:
        Root node of the trie structure.
    """
    root: Trie = {}
    for word in _load_dictionary():
        node = root
        for char in word:
            node = node.setdefault(char, ({}, []))[0]
        # Use '$' as terminal marker
        _terminal_children, terminal_words = node.setdefault("$", ({}, []))
        terminal_words.append(word)
    return root


def _lookup(node: Trie, pattern: list[str], index: int = 0) -> list[str]:
    """Look up words matching a pattern with wildcards.
    
    Args:
        node: Current trie node.
        pattern: Pattern to match (list of chars or '*' wildcards).
        index: Current position in the pattern.
        
    Returns:
        List of matching words.
    """
    if index == len(pattern):
        # End of pattern - collect all words from here
        results: list[str] = []
        for child_node, words in node.values():
            results.extend(words)
        return results

    char = pattern[index]
    results: list[str] = []
    
    if char == "*":
        # Wildcard matches any character
        for child_node, words in node.values():
            results.extend(words)
            results.extend(_lookup(child_node, pattern, index + 1))
    elif char in node:
        # Exact character match
        child_node, words = node[char]
        results.extend(words)
        results.extend(_lookup(child_node, pattern, index + 1))
        
    return results


def _gen(pattern: list[str]) -> RankingFunctionAny:
    """Generate ranked wildcard patterns from a character pattern.
    
    For each character, normally keeps it, exceptionally replaces with wildcard.
    
    Args:
        pattern: List of characters to generate patterns from.
        
    Returns:
        Ranking over patterns (tuples of chars or '*').
    """
    if not pattern:
        return RankingFunctionAny.singleton(tuple())

    head, *tail = pattern
    tail_rf = _gen(tail)

    # Normally keep the character, exceptionally wildcard it
    keep_head = tail_rf.map(lambda rest: (head,) + rest)
    wildcard = tail_rf.map(lambda rest: ("*",) + rest)
    return keep_head.merge(wildcard)


def correct(word: str) -> RankingFunctionAny:
    """Generate spelling corrections for a misspelled word.
    
    Uses a ranked wildcard approach: normally keeps each character,
    exceptionally replaces it with a wildcard that matches any character.
    
    Args:
        word: The potentially misspelled word.
        
    Returns:
        Ranking over correction patterns that match dictionary words,
        ordered by number of wildcards used (fewer = more normal).
    """
    pattern = list(word)

    def predicate(candidate: tuple[str, ...]) -> bool:
        """Check if the pattern matches any dictionary words."""
        return bool(_lookup(_build_trie(), list(candidate)))

    return observe(_gen(pattern), predicate)


def suggest(word: str, limit: int = 5) -> list[tuple[str, Rank]]:
    """Get top spelling correction suggestions.
    
    Args:
        word: The potentially misspelled word.
        limit: Maximum number of suggestions to return.
        
    Returns:
        List of (corrected_word, rank) tuples ordered by plausibility.
    """
    candidates = correct(word)
    return [("".join(value), rank) for value, rank in take_n(candidates, limit)]
