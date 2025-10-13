# ranked_belief Examples

This directory contains the C++ ports of the ranked-programming paper examples.
Each program is intentionally small and heavily commented so it can double as a
living tutorial for the library's lazy semantics.

## Building

The examples are part of the top-level CMake project. After configuring the
repository (see the root `README`), build them with:

```bash
cmake --build build --target boolean_circuit_example recursion_example
```

## Running

Boolean circuit diagnosis (mirrors `boolean_circuit.rkt`):

```bash
./build/examples/boolean_circuit_example
```

Recursive ranking with infinite support (mirrors `recursion.rkt`):

```bash
./build/examples/recursion_example
```

Each program prints the highest ranked results and reports how much lazy
structure was forced, demonstrating that the C++ port preserves the deferred
execution guarantees of the original Racket code.
