# ranked_belief

[![Documentation](https://img.shields.io/badge/docs-GitHub%20Pages-blue)](https://gitrdm.github.io/ranked_belief/)
[![Deploy Docs](https://github.com/gitrdm/ranked_belief/actions/workflows/deploy-docs.yml/badge.svg)](https://github.com/gitrdm/ranked_belief/actions/workflows/deploy-docs.yml)

A modern C++20 implementation of the ranked programming calculus that models epistemic uncertainty using ranking functions with fully lazy semantics. The library provides a complete algebra of lazy construction, transformation, and observation primitives that mirror the original Racket implementation while embracing idiomatic C++ abstractions.

## Overview

**ranked_belief** is a C++ library for probabilistic reasoning about epistemic uncertainty using ranking theory. It supports:

- **Lazy evaluation** throughout the computation graph - values are only computed when explicitly forced
- **Compositional abstractions** via map, filter, merge, and monadic bind (merge-apply)
- **Infinite structures** that can be lazily traversed and sampled
- **Thread-safe memoization** ensuring computations execute exactly once
- **Type erasure** for polymorphic ranking functions over heterogeneous types

The library consists of:

1. **Core C++ Library** - Header-only template library with full type safety and zero-cost abstractions
2. **C API** - Stable ABI for language interoperability (integer rankings only)
3. **Language Bindings** - Proof-of-concept Python and R bindings demonstrating C API usage

## Features

- 📚 **Literate-style documentation** - Every public API documented with Doxygen comments
- 💤 **Lazy evaluation everywhere** - Constructors, transformations, and observations defer work until values are requested
- 🧮 **Rich algebra of operations** - map/filter/merge/merge-apply, observation, normal/exceptional combinators
- 🧪 **Comprehensive test suite** - 426+ Google Tests validating semantic fidelity and concurrency guarantees
- 🛠️ **Examples and integration tests** - Demonstrations of ranked diagnosis, HMMs, and infinite lazy enumerations
- 🌐 **Language interoperability** - C API with Python and R bindings (proof-of-concept)

## Quickstart

### Prerequisites

- **C++20 toolchain** (GCC 10+, Clang 13+, or MSVC 19.29+)
  - Required C++20 features: concepts, `<=>` operator, `requires` clauses, `constexpr` improvements
  - Key usage: Type constraints via concepts, three-way comparison for `Rank`, SFINAE replacement with requires
  - *Note:* C++17 will not work due to missing concepts and spaceship operator support
- CMake 3.20 or newer
- Doxygen 1.9+ (optional, for API documentation)
- Python 3.8+ with pybind11 (optional, for Python bindings)
- R 4.0+ (optional, for R bindings)

### Configure & Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build
```

For detailed build configurations (debug, tests, sanitizers, fuzzing), see [BUILDING.md](BUILDING.md).

### Run the Test Suite

```bash
# C++ tests
ctest --test-dir build --output-on-failure

# Python tests (if bindings are built)
cd bindings/python
pytest tests/

# R tests (if installed)
cd bindings/rankedBeliefR
Rscript -e "library(testthat); library(rankedBeliefR); test_dir('tests/testthat')"
```

### Try the Examples

```bash
# C++ examples
./build/examples/boolean_circuit_example
./build/examples/recursion_example

# Python examples
cd bindings/python
python -m ranked_belief.examples.boolean_circuit

# R examples
cd bindings/rankedBeliefR/inst/examples
Rscript run_examples.R
```

The boolean circuit example ranks fault explanations for an observed output, while the recursion example demonstrates infinite lazy rankings.

### Generate API Documentation

```bash
make docs      # Generate Doxygen documentation
make show-docs # Generate and open in browser
```

HTML documentation will be written to `build/docs/html/index.html`. 

**Online Documentation**: The latest API documentation is automatically published to [GitHub Pages](https://gitrdm.github.io/ranked_belief/).

The generated reference complements the curated guide in `docs/API.md`.

## Core C++ Library

The header-only C++ library provides the full power of ranked belief computations with compile-time type safety:

```cpp
#include <ranked_belief/ranking_function.hpp>
#include <ranked_belief/operations/merge_apply.hpp>

using namespace ranked_belief;

// Create a normal/exceptional ranking
auto value_dist = normal_exceptional(
    singleton(10),  // normally 10 at rank 0
    singleton(20),  // exceptionally 20 at rank 1
    Rank::from_value(1)
);

// Compose computations with merge_apply (monadic bind)
auto result = merge_apply(value_dist, [](int v) {
    return singleton(v + 5);
});

// Sample the first few results
for (auto it = result.begin(); it != result.end(); ++it) {
    std::cout << it->value << " @ rank " << it->rank << "\n";
    if (it->rank > Rank::from_value(5)) break;
}
```

See `docs/API.md` for comprehensive usage patterns.

## C API

The C API (`include/ranked_belief/c_api.h`) provides a stable ABI for language interoperability. It supports:

- Integer-valued rankings only (no type erasure overhead)
- Lazy map/filter/merge/merge-apply operations
- Callback-based transformations
- Observation and conditioning

**Status:** Production-ready for integer rankings. Used by Python and R bindings.

## Language Bindings (Proof-of-Concept)

### Python Bindings

The Python bindings (`bindings/python/`) demonstrate the C API in action with a Pythonic interface:

```python
from ranked_belief import normal_exceptional, merge_apply, take_n

# Create ranked distributions
value = normal_exceptional(10, 20)  # normally 10, exceptionally 20
op = normal_exceptional(lambda x: x + 5, lambda x: x - 5)

# Compose with merge_apply
result = merge_apply(value, op)

# Sample results
print(take_n(result, 4))  # [(15, 0), (25, 1), (5, 1), (15, 2)]
```

**Status:** Proof-of-concept demonstrating C API usage. Includes examples mirroring the original Racket implementation.

### R Bindings

The R bindings (`bindings/rankedBeliefR/`) provide an R package wrapping the C API:

```r
library(rankedBeliefR)

# Create ranked distributions
ops <- rb_from_array_int(values = c(0L, 1L), ranks = c(0, 1))
values <- rb_from_array_int(values = c(10L, 20L), ranks = c(0, 1))

# Compose with rb_merge_apply_int
result <- rb_merge_apply_int(ops, function(op) {
  rb_merge_apply_int(values, function(val) {
    if (op == 0L) rb_singleton_int(val + 5L)
    else rb_singleton_int(val - 5L)
  })
})

# Sample results
rb_take_n_int(result, 4)
```

**Status:** Proof-of-concept with 27 passing tests. 

**Note:** Both Python and R bindings are intended to **exercise and validate the C API**, not as production-ready interfaces. They serve as:
- Integration tests for the C API
- Reference implementations for language binding authors
- Examples of idiomatic usage patterns in each language

For production use, the bindings would benefit from:
- Expanded type support (strings, floats, custom types)
- Performance optimizations (batch operations, vectorization)
- Comprehensive documentation and tutorials
- Package distribution (PyPI, CRAN)

## Examples

The repository includes examples in C++, Python, and R demonstrating ranked programming patterns:

- **Boolean Circuit Diagnosis** - Rank fault explanations by plausibility
- **Hidden Markov Models** - Forward algorithm with ranked state sequences
- **Robot Localization** - Bayesian filtering with ranked beliefs
- **Spelling Correction** - Dictionary lookup with edit distance ranking
- **Network Propagation** - Ranked path enumeration
- **Recursive Structures** - Infinite lazy doubling sequences

Each example is ported across all three languages for comparison.

## Contributing

1. Fork and clone the repository
2. Create a feature branch from `master`
3. Follow the checklist in `IMPLEMENTATION_CHECKLIST.md` and keep tests green
4. Ensure Doxygen warnings are resolved before submitting a PR
5. Add tests for new functionality

### Development Workflow

```bash
# Configure with tests enabled
cmake --preset tests

# Build and run tests
cmake --build build-test
ctest --test-dir build-test --output-on-failure

# Build with sanitizers for debugging
cmake --preset sanitizers
cmake --build build-sanitizers
./build-sanitizers/examples/boolean_circuit_example
```

## References

- Original Racket implementation: `ranked-programming/` (https://github.com/tjitze/ranked-programming.git)
- Design rationale: `CPP_PORT_DESIGN.md`
- API documentation: `docs/API.md`
- Build guide: `BUILDING.md`

## License

This project is released under the MIT License. See `LICENSE` for details.
