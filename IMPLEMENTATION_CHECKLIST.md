# ranked_belief Implementation Checklist

## Progress Summary (Last Updated: Phase 6.2 In Progress)

**Status**: Phase 6.2 In Progress – C-facing façade with opaque handles, lazy map/filter callbacks, and merge/observe support implemented and under review
**Test Count**: 434 tests passing
**Coverage**: 
- Phase 1: Foundation (Rank, Promise) ✅
- Phase 2: Data Structures (RankingElement, Iterator, Function) ✅  
- Phase 3.1: Construction Operations (6 constructors) ✅
- Phase 3.2: Map Operations (3 variants) ✅
- Phase 3.3: Filter Operations (3 variants) ✅
- Phase 3.4: Merge Operations (2 functions) ✅
- Phase 3.5: Merge-Apply Operation ✅
- Phase 3.6: Observe Operation ✅
- Phase 3.7: Normal/Exceptional Operations ✅
- Phase 4.1: Autocast Mechanism ✅
- Phase 4.2: Operator Overloads ✅
- Phase 5.1: Integration Tests ✅
- Phase 5.2: Examples ✅
- Phase 5.3: Documentation ✅
- Phase 6.1: Type Erasure Layer ✅
- Phase 6.2: C API ⏳ (header/implementation/tests authored; pending final commit gate)

**Next Phase**: 6.2 C API (finalise & commit)

---

### 1.1 Project Structure Setup
- [x] Create CMake project structure (CMakeLists.txt at root)
  - Set C++20 requirement: `set(CMAKE_CXX_STANDARD 20)`
  - Add compiler version checks (GCC 10+, Clang 13+, MSVC 19.29+)
- [x] Create directory structure: `include/ranked_belief/`, `src/`, `tests/`, `examples/`
- [x] Configure Google Test integration in CMake
- [x] Create `.clang-format` with project style
- [x] Initialize git repository with `.gitignore`
- [x] **COMMIT**: "Initialize project structure"

### 1.2 Rank Type Implementation# Coding Standards (MANDATORY)

### Quality Requirements
1. **NO** stubs, placeholders, TODOs, or simplified code
2. **YES** production-quality code only with extensive automated test coverage
3. **YES** root cause analysis for ALL issues - no compromises on quality
4. **YES** literate-style Doxygen comments for all public APIs
5. **YES** best-practice coding only (Modern C++20, RAII, const-correctness)
6. **YES** frequent git commits at logical gates (only when tests pass and quality standards met)

### Commit Gate Criteria
- All tests passing (100% of existing tests)
- New functionality has ≥90% test coverage
- No compiler warnings (-Wall -Wextra -Wpedantic)
- Doxygen comments complete for public APIs
- Code follows Modern C++20 idioms

### LLM Agent Reorientation Protocol
When context window resets:
1. Read this checklist to find current phase
2. Check function signatures in relevant section
3. Run `git log --oneline -10` to see recent commits
4. Run tests to verify current state
5. Review last completed checkpoint
6. Continue from next unchecked item

---

## Phase 1: Foundation (Weeks 1-2)

### 1.1 Project Structure Setup
- [x] Create CMake project structure (CMakeLists.txt at root)
  - Set C++20 requirement: `set(CMAKE_CXX_STANDARD 20)`
  - Add compiler version checks (GCC 10+, Clang 13+, MSVC 19.29+)
- [x] Create directory structure: `include/ranked_belief/`, `src/`, `tests/`, `bindings/`
- [x] Configure Google Test integration in CMake
- [x] Create `.clang-format` with project style
- [x] Initialize git repository with `.gitignore`
- [x] **COMMIT**: "Initialize project structure"

### 1.2 Rank Type Implementation
**File**: `include/ranked_belief/rank.hpp`

**Key Function Signatures**:
```cpp
class Rank {
  static Rank zero();
  static Rank infinity();
  bool is_infinity() const;
  Rank operator+(const Rank& other) const;
  std::strong_ordering operator<=>(const Rank& other) const;
};
```

**Checklist**:
- [x] Implement `Rank` class with `uint64_t` storage + infinity flag
- [x] Implement arithmetic operators with infinity handling
- [x] Implement comparison operators (spaceship operator)
- [x] Add stream output operator
- [x] Create `tests/rank_test.cpp` with comprehensive tests:
  - Arithmetic with normal ranks
  - Arithmetic with infinity
  - Overflow behavior
  - Comparison edge cases
- [x] All tests passing (66/66 tests)
- [x] **COMMIT**: "Implement Rank type with full test coverage"

### 1.3 Promise<T> Implementation
**File**: `include/ranked_belief/promise.hpp`

**Key Function Signatures**:
```cpp
template<typename T>
class Promise {
  explicit Promise(std::function<T()> computation);
  explicit Promise(T value);
  Promise(Promise&&) noexcept;
  Promise& operator=(Promise&&) noexcept;
  T& force();
  const T& force() const;
  bool is_forced() const;
  bool has_value() const;
  bool has_exception() const;
};
```

**Checklist**:
- [x] Implement move-only semantics
- [x] Implement thread-safe memoization using `std::call_once`
- [x] Handle exceptions from computation function
- [x] Handle forced promise moves (std::once_flag limitation)
- [x] Create `tests/promise_test.cpp`:
  - Single-threaded force
  - Multi-threaded concurrent force
  - Exception propagation
  - Move semantics
  - Complex types (vectors, unique_ptr, custom types)
  - Reference returns
- [x] All tests passing (45/45 tests)
- [x] **COMMIT**: "Implement Promise<T> with thread-safe memoization"

---

## Phase 2: Core Data Structures (Weeks 2-3)

### 2.1 RankingElement<T>
**File**: `include/ranked_belief/ranking_element.hpp`

**Key Function Signatures**:
```cpp
template<typename T>
class RankingElement {
  RankingElement(T value, Rank rank, Promise<std::shared_ptr<RankingElement<T>>> next);
  const T& value() const;
  Rank rank() const;
  std::shared_ptr<RankingElement<T>> next();
  bool is_last() const;
  bool next_is_forced() const;
};

// Helper functions
template<typename T>
std::shared_ptr<RankingElement<T>> make_terminal(T value, Rank rank);

template<typename T, typename F>
std::shared_ptr<RankingElement<T>> make_lazy_element(T value, Rank rank, F computation);

template<typename T>
std::shared_ptr<RankingElement<T>> make_infinite_sequence(
    std::function<std::pair<T, Rank>(std::size_t)> generator,
    std::size_t start_index = 0
);
```

**Checklist**:
- [x] Implement lazy linked list node
- [x] Store value, rank, and lazy next pointer (Promise<shared_ptr<RankingElement<T>>>)
- [x] Implement immutable semantics (deleted copy/move)
- [x] Add helper functions for construction (terminal, lazy, infinite sequence)
- [x] Create `tests/ranking_element_test.cpp`:
  - Construction with various types (lazy, eager, terminal)
  - Value/rank access and next pointer behavior
  - Sequence construction (finite and infinite)
  - Lazy evaluation of next
  - Memory management (shared_ptr semantics)
  - Thread safety (concurrent access)
  - Complex types (strings, vectors, custom types)
  - Edge cases (single element, same ranks, infinite ranks)
- [x] All tests passing (38/38 tests)
- [x] **COMMIT**: "Implement RankingElement<T> lazy list node"

### 2.2 RankingIterator<T>
**File**: `include/ranked_belief/ranking_iterator.hpp`

**Key Function Signatures**:
```cpp
template<typename T>
class RankingIterator {
  using iterator_category = std::input_iterator_tag;
  using value_type = std::pair<T, Rank>;
  
  explicit RankingIterator(std::shared_ptr<RankingElement<T>> start, bool deduplicate = true);
  RankingIterator& operator++();
  RankingIterator operator++(int);
  value_type operator*() const;
  bool operator==(const RankingIterator&) const;
  bool operator!=(const RankingIterator&) const;
  bool is_end() const;
  bool is_deduplicating() const;
};
```

**Checklist**:
- [x] Implement input iterator interface (C++20 standard)
- [x] Handle end-of-sequence (nullptr sentinel)
- [x] Implement deduplication logic (optional, controlled by flag)
- [x] Deduplication preserves first occurrence (minimum rank)
- [x] Create `tests/ranking_iterator_test.cpp`:
  - Construction (end sentinel, valid iterator, flags)
  - Dereference operations (value-rank pairs, various types)
  - Increment operations (pre/post-increment, reaching end)
  - Comparison operations (equality, inequality, end detection)
  - Iteration over finite sequences (single element, empty, multi-element)
  - Deduplication on/off (preserving first rank, all duplicates)
  - Complex types (strings, vectors)
  - C++20 ranges compatibility (distance, find, transform)
  - Edge cases (long sequences, many duplicates, iterator independence)
- [x] All tests passing (35/35 tests)
- [x] **COMMIT**: "Implement RankingIterator with deduplication"

### 2.3 RankingFunction<T> Core
**File**: `include/ranked_belief/ranking_function.hpp`

**Key Function Signatures**:
```cpp
template<typename T>
class RankingFunction {
  using iterator = RankingIterator<T>;
  using value_type = std::pair<T, Rank>;
  
  RankingFunction();  // Empty constructor
  explicit RankingFunction(std::shared_ptr<RankingElement<T>> head, bool dedup = true);
  
  iterator begin() const;
  iterator end() const;
  std::optional<value_type> first() const;
  bool is_empty() const;
  std::size_t size() const;
  bool is_deduplicating() const;
  std::shared_ptr<RankingElement<T>> head() const;
  
  bool operator==(const RankingFunction&) const;
  bool operator!=(const RankingFunction&) const;
};

// Factory functions
template<typename T> RankingFunction<T> make_empty_ranking();
template<typename T> RankingFunction<T> make_ranking_function(
    std::shared_ptr<RankingElement<T>> head, bool deduplicate = true);
template<typename T> RankingFunction<T> make_singleton_ranking(T value, Rank rank = Rank::zero());
```

**Checklist**:
- [x] Implement range interface (begin/end returning RankingIterator)
- [x] Store head element and deduplication flag
- [x] Implement query methods: first(), is_empty(), size(), is_deduplicating(), head()
- [x] Implement comparison operators (==, !=)
- [x] Implement factory functions (make_empty_ranking, make_ranking_function, make_singleton_ranking)
- [x] Create `tests/ranking_function_test.cpp`:
  - Construction (empty, from head, factory functions)
  - Range interface (begin/end, range-based for loops)
  - Deduplication behavior (with/without, preserving first rank)
  - Query operations (first, is_empty, size)
  - Comparison operations (equality, inequality)
  - Complex types (strings, vectors, pairs)
  - C++20 ranges (distance, find, transform, count, any_of)
  - Edge cases (single element, long sequences, const iteration, copy/assignment)
- [x] All tests passing (46/46 tests)
- [x] **COMMIT**: "Implement RankingFunction<T> core structure"

---

## Phase 3: Core Operations (Weeks 3-4)

### 3.1 Construction Functions
**File**: `include/ranked_belief/constructors.hpp`

**Key Function Signatures**:
```cpp
template<typename T> RankingFunction<T> from_list(std::vector<std::pair<T, Rank>>, bool deduplicate);
template<typename T> RankingFunction<T> from_values_uniform(std::vector<T>, Rank rank, bool deduplicate);
template<typename T> RankingFunction<T> from_values_sequential(std::vector<T>, Rank start_rank, bool deduplicate);
template<typename T, typename F> RankingFunction<T> from_values_with_ranker(std::vector<T>, F rank_fn, bool deduplicate);
template<typename T, typename F> RankingFunction<T> from_generator(F generator, size_t start_index, bool deduplicate);
template<typename R> auto from_range(R&& range, Rank start_rank, bool deduplicate);
template<typename R> auto from_pair_range(R&& range, bool deduplicate);
template<typename T> RankingFunction<T> singleton(T value, Rank rank);
template<typename T> RankingFunction<T> empty();
```

**Checklist**:
- [x] Implement from_list (value-rank pairs)
- [x] Implement from_values_uniform (all same rank)
- [x] Implement from_values_sequential (sequential ranks)
- [x] Implement from_values_with_ranker (custom rank function with C++20 concepts)
- [x] Implement from_generator for lazy infinite sequences
- [x] Implement from_range (C++20 ranges integration)
- [x] Implement from_pair_range (handles std::map const keys)
- [x] Implement singleton/empty convenience aliases
- [x] Create `tests/constructors_test.cpp`:
  - All constructor variants (58 tests)
  - Type deduction tests
  - Lazy behavior verification
  - C++20 ranges integration (filter, transform)
  - Edge cases (empty, large sequences, complex types)
- [x] All tests passing (271/271)
- [x] **COMMIT**: "Implement construction functions for RankingFunction (Phase 3.1)"

### 3.2 Map Operations
**Files**: 
- `include/ranked_belief/operations/map.hpp`
- Modified: `include/ranked_belief/ranking_element.hpp` (added Promise<T> support for lazy values)

**Key Function Signatures**:
```cpp
template<typename T, typename F>
requires std::invocable<F, const T&>
auto map(const RankingFunction<T>& rf, F func, bool deduplicate = true) 
    -> RankingFunction<std::invoke_result_t<F, const T&>>;

template<typename T, typename F>
requires std::invocable<F, const T&, Rank> && PairReturning<F, T>
auto map_with_rank(const RankingFunction<T>& rf, F func, bool deduplicate = true) 
    -> RankingFunction<typename std::invoke_result_t<F, const T&, Rank>::first_type>;

template<typename T, typename F>
requires std::invocable<F, const T&, size_t>
auto map_with_index(const RankingFunction<T>& rf, F func, bool deduplicate = true)
    -> RankingFunction<std::invoke_result_t<F, const T&, size_t>>;
```

**Checklist**:
- [x] **Architecture Change**: Modified RankingElement for lazy values
  - Changed `T value_` to `Promise<T> value_` for lazy computation
  - Added `Promise<T>` constructor overload
  - Updated `value()` to force Promise (thread-safe memoization)
  - Maintains backward compatibility with eager constructors
- [x] Implement `map<T,F>`: Transform values, preserve ranks
- [x] Implement `map_with_rank<T,F>`: Transform both values and ranks
- [x] Implement `map_with_index<T,F>`: Transform with index access
- [x] Use C++20 concepts for type safety (std::invocable, std::invoke_result_t)
- [x] Full lazy evaluation matching Racket semantics:
  - map() returns immediately without computation
  - Values computed only on access (first() or iteration)
  - Results memoized for efficiency
  - Thread-safe through Promise guarantees
- [x] Recursive lambda pattern with shared_ptr (avoid dangling references)
- [x] Handle function exceptions with proper propagation
- [x] Create `tests/operations/map_test.cpp` (46 comprehensive tests):
  - Basic map (empty, single, multiple, rank preservation)
  - Type transformations (int→string, int→double, int→pair)
  - Lazy evaluation and memoization verification
  - Deduplication behavior (with/without, creating new duplicates)
  - map_with_rank (basic, changing ranks, empty)
  - map_with_index (basic, rank preservation, empty, indexed pairs)
  - Exception handling (function throws, propagation)
  - Chaining/composition (multiple maps, map with index)
  - Edge cases (identity, constant, large 1000-element sequence, captures)
- [x] All tests passing (300/300 tests: 271 existing + 29 new)
- [x] **COMMIT**: "Phase 3.2: Implement lazy map operations with Promise-based values"

### 3.3 Filter Operation ✓
**File**: `include/ranked_belief/operations/filter.hpp`

**Key Function Signatures**:
```cpp
template<typename T, typename Pred>
RankingFunction<T> filter(const RankingFunction<T>& rf, Pred predicate);
template<typename T>
RankingFunction<T> take(const RankingFunction<T>& rf, std::size_t n);
template<typename T>
RankingFunction<T> take_while_rank(const RankingFunction<T>& rf, Rank max_rank);
```

**Checklist**:
- [x] Implement lazy filter preserving ranks
- [x] Skip elements not satisfying predicate
- [x] Maintain lazy evaluation
- [x] Implement take() (Racket's filter-after)
- [x] Implement take_while_rank() (Racket's up-to-rank)
- [x] Create `tests/operations/filter_test.cpp`:
  - Basic filtering (empty, single, multiple elements)
  - Rank preservation (sequential, uniform, non-sequential)
  - Lazy evaluation with infinite sequences
  - Complex predicates and type transformations
  - Deduplication behavior (with/without)
  - Operation chaining (filter+map+take, etc.)
  - Exception handling in predicates
  - Large sequence performance (10K elements)
  - take() tests (basic, rank preservation, lazy, chaining)
  - take_while_rank() tests (basic, non-sequential ranks, lazy)
- [x] All tests passing (343/343 tests: 300 existing + 43 new)
- [x] **COMMIT**: "Phase 3.3: Implement lazy filter operations"

### 3.4 Merge Operation
**File**: `include/ranked_belief/operations/merge.hpp`

**Key Function Signatures**:
```cpp
template<typename T>
RankingFunction<T> merge(const RankingFunction<T>& rf1, const RankingFunction<T>& rf2);
template<typename T>
RankingFunction<T> merge_all(const std::vector<RankingFunction<T>>& rankings);
```

**Checklist**:
- [x] Implement lazy merge maintaining sorted order (by rank)
- [x] Combine ranks using min for duplicates
- [x] Support n-way merge efficiently
- [x] Create `tests/operations/merge_test.cpp`:
  - Binary merge
  - N-way merge
  - Interleaving behavior
  - Rank combination
- [x] All tests passing
- [x] **COMMIT**: "Implement lazy merge operation"

**Status**: ✅ Complete (Commit: 9e4f59c)
- merge() and merge_all() implemented
- 30 tests covering all scenarios
- Verified against Racket semantics (seen_rank parameter)
- Deduplication support (default true)
- All 373 tests passing

### 3.5 Merge-Apply (Applicative) Operation
**File**: `include/ranked_belief/operations/merge_apply.hpp`

**Key Function Signatures**:
```cpp
template<typename F, typename... Args>
auto merge_apply(const RankingFunction<F>& funcs, const RankingFunction<Args>&... args)
  -> RankingFunction<std::invoke_result_t<F, Args...>>;
```

**Checklist**:
- [x] Implement lazy applicative merge
- [x] Apply ranking-of-functions to ranking-of-arguments
- [x] Combine ranks using addition
- [x] Create `tests/operations/merge_apply_test.cpp`:
  - Single argument application
  - Multi-argument application
  - Rank addition verification
  - Lazy evaluation
- [x] All tests passing
- [x] **COMMIT**: "Implement merge-apply operation"

**Status**: ✅ Complete — lazy applicative merge defers evaluation via promises, preserves rank ordering, and passes 27 dedicated tests plus the full suite (395 tests). See commit "Implement merge-apply operation".

**Notes**:
- Introduced `detail::merge_with_ranks` helper to weave function/argument tails lazily.
- Ensured `merge_apply` composes arbitrary arity with `std::invoke` and propagates moves safely.
- Verified laziness by counting generator/function invocations in `MergeApplyIsLazy`.

### 3.6 Observe Operation
**File**: `include/ranked_belief/operations/observe.hpp`

**Key Function Signatures**:
```cpp
template<typename T>
RankingFunction<T> observe(const RankingFunction<T>& rf, const T& observed_value);
```

**Checklist**:
- [x] Implement conditioning by observation
- [x] Shift ranks so observed value has rank 0 while preserving laziness
- [x] Filter out impossible values
- [x] Create `tests/operations/observe_test.cpp`:
  - Observe existing value
  - Observe non-existing value
  - Multiple observations
  - Rank shifting verification
  - Lazy semantics and infinite rank handling
- [x] All tests passing (406/406)
- [x] **COMMIT**: "Implement observe conditioning operation"

**Status**: ✅ Complete — Observe now reuses lazy filtering, normalizes finite ranks, guards infinite ranks, and ships with 11 dedicated tests covering normalization, deduplication, laziness, and sequential observations.

### 3.7 Normal/Exceptional Operations
**File**: `include/ranked_belief/operations/nrm_exc.hpp`

**Key Function Signatures**:
```cpp
template<typename T>
std::optional<T> most_normal(const RankingFunction<T>& rf);
template<typename T>
std::vector<std::pair<T, Rank>> take_n(const RankingFunction<T>& rf, size_t n);
```

**Checklist**:
- [x] Implement most_normal (first element)
- [x] Implement take_n for extracting prefix while respecting laziness
- [x] Create `tests/operations/nrm_exc_test.cpp`:
  - Extract most normal value
  - Take first n elements
  - Empty ranking behavior
  - Rank preservation and laziness of take_n
- [x] All tests passing (413/413)
- [x] **COMMIT**: "Implement normal/exceptional query operations"

**Status**: ✅ Implementation complete — `most_normal` forwards to the lazy head accessor, while `take_n` materialises only the requested prefix and leaves the tail untouched. Seven targeted GoogleTests verify empty cases, partial pulls, rank preservation, and generator laziness.

---

## Phase 4: Autocast & Type System (Week 4)

### 4.1 Autocast Mechanism
**File**: `include/ranked_belief/autocast.hpp`

**Key Function Signatures**:
```cpp
template<typename T>
concept IsRankingFunction = /* ... */;

template<typename T>
RankingFunction<T> autocast(T&& value);

template<typename T> requires IsRankingFunction<T>
T&& autocast(T&& rf);
```

**Checklist**:
- [x] Define IsRankingFunction concept
- [x] Implement autocast for regular values (convert to singleton)
- [x] Implement autocast for RankingFunction (pass-through)
- [x] Create `tests/autocast_test.cpp`:
  - Autocast scalars
  - Autocast vectors
  - Autocast ranking functions
  - Type deduction verification
  - Laziness preservation checks
- [x] All tests passing (418/418)
- [x] **COMMIT**: "Implement autocast mechanism with concepts"

**Status**: ✅ Complete — Introduced the `IsRankingFunction` concept with detection traits and delivered dual `autocast` overloads. Plain values lift into singleton rankings while existing rankings are forwarded without forcing additional computation. Five targeted GoogleTests cover value lifting, reference preservation, and laziness.

### 4.2 Operator Overloads
**File**: `include/ranked_belief/operators.hpp`

**Key Function Signatures**:
```cpp
template<typename T, typename U>
auto operator+(const RankingFunction<T>& lhs, const RankingFunction<U>& rhs);

template<typename T, typename U> requires (!IsRankingFunction<U>)
auto operator+(const RankingFunction<T>& lhs, U&& rhs);

// Similar for -, *, /, ==, !=, <, >, <=, >=
```

**Checklist**:
- [x] Implement arithmetic operators using merge_apply
- [x] Implement comparison operators
- [x] Support mixed ranked/non-ranked operands via autocast
- [x] Create `tests/operators_test.cpp`:
  - Arithmetic with two rankings
  - Arithmetic with ranking and scalar
  - Comparison operators
  - Type promotion and laziness safeguards
- [x] All tests passing (423/423)
- [x] **COMMIT**: "Implement operator overloads with autocast"

**Status**: ✅ Complete — Added `combine_binary` with autocast lifting and lazy cartesian merges, implemented the full suite of arithmetic and comparison overloads with Doxygen coverage, and expanded operator tests to cover scalar mixing, ordering, boolean collapse, and generator laziness.

---

## Phase 5: Testing & Examples (Week 4)

### 5.1 Integration Tests
**File**: `tests/integration_test.cpp`

**Checklist**:
- [x] Implement monty_hall_test (three doors problem)
- [x] Implement dice_sum_test (sum of two dice)
- [x] Implement recursive_sequence_test (Fibonacci-like)
- [x] Verify results match expected rank distributions
- [x] All tests passing
- [x] **COMMIT**: "Add integration tests for core scenarios"

**Status**: ✅ Complete — Added `integration_test.cpp` covering Monty Hall posterior ranking, dice sum combinatorics with laziness checks, and Fibonacci generation via lazy generator memoisation. Test suite now counts 426 cases.

### 5.2 Examples
**Files**: `examples/boolean_circuit.cpp`, `examples/recursion.cpp`

**Checklist**:
- [x] Port boolean_circuit.rkt to C++
- [x] Port recursion.rkt to C++
- [x] Add README.md in examples/ with build instructions
- [x] Verify examples compile and produce correct output
- [x] **COMMIT**: "Add C++ example programs"

**Status**: ✅ Complete — C++ examples now mirror the original Racket demonstrations, showcasing lazy diagnosis of the boolean circuit and the recursive ranked chain. Both executables build via CMake, emit ranked outputs, and report forced tails to make laziness explicit.

### 5.3 Documentation
**Files**: `README.md`, `docs/API.md`, `Doxyfile`

**Checklist**:
- [x] Write README.md with quickstart guide
- [x] Document all public APIs with usage examples
- [x] Configure Doxygen (Doxyfile)
- [x] Generate HTML documentation
- [x] **COMMIT**: "Complete core library documentation"

---

## Phase 6: C API & FFI (Weeks 5-6)

### 6.1 Type Erasure Layer
**File**: `include/ranked_belief/type_erasure.hpp`

**Key Function Signatures**:
```cpp
class RankingFunctionAny {
  template<typename T> explicit RankingFunctionAny(RankingFunction<T> rf);
  std::any first_value() const;
  std::optional<Rank> first_rank() const;
  RankingFunctionAny map(std::function<std::any(std::any)> func) const;
};
```

**Checklist**:
- [x] Implement type-erased wrapper using std::any
- [x] Implement type-erased versions of all operations
- [x] Create `tests/type_erasure_test.cpp`:
  - Wrap various types
  - Type-erased operations
  - Type recovery
- [x] All tests passing
- [x] **COMMIT**: "Implement type erasure layer"

**Status**: ✅ Complete — Added `RankingFunctionAny` concept/model façade with Doxygen documentation, guarded deduplication for non-equality payloads, full lazy delegation (`map`, `filter`, `merge`, `merge_apply`, `observe`, `take_n`), and a GoogleTest suite validating wrapping, heterogenous merges, type recovery, and laziness counters.

### 6.2 C API
**File**: `include/ranked_belief/c_api.h`, `src/c_api.cpp`

**Key Function Signatures**:
```c
typedef enum rb_status rb_status;
typedef struct rb_ranking_t rb_ranking_t;

typedef rb_status (*rb_map_callback_t)(int input_value, void *context, int *output_value);
typedef rb_status (*rb_filter_callback_t)(int input_value, void *context, int *keep);

rb_status rb_singleton_int(int value, rb_ranking_t **out_ranking);
rb_status rb_from_array_int(const int *values, const uint64_t *ranks, size_t count, rb_ranking_t **out_ranking);
rb_status rb_map_int(const rb_ranking_t *ranking, rb_map_callback_t callback, void *context, rb_ranking_t **out_ranking);
rb_status rb_filter_int(const rb_ranking_t *ranking, rb_filter_callback_t callback, void *context, rb_ranking_t **out_ranking);
rb_status rb_merge_int(const rb_ranking_t *lhs, const rb_ranking_t *rhs, rb_ranking_t **out_ranking);
rb_status rb_observe_value_int(const rb_ranking_t *ranking, int value, rb_ranking_t **out_ranking);
rb_status rb_is_empty(const rb_ranking_t *ranking, int *out_is_empty);
rb_status rb_first_int(const rb_ranking_t *ranking, int *out_value, uint64_t *out_rank, int *out_has_value);
rb_status rb_take_n_int(const rb_ranking_t *ranking, size_t n, int *out_values, uint64_t *out_ranks, size_t buffer_size, size_t *out_count);
void rb_ranking_free(rb_ranking_t *ranking);
```

**Checklist**:
- [x] Define opaque handle types (`rb_ranking_t`) and status/ callback interfaces (`rb_status`, `rb_map_callback_t`, `rb_filter_callback_t`)
- [x] Implement C wrappers for core operations with lazy semantics preserved (`rb_map_int`, `rb_filter_int`, `rb_merge_int`, `rb_observe_value_int`, `rb_take_n_int`, etc.)
- [x] Implement proper error handling (status translation, buffer validation, callback propagation)
- [x] Create `tests/c_api_test.c`:
  - Pure C test binary exercising map, filter, merge, observe, take, null operands, and callback failures
  - Custom macros to avoid unused-variable warnings under `-Werror`
  - Verifies laziness counters and buffer error reporting
- [x] All tests passing (ctest: 434/434)
- [ ] **COMMIT**: "Implement C API for FFI compatibility"

**Status**: ⏳ In Progress — Library build targets, public header, and C test suite in place; ready for documentation polish and final commit gate.

---

## Phase 7: Python Bindings (Weeks 7-9)

### 7.1 Pybind11 Setup
**File**: `bindings/python/CMakeLists.txt`

**Checklist**:
- [ ] Configure pybind11 in CMake
- [ ] Create Python module structure
- [ ] Set up pytest infrastructure
- [ ] **COMMIT**: "Configure pybind11 build system"

### 7.2 Core Bindings
**File**: `bindings/python/ranked_belief.cpp`

**Key Bindings**:
```cpp
py::class_<Rank>(m, "Rank")
  .def_static("zero", &Rank::zero)
  .def("is_infinity", &Rank::is_infinity);

py::class_<RankingFunction<int>>(m, "RankingFunctionInt")
  .def("map", &RankingFunction<int>::map<...>)
  .def("filter", &RankingFunction<int>::filter<...>);
```

**Checklist**:
- [ ] Bind Rank class
- [ ] Bind RankingFunction<int>, RankingFunction<double>, RankingFunction<std::string>
- [ ] Bind construction functions
- [ ] Bind operations (map, filter, merge, etc.)
- [ ] Create `bindings/python/tests/test_core.py`:
  - Test all bound operations
  - Test Python lambda functions
  - Test error handling
- [ ] All tests passing
- [ ] **COMMIT**: "Implement Python core bindings"

### 7.3 Python Examples & Docs
**Files**: `bindings/python/examples/`, `bindings/python/README.md`

**Checklist**:
- [ ] Port examples to Python
- [ ] Write Python tutorial
- [ ] Add type stubs (.pyi files) for IDE support
- [ ] **COMMIT**: "Complete Python bindings with examples"

---

## Phase 8: R Bindings (Weeks 10-12)

### 8.1 Rcpp Setup
**File**: `bindings/r/DESCRIPTION`, `bindings/r/src/Makevars`

**Checklist**:
- [ ] Create R package structure
- [ ] Configure Rcpp in build system
- [ ] Set up testthat infrastructure
- [ ] **COMMIT**: "Configure Rcpp package structure"

### 8.2 Core Bindings
**File**: `bindings/r/src/ranked_belief.cpp`

**Key Bindings**:
```cpp
RCPP_MODULE(ranked_belief) {
  class_<Rank>("Rank")
    .method("is_infinity", &Rank::is_infinity);
  
  class_<RankingFunction<int>>("RankingFunctionInt")
    .method("map", &RankingFunction<int>::map<...>);
}
```

**Checklist**:
- [ ] Bind core types to R S4 classes
- [ ] Bind operations
- [ ] Handle R vector conversions
- [ ] Create `bindings/r/tests/testthat/test-core.R`:
  - Test all bound operations
  - Test R function callbacks
  - Test error handling
- [ ] All tests passing
- [ ] **COMMIT**: "Implement R core bindings"

### 8.3 R Examples & Docs
**Files**: `bindings/r/vignettes/`, `bindings/r/man/`

**Checklist**:
- [ ] Write R vignettes
- [ ] Generate roxygen2 documentation
- [ ] Port examples to R
- [ ] **COMMIT**: "Complete R bindings with documentation"

---

## Phase 9: Release Preparation (Weeks 13-14)

### 9.1 Performance Optimization
**Checklist**:
- [ ] Run profiler on example programs
- [ ] Optimize hot paths identified
- [ ] Add benchmarks (Google Benchmark)
- [ ] Document performance characteristics
- [ ] **COMMIT**: "Optimize performance and add benchmarks"

### 9.2 Continuous Integration
**File**: `.github/workflows/ci.yml`

**Checklist**:
- [ ] Configure GitHub Actions CI
- [ ] Test on Linux (GCC, Clang), macOS, Windows (MSVC)
- [ ] Run sanitizers (ASan, UBSan, TSan)
- [ ] Check code coverage (lcov/gcov)
- [ ] **COMMIT**: "Add CI pipeline with multi-platform testing"

### 9.3 Packaging
**Checklist**:
- [ ] Create CPack configuration for library
- [ ] Create Python wheel build (setup.py/pyproject.toml)
- [ ] Create R package tarball
- [ ] Write installation guides for each platform
- [ ] **COMMIT**: "Add packaging configuration"

### 9.4 Release
**Checklist**:
- [ ] Tag version 1.0.0
- [ ] Create GitHub release with binaries
- [ ] Publish to PyPI (Python)
- [ ] Publish to CRAN (R)
- [ ] Announce release
- [ ] **COMMIT**: "Release v1.0.0"

---

## Function Signature Reference (Quick Lookup)

### Core Types
```cpp
class Rank;                                    // rank.hpp
template<typename T> class Promise;            // promise.hpp
template<typename T> class RankingElement;     // ranking_element.hpp
template<typename T> class RankingIterator;    // ranking_iterator.hpp
template<typename T> class RankingFunction;    // ranking_function.hpp
```

### Construction
```cpp
template<typename T> RankingFunction<T> singleton(T);
template<typename T> RankingFunction<T> uniform(std::vector<T>);
template<typename T, typename Gen> RankingFunction<T> from_generator(Gen);
```

### Operations
```cpp
template<typename T, typename F> auto map(const RankingFunction<T>&, F);
template<typename T, typename Pred> RankingFunction<T> filter(const RankingFunction<T>&, Pred);
template<typename T> RankingFunction<T> merge(const RankingFunction<T>&, const RankingFunction<T>&);
template<typename F, typename... Args> auto merge_apply(const RankingFunction<F>&, const RankingFunction<Args>&...);
template<typename T> RankingFunction<T> observe(const RankingFunction<T>&, const T&);
template<typename T> std::optional<T> most_normal(const RankingFunction<T>&);
```

### Type System
```cpp
template<typename T> RankingFunction<T> autocast(T&&);
template<typename T> requires IsRankingFunction<T> T&& autocast(T&&);
```

### FFI Layer
```cpp
class RankingFunctionAny;                      // type_erasure.hpp
rb_ranking_t* rb_singleton_int(int);          // c_api.h
```

---

## Progress Tracking

**Current Phase**: Phase 3 - Core Operations
**Last Commit**: 6dffe87 - Implement construction functions for RankingFunction (Phase 3.1)
**Test Coverage**: 100% (271/271 tests passing: 66 Rank + 45 Promise + 38 RankingElement + 35 RankingIterator + 46 RankingFunction + 41 Constructors)

### Completed Items
✅ Phase 1.1: Project Structure Setup
  - CMakeLists.txt with C++20 requirement
  - Directory structure (include/, src/, tests/, examples/)
  - Google Test integration
  - .clang-format configuration
  - .gitignore

✅ Phase 1.2: Rank Type Implementation
  - Full Rank class with type safety
  - Arithmetic operators (addition, subtraction)
  - Comparison operators (spaceship operator)
  - Min/max operations
  - Increment/decrement operators
  - Stream output
  - 66 comprehensive tests (all passing)
  - Literate Doxygen comments

✅ Phase 1.3: Promise<T> Implementation
  - Thread-safe lazy evaluation with std::call_once
  - Move-only semantics
  - Exception caching and re-throwing
  - Handles forced promise moves correctly
  - Helper functions: make_promise, make_promise_value
  - 45 comprehensive tests (all passing)
  - Literate Doxygen comments

✅ Phase 2.1: RankingElement<T> Implementation
  - Lazy linked list node with Promise-based next pointer
  - Helper functions: make_terminal, make_element, make_lazy_element
  - make_infinite_sequence for infinite lazy sequences
  - 38 comprehensive tests (all passing)
  - Literate Doxygen comments

✅ Phase 2.2: RankingIterator<T> Implementation
  - C++20 input_iterator with std::input_iterator_tag
  - Optional deduplication (preserves first occurrence)
  - Works with std::ranges algorithms
  - 35 comprehensive tests (all passing)
  - Literate Doxygen comments

✅ Phase 2.3: RankingFunction<T> Implementation
  - Main user-facing API with range interface (begin/end)
  - Query methods: first, is_empty, size
  - Factory functions: make_empty_ranking, make_ranking_function, make_singleton_ranking
  - 46 comprehensive tests (all passing)
  - Literate Doxygen comments

✅ Phase 3.1: Construction Functions Implementation
  - from_list, from_values_uniform, from_values_sequential
  - from_values_with_ranker with C++20 concepts
  - from_generator for infinite sequences
  - from_range with C++20 ranges integration
  - from_pair_range handling std::map const keys
  - singleton/empty convenience aliases
  - 41 comprehensive tests (all passing)
  - Literate Doxygen comments

Update this section as you progress through each phase.
