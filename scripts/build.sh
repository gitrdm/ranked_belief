#!/usr/bin/env bash
set -euo pipefail

# Lightweight helper for common builds using CMake presets.
# Usage: ./scripts/build.sh <preset> [-- target]
# Examples:
#   ./scripts/build.sh fuzz
#   ./scripts/build.sh tests -- target

preset=${1:-default}
shift || true

echo "Configuring and building preset: $preset"
cmake --preset "$preset"
if [[ $# -gt 0 ]]; then
  cmake --build "${PWD}/$(jq -r --arg p "$preset" '.configurePresets[] | select(.name==$p) | .binaryDir' CMakePresets.json)" -- "$@"
else
  cmake --build --preset "$preset"
fi
