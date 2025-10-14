# ranked_belief

A modern C++20 port of the ranked programming calculus that models epistemic uncertainty using ranking functions with lazy semantics. The library provides fully lazy construction, transformation, and observation primitives that mirror the original Racket implementation while embracing idiomatic C++ abstractions.

## Features

- 📚 **Literate-style documentation** for every public API, surfaced through Markdown and generated Doxygen reference manuals.
- 💤 **Lazy evaluation everywhere**: constructors, higher-order transformations, and observations defer work until the caller requests values.
- 🧮 **Rich algebra of operations** covering map/filter/merge/merge-apply, observation, and normal/exceptional queries.
- 🧪 **Extensive automated test suite** (426 Google Tests) validating semantic fidelity and concurrency guarantees.
- 🛠️ **Examples and integration tests** that demonstrate ranked diagnosis problems and infinite lazy enumerations.

## Quickstart

### Prerequisites

- C++20 toolchain (GCC 10+, Clang 13+, or MSVC 19.29+)
- CMake 3.20 or newer
- Doxygen 1.9 or newer (for API documentation generation)

### Configure & Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build
```

### Run the Test Suite

```bash
ctest --test-dir build --output-on-failure
```

### Try the Examples

```bash
./build/examples/boolean_circuit_example
./build/examples/recursion_example
```

The boolean circuit example ranks explanations for an observed output, while the recursion example walks an infinite lazy ranking and reports how many nodes were forced.

### Generate API Documentation

```bash
doxygen Doxyfile
```

HTML documentation will be written to `build/docs/html/index.html`. Pair the generated reference with the curated guide in `docs/API.md` for rich usage examples.

## Repository Layout

- `include/` – Public headers with literate Doxygen comments.
- `src/` – (Reserved) Implementation source files.
- `tests/` – Google Test suite covering every feature.
- `examples/` – Executable demonstrations that showcase lazy semantics.
- `docs/` – Markdown guides and generated API documentation outputs.
- `CPP_PORT_DESIGN.md` – Design rationale for the C++ port.
- `ranked-programming/` – Original Racket implementation for reference.

## Contributing

1. Fork and clone the repository.
2. Create a feature branch from `master`.
3. Follow the checklist in `IMPLEMENTATION_CHECKLIST.md` and keep tests green.
4. Ensure Doxygen warnings are resolved before submitting a PR.

## License

This project is released under the MIT License. See `LICENSE` for details.
