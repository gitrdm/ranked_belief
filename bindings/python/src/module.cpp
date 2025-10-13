#include <pybind11/pybind11.h>

namespace py = pybind11;

PYBIND11_MODULE(_ranked_belief_core, m)
{
    m.doc() = "Ranked belief Python bindings (Phase 7.1 scaffold).";
}
