"""Python package exposing ranked belief bindings."""

from importlib import import_module as _import_module
from importlib.util import find_spec as _find_spec

__all__ = ["__version__"]
__version__ = "0.1.0"

# Attempt to import the compiled extension; defer failure until attribute access
if _find_spec("ranked_belief._ranked_belief_core") is not None:
    _core = _import_module("._ranked_belief_core", __name__)
else:  # pragma: no cover - helps when module is not yet built
    _core = None
