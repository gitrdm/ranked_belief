# Ranked Belief Python Bindings

This directory hosts the Python bindings for the ranked belief library. Phase 7.1 establishes the
build and test scaffolding:

- The bindings are built via [pybind11](https://pybind11.readthedocs.io/) and integrated with the
  project through CMake (`RANKED_BELIEF_BUILD_PYTHON_BINDINGS=ON`).
- A minimal extension module (`_ranked_belief_core`) is produced alongside a Python package
  namespace (`ranked_belief`).
- Pytest-based smoke tests live under `tests/` and are wired into CTest.

Subsequent phases will populate the module with the full ranked belief API surface.
