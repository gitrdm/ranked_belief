"""Pytest configuration ensuring the built extension is importable."""

from __future__ import annotations

import sys
from pathlib import Path

# Project root: bindings/python/tests/ -> bindings/python -> bindings -> repo root
_ROOT = Path(__file__).resolve().parents[3]
_SOURCE_PACKAGE = _ROOT / "bindings" / "python"
_BUILD_PACKAGE = _ROOT / "build" / "bindings" / "python"

if _BUILD_PACKAGE.exists():
    build_str = str(_BUILD_PACKAGE)
    if build_str not in sys.path:
        sys.path.append(build_str)

if _SOURCE_PACKAGE.exists():
    source_str = str(_SOURCE_PACKAGE)
    if source_str not in sys.path:
        sys.path.insert(0, source_str)