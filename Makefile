.DEFAULT_GOAL := build

RUNS ?= 1000
FUZZ_TARGET := ./build-fuzz/fuzz/ranked_belief_fuzz

.PHONY: help build configure-default build-default tests configure-tests build-tests test \
	sanitizers configure-sanitizers build-sanitizers fuzz configure-fuzz build-fuzz-target \
	fuzz-run clean

help:
	@echo "Available targets:"
	@echo "  build             Configure and build the default preset"
	@echo "  tests             Configure and build the tests preset"
	@echo "  test              Build tests and run the test suite"
	@echo "  sanitizers        Configure and build with sanitizers preset"
	@echo "  fuzz              Configure and build the fuzzing preset"
	@echo "  fuzz-run          Build fuzz target (if needed) and run it (override RUNS=N)"
	@echo "  clean             Remove generated build directories"

build: build-default

configure-default:
	cmake -S . -B build \
		-DCMAKE_BUILD_TYPE=RelWithDebInfo \
		-DRANKED_BELIEF_BUILD_TESTS=ON \
		-DRANKED_BELIEF_BUILD_FUZZERS=OFF \
		-DRANKED_BELIEF_ENABLE_SANITIZERS=OFF

build-default: configure-default
	cmake --build build

configure-tests:
	cmake --preset tests

build-tests: configure-tests
	cmake --build --preset tests

# Backwards-compatible alias for build-tests
tests: build-tests

test: build-tests
	ctest --output-on-failure --test-dir build-test

configure-sanitizers:
	cmake --preset sanitizers

build-sanitizers: configure-sanitizers
	cmake --build --preset sanitizers

sanitizers: build-sanitizers

configure-fuzz:
	cmake --preset fuzz

build-fuzz-target: configure-fuzz
	cmake --build --preset fuzz

fuzz: build-fuzz-target

fuzz-run: build-fuzz-target
	$(FUZZ_TARGET) -runs=$(RUNS)

clean:
	rm -rf build build-test build-sanitizers build-fuzz
