# Ranked Belief Python Bindings

The Python package mirrors the full lazy semantics of the C++ ranked belief
library and now ships with a set of high-level helpers and example programs
ported from the original Racket distribution.  This document summarises how to
build, use, and extend the bindings.

## Building

Enable the bindings when configuring CMake:

```bash
cmake -S . -B build -DRANKED_BELIEF_BUILD_PYTHON_BINDINGS=ON
cmake --build build --target _ranked_belief_core
```

The build copies the pure-Python package into the binary tree so that the
compiled module, helper DSL, and examples can be imported without additional
packaging steps.

## Package Layout

```
ranked_belief/
    __init__.py          # Core exports (Rank, typed specialisations, RankingFunctionAny)
    dsl.py               # Convenience helpers mirroring the Racket DSL
    examples/            # Python ports of the ranked-programming examples
    tests/               # Pytest suite exercising the bindings and examples
```

### Core API

`ranked_belief.Rank` and the typed ranking functions (`RankingFunctionInt`,
`RankingFunctionFloat`, `RankingFunctionString`) expose the same lazy
operations as their C++ counterparts.  Phase 7.3 also introduces
`RankingFunctionAny`, a type-erased wrapper that stores Python objects lazily
and powers the DSL utilities and examples.

### DSL Helpers

The module `ranked_belief.dsl` ports the key ranked-programming combinators:

- `ensure_ranking` / `RankingFunctionAny.singleton`
- `normal_exceptional`, `either_of`, `either_or`
- `ranked_apply`, `merge_apply`, `observe`, `observe_value`
- `rlet_star` for sequential ranked bindings
- `take_n` to materialise finite prefixes while preserving laziness

These helpers make it straightforward to translate the Racket examples into
idiomatic Python while retaining deferred evaluation.

## Example Programs

Each example mirrors its Racket counterpart and is importable as a normal
Python module:

- `boolean_circuit.py` – Diagnoses faulty gate configurations.
- `recursion.py` – Demonstrates infinite rankings and conditional observation.
- `ranked_let.py` – Shows ranked let bindings with dependent variables.
- `ranked_procedure_call.py` – Illustrates ranked application of operators.
- `ranking_network.py` – Encodes the hazard/burglary/fire sensor network.
- `hidden_markov.py` – Hidden Markov model with ranked transitions/emissions.
- `localisation.py` – Robot localisation HMM using neighbourhood observations.
- `spelling_correction.py` – Trie-backed spelling corrector supporting
  wildcards.

The examples return rankings rather than printing to stdout so they can be
tested directly.  Most modules expose convenience helpers such as
`diagnose(...)`, `first_values(...)`, or `sequence_likelihoods(...)` to obtain
materialised prefixes for inspection.

## Running Tests

The pytest suite covers both the core bindings and the translated examples:

```bash
cmake --build build --target ranked_belief_python_tests
ctest --test-dir build -R ranked_belief_python_tests -V
```

Running `pytest` from `bindings/python/` manually is also supported.  The
CTest target configures `PYTHONPATH` so the in-tree package and the built
extension are discovered automatically.

## Extending

- Add new bindings in `src/module.cpp`, using the helper function
  `bind_ranking_function<T>` as a reference for laziness-preserving wrappers.
- Extend the DSL or examples by composing the provided helpers; every
  operation defers evaluation until results are forced, matching the C++ and
  Racket semantics.
- Remember to add tests under `bindings/python/tests/` for new behaviours and
  wire them into the existing pytest suite.

With these additions, Phase 7.3 is complete: the Python bindings now ship with
a feature-complete API, convenience DSL, comprehensive examples, and
integration tests mirroring the ranked-programming package.
