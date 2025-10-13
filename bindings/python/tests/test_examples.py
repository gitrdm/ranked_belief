"""Integration tests for the Python ports of the ranked-programming examples."""

from __future__ import annotations

from pathlib import Path

import pytest  # type: ignore[import-not-found]

from ranked_belief import Rank, RankingFunctionAny
from ranked_belief.examples import boolean_circuit, ranked_let, ranked_procedure_call
from ranked_belief.examples.hidden_markov import hmm as hmm_weather
#from ranked_belief.examples.localisation import hmm as hmm_localisation
from ranked_belief.examples.recursion import first_values, recursive_fun, values_greater_than

TYPED_BUILD_PATH = Path(__file__).resolve().parents[2]


@pytest.mark.parametrize(
    ("input1", "input2", "input3", "output", "expected_top_gate"),
    [
        (False, False, True, False, "O2"),
        (True, False, False, True, "N"),
    ],
)
def test_boolean_circuit_top_ranked_gate(input1, input2, input3, output, expected_top_gate):
    results = boolean_circuit.diagnose(input1, input2, input3, output, limit=3)
    assert results
    top_scenario, rank = results[0]
    assert isinstance(rank, Rank)
    assert top_scenario[expected_top_gate] is False


def test_recursive_fun_infinite_sequence_remains_lazy():
    ranking = recursive_fun(1)
    assert isinstance(ranking, RankingFunctionAny)
    first = ranking.first()
    assert first is not None
    assert first[0] == 1

    # Taking more values shouldn't exhaust the lazy sequence
    prefix = first_values(10)
    assert len(prefix) == 10
    posterior = values_greater_than(100, limit=3)
    assert all(value > 100 for value, _ in posterior)


def test_ranked_let_preferences():
    preferences = ranked_let.materialised_preferences(limit=4)
    assert {tuple(sorted(pref.items())) for pref, _ in preferences}


@pytest.mark.parametrize(
    "observations",
    [
        ("no", "no", "yes"),
        ("yes", "yes", "no"),
    ],
)
def test_hidden_markov_returns_paths(observations):
    ranking = hmm_weather(observations)
    prefix = ranking.take_n(5)
    assert prefix
    for path, rank in prefix:
        assert isinstance(rank, Rank)
        assert len(path) == len(observations)


def test_ranked_procedure_call_uncertainty():
    results = ranked_procedure_call.uncertain_operator_sum()
    assert results
    values = [value for value, _ in results]
    assert set(values) <= {15, -5}
