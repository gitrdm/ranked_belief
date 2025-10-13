# ranked_belief Implementation C### 1.1 Project Structure Setup
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
- [ ] Create CMake project structure (CMakeLists.txt at root)
  - Set C++20 requirement: `set(CMAKE_CXX_STANDARD 20)`
  - Add compiler version checks (GCC 10+, Clang 13+, MSVC 19.29+)
- [ ] Create directory structure: `include/ranked_belief/`, `src/`, `tests/`, `bindings/`
- [ ] Configure Google Test integration in CMake
- [ ] Create `.clang-format` with project style
- [ ] Initialize git repository with `.gitignore`
- [ ] **COMMIT**: "Initialize project structure"

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
  Promise(Promise&&) noexcept;
  Promise& operator=(Promise&&) noexcept;
  T& force();
  const T& force() const;
  bool is_forced() const;
};
```

**Checklist**:
- [ ] Implement move-only semantics
- [ ] Implement thread-safe memoization using `std::call_once`
- [ ] Handle exceptions from computation function
- [ ] Create `tests/promise_test.cpp`:
  - Single-threaded force
  - Multi-threaded concurrent force
  - Exception propagation
  - Move semantics
- [ ] All tests passing
- [ ] **COMMIT**: "Implement Promise<T> with thread-safe memoization"

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
};
```

**Checklist**:
- [ ] Implement lazy linked list node
- [ ] Store value, rank, and lazy next pointer
- [ ] Implement move semantics for efficiency
- [ ] Create `tests/ranking_element_test.cpp`:
  - Construction with various types
  - Lazy evaluation of next
  - Memory management (shared_ptr semantics)
- [ ] All tests passing
- [ ] **COMMIT**: "Implement RankingElement<T> lazy list node"

### 2.2 RankingFunction<T> Iterator
**File**: `include/ranked_belief/ranking_iterator.hpp`

**Key Function Signatures**:
```cpp
template<typename T>
class RankingIterator {
  using iterator_category = std::input_iterator_tag;
  using value_type = std::pair<T, Rank>;
  RankingIterator& operator++();
  value_type operator*() const;
  bool operator==(const RankingIterator&) const;
};
```

**Checklist**:
- [ ] Implement input iterator interface
- [ ] Handle end-of-sequence (nullptr sentinel)
- [ ] Implement deduplication logic (optional, controlled by flag)
- [ ] Create `tests/ranking_iterator_test.cpp`:
  - Iteration over finite sequence
  - Deduplication on/off
  - End detection
  - C++20 ranges compatibility
- [ ] All tests passing
- [ ] **COMMIT**: "Implement RankingIterator with deduplication"

### 2.3 RankingFunction<T> Core
**File**: `include/ranked_belief/ranking_function.hpp`

**Key Function Signatures**:
```cpp
template<typename T>
class RankingFunction {
  explicit RankingFunction(std::shared_ptr<RankingElement<T>> head, bool dedup = true);
  RankingIterator<T> begin() const;
  RankingIterator<T> end() const;
  std::optional<std::pair<T, Rank>> first() const;
  bool is_empty() const;
};
```

**Checklist**:
- [ ] Implement range interface (begin/end)
- [ ] Store head element and deduplication flag
- [ ] Implement first(), is_empty() query methods
- [ ] Create `tests/ranking_function_test.cpp`:
  - Construction from lazy sequences
  - Range-based for loop iteration
  - Query operations
  - Empty ranking functions
- [ ] All tests passing
- [ ] **COMMIT**: "Implement RankingFunction<T> core structure"

---

## Phase 3: Core Operations (Weeks 3-4)

### 3.1 Construction Functions
**File**: `include/ranked_belief/constructors.hpp`

**Key Function Signatures**:
```cpp
template<typename T> RankingFunction<T> singleton(T value);
template<typename T> RankingFunction<T> uniform(std::vector<T> values);
template<typename T, typename Gen> RankingFunction<T> from_generator(Gen generator);
template<typename T> RankingFunction<T> empty();
```

**Checklist**:
- [ ] Implement singleton (rank 0)
- [ ] Implement uniform (all same rank)
- [ ] Implement from_generator for lazy infinite sequences
- [ ] Implement empty ranking
- [ ] Create `tests/constructors_test.cpp`:
  - All constructor variants
  - Type deduction
  - Lazy behavior verification
- [ ] All tests passing
- [ ] **COMMIT**: "Implement ranking construction functions"

### 3.2 Map Operation
**File**: `include/ranked_belief/operations/map.hpp`

**Key Function Signatures**:
```cpp
template<typename T, typename F>
auto map(const RankingFunction<T>& rf, F func) -> RankingFunction<std::invoke_result_t<F, T>>;
```

**Checklist**:
- [ ] Implement lazy map preserving rank structure
- [ ] Use Promise<T> for deferred computation
- [ ] Handle function exceptions properly
- [ ] Create `tests/operations/map_test.cpp`:
  - Map with pure functions
  - Map with type transformations
  - Lazy evaluation verification
  - Exception handling
- [ ] All tests passing
- [ ] **COMMIT**: "Implement lazy map operation"

### 3.3 Filter Operation
**File**: `include/ranked_belief/operations/filter.hpp`

**Key Function Signatures**:
```cpp
template<typename T, typename Pred>
RankingFunction<T> filter(const RankingFunction<T>& rf, Pred predicate);
```

**Checklist**:
- [ ] Implement lazy filter preserving ranks
- [ ] Skip elements not satisfying predicate
- [ ] Maintain lazy evaluation
- [ ] Create `tests/operations/filter_test.cpp`:
  - Filter finite sequences
  - Filter infinite sequences (verify laziness)
  - Empty result cases
- [ ] All tests passing
- [ ] **COMMIT**: "Implement lazy filter operation"

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
- [ ] Implement lazy merge maintaining sorted order (by rank)
- [ ] Combine ranks using min for duplicates
- [ ] Support n-way merge efficiently
- [ ] Create `tests/operations/merge_test.cpp`:
  - Binary merge
  - N-way merge
  - Interleaving behavior
  - Rank combination
- [ ] All tests passing
- [ ] **COMMIT**: "Implement lazy merge operation"

### 3.5 Merge-Apply (Applicative) Operation
**File**: `include/ranked_belief/operations/merge_apply.hpp`

**Key Function Signatures**:
```cpp
template<typename F, typename... Args>
auto merge_apply(const RankingFunction<F>& funcs, const RankingFunction<Args>&... args)
  -> RankingFunction<std::invoke_result_t<F, Args...>>;
```

**Checklist**:
- [ ] Implement lazy applicative merge
- [ ] Apply ranking-of-functions to ranking-of-arguments
- [ ] Combine ranks using addition
- [ ] Create `tests/operations/merge_apply_test.cpp`:
  - Single argument application
  - Multi-argument application
  - Rank addition verification
  - Lazy evaluation
- [ ] All tests passing
- [ ] **COMMIT**: "Implement merge-apply operation"

### 3.6 Observe Operation
**File**: `include/ranked_belief/operations/observe.hpp`

**Key Function Signatures**:
```cpp
template<typename T>
RankingFunction<T> observe(const RankingFunction<T>& rf, const T& observed_value);
```

**Checklist**:
- [ ] Implement conditioning by observation
- [ ] Shift ranks so observed value has rank 0
- [ ] Filter out impossible values
- [ ] Create `tests/operations/observe_test.cpp`:
  - Observe existing value
  - Observe non-existing value
  - Multiple observations
  - Rank shifting verification
- [ ] All tests passing
- [ ] **COMMIT**: "Implement observe conditioning operation"

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
- [ ] Implement most_normal (first element)
- [ ] Implement take_n for extracting prefix
- [ ] Create `tests/operations/nrm_exc_test.cpp`:
  - Extract most normal value
  - Take first n elements
  - Empty ranking behavior
- [ ] All tests passing
- [ ] **COMMIT**: "Implement normal/exceptional query operations"

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
- [ ] Define IsRankingFunction concept
- [ ] Implement autocast for regular values (convert to singleton)
- [ ] Implement autocast for RankingFunction (pass-through)
- [ ] Create `tests/autocast_test.cpp`:
  - Autocast scalars
  - Autocast vectors
  - Autocast ranking functions
  - Type deduction verification
- [ ] All tests passing
- [ ] **COMMIT**: "Implement autocast mechanism with concepts"

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
- [ ] Implement arithmetic operators using merge_apply
- [ ] Implement comparison operators
- [ ] Support mixed ranked/non-ranked operands via autocast
- [ ] Create `tests/operators_test.cpp`:
  - Arithmetic with two rankings
  - Arithmetic with ranking and scalar
  - Comparison operators
  - Type promotion
- [ ] All tests passing
- [ ] **COMMIT**: "Implement operator overloads with autocast"

---

## Phase 5: Testing & Examples (Week 4)

### 5.1 Integration Tests
**File**: `tests/integration_test.cpp`

**Checklist**:
- [ ] Implement monty_hall_test (three doors problem)
- [ ] Implement dice_sum_test (sum of two dice)
- [ ] Implement recursive_sequence_test (Fibonacci-like)
- [ ] Verify results match expected rank distributions
- [ ] All tests passing
- [ ] **COMMIT**: "Add integration tests for core scenarios"

### 5.2 Examples
**Files**: `examples/boolean_circuit.cpp`, `examples/recursion.cpp`

**Checklist**:
- [ ] Port boolean_circuit.rkt to C++
- [ ] Port recursion.rkt to C++
- [ ] Add README.md in examples/ with build instructions
- [ ] Verify examples compile and produce correct output
- [ ] **COMMIT**: "Add C++ example programs"

### 5.3 Documentation
**Files**: `README.md`, `docs/API.md`

**Checklist**:
- [ ] Write README.md with quickstart guide
- [ ] Document all public APIs with usage examples
- [ ] Configure Doxygen (Doxyfile)
- [ ] Generate HTML documentation
- [ ] **COMMIT**: "Complete core library documentation"

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
- [ ] Implement type-erased wrapper using std::any
- [ ] Implement type-erased versions of all operations
- [ ] Create `tests/type_erasure_test.cpp`:
  - Wrap various types
  - Type-erased operations
  - Type recovery
- [ ] All tests passing
- [ ] **COMMIT**: "Implement type erasure layer"

### 6.2 C API
**File**: `include/ranked_belief/c_api.h`, `src/c_api.cpp`

**Key Function Signatures**:
```cpp
// C API (extern "C")
typedef struct rb_ranking_t rb_ranking_t;
rb_ranking_t* rb_singleton_int(int value);
rb_ranking_t* rb_map(rb_ranking_t* rf, void* func, void* context);
void rb_ranking_free(rb_ranking_t* rf);
```

**Checklist**:
- [ ] Define opaque handle types
- [ ] Implement C wrappers for core operations
- [ ] Implement proper error handling (return codes)
- [ ] Create `tests/c_api_test.c`:
  - C language test file
  - Test all C API functions
  - Memory leak verification
- [ ] All tests passing
- [ ] **COMMIT**: "Implement C API for FFI compatibility"

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

**Current Phase**: Phase 1 - Foundation
**Last Commit**: 24cff58 - Initialize project structure and implement Rank type
**Test Coverage**: 100% (66/66 tests passing for Rank)

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

Update this section as you progress through each phase.
