# Building and recommended workspace layout

This project historically accumulated multiple build directories (for example `build`, `build-test`, `build-fuzz`, `build-sanitizers`). That's useful during active development, but it can become confusing. This file documents a recommended, consistent approach and provides small helpers to make builds repeatable.

Recommended build presets
- `build` (or preset `default`) — standard RelWithDebInfo build with examples and library
- `build-debug` (preset `debug`) — debug build
- `build-test` (preset `tests`) — a build that enables the test-suite (GoogleTest) and CTest
- `build-sanitizers` (preset `sanitizers`) — build with Address/Undefined sanitizers (Ninja generator recommended)
- `build-fuzz` (preset `fuzz`) — libFuzzer targets built with Clang + sanitizers (Ninja recommended)

Why separate build trees?
- Different configurations (sanitizers, compiler choice, build-type, fuzzers) need different cache variables and sometimes different toolchains (Clang vs GCC). Re-configuring a single build directory repeatedly is slow and error-prone.
- Keep one build tree per orthogonal purpose (dev / tests / sanitizers / fuzz) and remove unused ones when not needed.

Generator (Ninja vs Make)
- The project can be built with either Unix Makefiles or Ninja. Some of the sanitizer/fuzzer workflows prefer Ninja because its incremental build is faster and it integrates well with clang tooling.
- Recommendation: prefer Ninja for `sanitizers` and `fuzz` presets (the CMakePresets here do that). Keep `default`/`tests` on Unix Makefiles for portability unless you and your CI prefer Ninja project-wide.

Cleaning up / consolidation
- If you want to consolidate and keep only the canonical set, remove the extra directories:

```bash
rm -rf build build-sanitizers build-test build-fuzz
```

Then use the presets to create the exact build you need (see below).

Quick usage
- Configure + build a preset (uses CMake 3.20+ presets):

```bash
cmake --preset fuzz
cmake --build --preset fuzz
```

- Helper script:

```bash
./scripts/build.sh fuzz
```

Notes
- Keep `fuzz` and `sanitizers` in separate build trees. They often need Clang and -fsanitize flags. Tests and normal builds can use GCC/Make if you prefer.
- If you adopt Ninja project-wide, update CMakePresets.json generator fields (or run `cmake -G Ninja -S . -B build ...`). Consistency is more important than which generator you choose.
