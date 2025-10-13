# ranked_belief API Guide

This document distills the literate Doxygen comments found throughout `include/ranked_belief/` into a narrative tour of the public API. The focus is on **lazy semantics**, showing where evaluation is deferred and how ranks propagate through the algebra.

> ℹ️ All code snippets are valid C++20 and compile against the headers in this repository. They assume `using namespace ranked_belief;` or fully qualified names where necessary.

## Core Concepts

### `Rank`

Represents the ordinal plausibility of an outcome. Lower ranks correspond to more plausible explanations.

```cpp
using ranked_belief::Rank;

Rank zero = Rank::zero();             // Always finite, value 0
Rank rare = Rank::from_value(3);      // Custom finite rank
Rank impossible = Rank::infinity();   // Impossible observation
```

- Addition (`operator+`) saturates at infinity and throws on overflow for finite operands.
- Comparison uses a three-way comparison (`<=>`) that treats infinity as greater than any finite rank.
- Stream output prints either the finite value or `∞`.

### `Promise<T>`

Thread-safe, memoized thunk that defers the computation of a value. Every lazy tail in the library is wrapped in a `Promise` to guarantee single evaluation even under concurrent access.

```cpp
Promise<int> expensive{[] { return simulate(); }};
auto value = expensive.force();  // Executes at most once
```

- Construct from a lambda (`std::function<T()>`) or an eagerly known value.
- `force()` and `force() const` block until the computation completes or throws.
- Use `is_forced()`, `has_value()`, and `has_exception()` to inspect the memoization state.

### `RankingElement<T>`

Immutable node in the lazy linked list backing every ranking function.

```cpp
auto tail = make_lazy_element<int>(
  1,
  Rank::from_value(1),
  [] { return make_terminal(2, Rank::from_value(2)); }
);
auto head = make_lazy_element(
  0,
  Rank::zero(),
  [tail] { return tail; }
);
```

- Stores the value inside a `Promise<T>` and the next pointer inside a `Promise<std::shared_ptr<RankingElement<T>>>`.
- `next()` forces the tail lazily; `next_is_forced()` reports whether that has already happened.
- Helper factories `make_terminal`, `make_element`, `make_lazy_element`, and `make_infinite_sequence` cover the common creation patterns.

### `RankingIterator<T>`

Input iterator that walks the lazy ranking structure without forcing more elements than necessary.

```cpp
RankingFunction<int> rf = constructors::from_values_sequential({0,1,2}, Rank::zero(), true);
for (auto [value, rank] : rf) {
  std::cout << value << " @ " << rank << '\n';
}
```

- Deduplicates adjacent equal values by default (preserving the lowest rank); disable by passing `false` to the constructor or `RankingFunction` factory.
- Compatible with `<ranges>` algorithms.

### `RankingFunction<T>`

The primary container type representing a lazy ranking over values of `T`.

```cpp
auto rf = constructors::singleton(std::string{"working"}, Rank::zero());
if (auto first = rf.first()) {
  std::cout << first->first << " has rank " << first->second << '\n';
}
```

- `begin()`/`end()` create iterators that respect deduplication settings.
- `first()` returns the minimal-rank element without materializing the rest of the structure.
- `size()` counts elements lazily; it pulls the entire sequence only if necessary.
- `is_deduplicating()` indicates whether duplicates are removed on traversal.

## Construction Utilities (`constructors.hpp`)

The constructors compose ranking functions from vectors, ranges, and generators while keeping evaluation lazy.

```cpp
using ranked_belief::constructors;

auto rf = constructors::from_generator<int>(
  [](std::size_t index) {
    return std::pair{static_cast<int>(index), Rank::from_value(index)};
  },
  /*start_index=*/0,
  /*deduplicate=*/true
);
```

- `from_list` consumes value-rank pairs.
- `from_values_uniform` and `from_values_sequential` assign consistent ranks across a vector.
- `from_generator` and `make_infinite_sequence` allow infinite lazy streams; ranks and values are computed on demand.
- `singleton` and `empty` provide convenience wrappers.

## Transformation Operations

### `map`, `map_with_rank`, `map_with_index`

Apply functions lazily while preserving or adjusting ranks.

```cpp
auto doubled = map(rf, [](int v) { return v * 2; });
auto labelled = map_with_index(rf, [](int v, std::size_t index) {
  return fmt::format("{}: {}", index, v);
});
auto reweighted = map_with_rank(rf, [](int v, Rank r) {
  return std::pair{v, r + Rank::from_value(2)};
});
```

- All variants return immediately; computation happens only when consumers force values.
- Exceptions thrown inside the mapping functions propagate through the lazy tail during traversal.

### `filter`, `take`, `take_while_rank`

Restrict rankings based on predicates, prefixes, or rank thresholds.

```cpp
auto plausible = filter(rf, [](int v) { return v % 2 == 0; });
auto top_three = take(rf, 3);
auto within_one = take_while_rank(rf, Rank::from_value(1));
```

- Filtering never forces more of the source ranking than needed to satisfy the request.
- `take` materializes only the requested prefix and keeps the tail lazy.

### `merge` and `merge_all`

Combine rankings while preserving global rank ordering.

```cpp
auto combined = merge(rf_a, rf_b);
auto all = merge_all(std::vector{rf_a, rf_b, rf_c});
```

- Duplicate values adopt the minimal combined rank (deduplication-aware).
- Infinite inputs interleave without starving any branch thanks to lazy tails.

### `merge_apply`

Applicative merge that evaluates a ranking of functions against rankings of arguments.

```cpp
auto adders = constructors::from_list<std::function<int(int)>>({
  {[](int x) { return x + 1; }, Rank::zero()},
  {[](int x) { return x + 2; }, Rank::from_value(1)}
});

auto applied = merge_apply(adders, rf_values);
```

- Rank contributions accumulate across the function and argument branches.
- Returns lazily just like the simpler merge.

### `observe`

Condition a ranking on an observed outcome.

```cpp
auto posterior = observe(rf, expected_value);
if (auto explanation = posterior.first()) {
  fmt::print("Most plausible explanation: {} (rank {})\n",
             explanation->first, explanation->second.value());
}
```

- Normalizes ranks so the observed value has rank zero.
- Inconsistent observations yield an empty ranking.

### Normal / Exceptional Queries (`most_normal`, `take_n`)

Materialize the most plausible element or a finite prefix while preserving laziness elsewhere.

```cpp
auto explanation = most_normal(posterior);
auto sample = take_n(posterior, 5);  // returns std::vector<std::pair<T, Rank>>
```

- `most_normal` is constant-time when the head is already forced.
- `take_n` only forces the prefixes requested and leaves the remainder untouched.

## Autocast & Operator Overloads

### `autocast`

Lifts scalars into singleton rankings or forwards ranking functions untouched.

```cpp
RankingFunction<int> rf = autocast(5);       // singleton
RankingFunction<int> same = autocast(rf);    // identity
```

### Arithmetic and Comparison Operators

Operators are sugar on top of `merge_apply` and `autocast`, so they also respect lazy semantics.

```cpp
auto sum = rf_numbers + 3;        // scalar lifted lazily
auto product = rf_a * rf_b;       // ranks add
auto comparison = (rf_a == rf_b); // returns RankingFunction<bool>
```

- Operators never force their operands during construction.
- Result rankings inherit deduplication from the left-hand operand.

## Working with Laziness

- Favor `take_n`, `take`, or range-based loops with explicit limits when exploring infinite rankings.
- Use `next_is_forced()` and `Promise::is_forced()` in diagnostics to confirm deferred evaluation.
- All operations are thread-safe thanks to the underlying promises, but avoid forcing the same ranking from multiple threads without need—memoization still happens, yet contention may reduce throughput.

## Documentation & Tooling

- Generate HTML reference docs via `doxygen Doxyfile` (output in `build/docs/html`).
- Pair the generated documentation with these examples to understand how rank arithmetic and lazy semantics compose across the API surface.
- Address any Doxygen warnings before merging new functionality; they often indicate missing narrative comments or malformed markup in the headers.
