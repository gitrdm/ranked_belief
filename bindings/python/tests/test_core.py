import itertools

import pytest  # type: ignore[import-not-found]

import ranked_belief as rb


@pytest.fixture
def sequential_int_ranking():
    return rb.RankingFunctionInt.from_values_sequential([1, 2, 3, 4])


def test_rank_arithmetic_and_comparison():
    r0 = rb.Rank.zero()
    r1 = rb.Rank.from_value(1)
    assert r0 == rb.Rank()
    assert r1 > r0
    assert int(r1) == 1
    assert (r0 + r1).value() == 1
    assert (rb.Rank.infinity() + r1).is_infinity()
    with pytest.raises(ValueError):
        rb.Rank.from_value(rb.Rank.max_finite_value())
    with pytest.raises(TypeError):
        int(rb.Rank.infinity())


def test_lazy_map_preserves_memoisation(sequential_int_ranking):
    calls = {"count": 0}

    def transform(value: int) -> int:
        calls["count"] += 1
        return value * 10

    mapped = sequential_int_ranking.map(transform)
    assert calls["count"] == 0

    first = mapped.first()
    assert calls["count"] == 1
    assert first is not None
    assert first[0] == 10
    assert first[1] == rb.Rank.zero()

    again = mapped.first()
    assert calls["count"] == 1
    assert again is not None and again[0] == 10


def test_lazy_filter():
    calls = {"count": 0}

    def generator(index: int) -> tuple[int, rb.Rank]:
        calls["count"] += 1
        return index, rb.Rank.from_value(index)

    source = rb.RankingFunctionInt.from_generator(generator)
    assert calls["count"] == 1  # from_generator materialises the head lazily

    filtered = source.filter(lambda value: value % 2 == 0)
    assert calls["count"] == 1  # constructing the filter should not over-force

    iterator = iter(filtered)
    first = next(iterator)
    assert first == (0, rb.Rank.zero())
    assert calls["count"] == 1  # head already available

    second = next(iterator)
    assert second == (2, rb.Rank.from_value(2))
    assert calls["count"] <= 4  # should only inspect as many elements as needed


def test_merge_and_merge_all():
    r0 = rb.Rank.zero()
    r1 = rb.Rank.from_value(1)
    r2 = rb.Rank.from_value(2)
    r3 = rb.Rank.from_value(3)

    left = rb.RankingFunctionInt.from_list([(1, r0), (3, r2)])
    right = rb.RankingFunctionInt.from_list([(2, r1), (4, r3)])

    merged = left.merge(right)
    assert [value for value, _ in merged.materialize(4)] == [1, 2, 3, 4]

    combined = rb.RankingFunctionInt.merge_all([left, right])
    assert [value for value, _ in combined.materialize(4)] == [1, 2, 3, 4]


def test_merge_apply():
    base = rb.RankingFunctionInt.from_values_sequential([1, 2])

    def branch(value: int):
        return rb.RankingFunctionInt.from_list([
            (value, rb.Rank.zero()),
            (value * 10, rb.Rank.from_value(1)),
        ])

    result = base.merge_apply(branch)
    assert [value for value, _ in result.materialize(4)] == [1, 10, 2, 20]


def test_observe_value_normalises_ranks():
    ranking = rb.RankingFunctionInt.from_values_sequential([5, 6, 7])
    observed = ranking.observe_value(7)
    materialised = observed.materialize(1)
    assert len(materialised) == 1
    value, rank = materialised[0]
    assert value == 7
    assert rank == rb.Rank.zero()


def test_most_normal_and_truthiness():
    empty = rb.RankingFunctionString()
    assert not empty
    singleton = rb.RankingFunctionString.singleton("hello")
    assert singleton
    assert singleton.most_normal() == "hello"


def test_take_variants():
    ranking = rb.RankingFunctionInt.from_values_sequential([1, 2, 3, 4])
    prefix = ranking.take(2)
    assert [value for value, _ in prefix.materialize(5)] == [1, 2]

    limited = ranking.take_while_rank(rb.Rank.from_value(1))
    assert [value for value, _ in limited.materialize(5)] == [1, 2]


def test_from_generator_is_lazy():
    calls = {"count": 0}

    def generator(index: int):
        calls["count"] += 1
        return float(index), rb.Rank.from_value(index)

    ranking = rb.RankingFunctionFloat.from_generator(generator)
    assert calls["count"] == 1  # head promise is realised to seed the ranking

    values = ranking.materialize(3)
    assert 3 <= calls["count"] <= 4
    assert [value for value, _ in values] == [0.0, 1.0, 2.0]


def test_error_propagates_from_python_callable():
    ranking = rb.RankingFunctionInt.from_values_uniform([1])

    def explode(_: int) -> int:
        raise ValueError("boom")

    mapped = ranking.map(explode)
    with pytest.raises(ValueError):
        mapped.first()


def test_string_rankings_support_merge_and_filter():
    greetings = rb.RankingFunctionString.from_list([
        ("hi", rb.Rank.zero()),
        ("hello", rb.Rank.from_value(2)),
    ])
    salutations = rb.RankingFunctionString.singleton("hey", rb.Rank.from_value(1))

    merged = greetings.merge(salutations)
    assert [value for value, _ in merged.materialize(3)] == ["hi", "hey", "hello"]

    filtered = merged.filter(lambda text: text.startswith("h"))
    assert [value for value, _ in filtered.materialize(3)] == ["hi", "hey", "hello"]


def test_observe_with_predicate():
    ranking = rb.RankingFunctionInt.from_values_sequential([10, 20, 30])

    def predicate(value: int) -> bool:
        return value >= 20

    observed = ranking.observe(predicate)
    materialised = observed.materialize(2)
    assert [value for value, _ in materialised] == [20, 30]
    assert materialised[0][1] == rb.Rank.zero()


def test_materialize_only_forces_prefix():
    ranking = rb.RankingFunctionInt.from_generator(lambda index: (index, rb.Rank.from_value(index)))
    prefix = ranking.materialize(5)
    assert len(prefix) == 5
    assert prefix[-1][0] == 4

    # Ensure subsequent materialisation continues lazily from promises
    next_prefix = ranking.take(7).materialize(7)
    assert len(next_prefix) == 7
    assert next_prefix[-1][0] == 6


def test_iterator_consumption_matches_python_protocol(sequential_int_ranking):
    iterator = iter(sequential_int_ranking)
    values = list(itertools.islice(iterator, 3))
    assert [value for value, _ in values] == [1, 2, 3]

    # Further iteration resumes lazily
    remaining = list(iterator)
    assert [value for value, _ in remaining] == [4]


def test_take_n_does_not_force_entire_sequence():
    calls = {"count": 0}

    def generator(index: int):
        calls["count"] += 1
        return index, rb.Rank.from_value(index)

    ranking = rb.RankingFunctionInt.from_generator(generator)
    ranking.materialize(2)
    assert calls["count"] <= 3
