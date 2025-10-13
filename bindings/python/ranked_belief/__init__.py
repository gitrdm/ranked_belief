"""Python package exposing ranked belief bindings."""

from importlib import import_module as _import_module
from importlib import util as _importlib_util
from pathlib import Path as _Path
import sys as _sys
from typing import Any, Tuple, TYPE_CHECKING

_EXPORTED_NAMES: Tuple[str, ...] = (
    "Rank",
    "RankingFunctionInt",
    "RankingFunctionFloat",
    "RankingFunctionString",
)

if TYPE_CHECKING:  # pragma: no cover - assists static analysis
    from ._ranked_belief_core import (  # type: ignore[attr-defined]
        Rank,
        RankingFunctionFloat,
        RankingFunctionInt,
        RankingFunctionString,
    )

__all__ = [
    "__version__",
    "Rank",
    "RankingFunctionInt",
    "RankingFunctionFloat",
    "RankingFunctionString",
]
__version__ = "0.1.0"

# Attempt to import the compiled extension; defer failure until attribute access
_core_error: Exception | None = None
def _load_core_from_additional_paths() -> Any | None:
    package_name = __name__.split(".")[-1]
    extension_prefix = "_ranked_belief_core"
    for entry in _sys.path:
        if not entry:
            continue
        candidate_dir = _Path(entry) / package_name
        if not candidate_dir.is_dir():
            continue
        for suffix in (".so", ".pyd", ".dll", ".dylib"):
            matches = sorted(candidate_dir.glob(f"{extension_prefix}*{suffix}"))
            if not matches:
                continue
            spec = _importlib_util.spec_from_file_location(
                f"{__name__}.{extension_prefix}", matches[0]
            )
            if spec and spec.loader:
                module = _importlib_util.module_from_spec(spec)
                spec.loader.exec_module(module)
                _sys.modules[spec.name] = module
                return module
    return None


try:
    _core = _import_module("._ranked_belief_core", __name__)
except ModuleNotFoundError:  # pragma: no cover - helps when module is not yet built
    _core = _load_core_from_additional_paths()
except Exception as exc:  # pragma: no cover - surfacing linker/runtime errors lazily
    _core = None
    _core_error = exc

if _core is not None:
    globals().update({name: getattr(_core, name) for name in _EXPORTED_NAMES})


def __getattr__(name: str) -> Any:
    """Expose symbols from the compiled extension lazily."""

    if name in _EXPORTED_NAMES:
        if _core is None:
            if _core_error is not None:
                raise ImportError(
                    "ranked_belief._ranked_belief_core failed to import due to a runtime error"
                ) from _core_error
            raise ImportError(
                "ranked_belief._ranked_belief_core failed to import; build the Python bindings "
                "by enabling the RANKED_BELIEF_BUILD_PYTHON_BINDINGS CMake option"
            ) from None
        return getattr(_core, name)
    raise AttributeError(f"module 'ranked_belief' has no attribute {name!r}")


def __dir__() -> list[str]:
    return sorted(set(__all__))
