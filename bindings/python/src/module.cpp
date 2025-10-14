#include <pybind11/functional.h>
#include <pybind11/operators.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <any>
#include <optional>
#include <sstream>
#include <string>
#include <typeindex>
#include <utility>
#include <vector>

#include "ranked_belief/constructors.hpp"
#include "ranked_belief/operations/filter.hpp"
#include "ranked_belief/operations/map.hpp"
#include "ranked_belief/operations/merge.hpp"
#include "ranked_belief/operations/merge_apply.hpp"
#include "ranked_belief/operations/nrm_exc.hpp"
#include "ranked_belief/operations/observe.hpp"
#include "ranked_belief/rank.hpp"
#include "ranked_belief/ranking_function.hpp"
#include "ranked_belief/detail/any_equality_registry.hpp"
#include "ranked_belief/type_erasure.hpp"

namespace py = pybind11;
namespace rb = ranked_belief;

namespace {

struct PyObjectDeleter {
    void operator()(PyObject* ptr) const noexcept
    {
        if (ptr != nullptr) {
            py::gil_scoped_acquire gil;
            Py_DECREF(ptr);
        }
    }
};

using PyObjectPtr = std::shared_ptr<PyObject>;

[[nodiscard]] PyObjectPtr make_py_object_ptr(py::handle handle)
{
    PyObject* raw = handle.ptr();
    if (raw == Py_None) {
        return {};
    }
    py::gil_scoped_acquire gil;
    Py_INCREF(raw);
    return PyObjectPtr(raw, PyObjectDeleter{});
}

struct PyValue {
    py::object object;

    explicit PyValue(py::handle handle)
        : object(py::reinterpret_borrow<py::object>(handle))
    {}

    explicit PyValue(py::object obj)
        : object(std::move(obj))
    {}
};

[[nodiscard]] bool operator==(const PyValue& lhs, const PyValue& rhs)
{
    py::gil_scoped_acquire gil;
    int result = PyObject_RichCompareBool(lhs.object.ptr(), rhs.object.ptr(), Py_EQ);
    if (result == -1) {
        throw py::error_already_set();
    }
    return result == 1;
}

[[maybe_unused]] const bool registered_pyvalue_equality = [] {
    rb::detail::register_any_equality(
        std::type_index(typeid(PyValue)),
        [](const std::any& lhs, const std::any& rhs) {
            return std::any_cast<const PyValue&>(lhs) == std::any_cast<const PyValue&>(rhs);
        });
    return true;
}();

[[nodiscard]] std::any py_to_any(py::handle handle)
{
    return std::any{PyValue(handle)};
}

[[nodiscard]] py::object any_to_py(const std::any& value)
{
    if (value.type() == typeid(PyValue)) {
        const auto& wrapped = std::any_cast<const PyValue&>(value);
        return wrapped.object;
    }
    if (value.type() == typeid(PyObjectPtr)) {
        const auto& ptr = std::any_cast<const PyObjectPtr&>(value);
        if (!ptr) {
            return py::none();
        }
        py::gil_scoped_acquire gil;
        Py_INCREF(ptr.get());
        return py::reinterpret_steal<py::object>(ptr.get());
    }
    if (value.type() == typeid(std::nullptr_t)) {
        return py::none();
    }
    if (value.type() == typeid(bool)) {
        return py::bool_(std::any_cast<bool>(value));
    }
    if (value.type() == typeid(int)) {
        return py::int_(std::any_cast<int>(value));
    }
    if (value.type() == typeid(long)) {
        return py::int_(std::any_cast<long>(value));
    }
    if (value.type() == typeid(long long)) {
        return py::int_(std::any_cast<long long>(value));
    }
    if (value.type() == typeid(unsigned long)) {
        return py::int_(std::any_cast<unsigned long>(value));
    }
    if (value.type() == typeid(unsigned long long)) {
        return py::int_(std::any_cast<unsigned long long>(value));
    }
    if (value.type() == typeid(double)) {
        return py::float_(std::any_cast<double>(value));
    }
    if (value.type() == typeid(float)) {
        return py::float_(std::any_cast<float>(value));
    }
    if (value.type() == typeid(std::string)) {
        return py::str(std::any_cast<const std::string&>(value));
    }
    if (value.type() == typeid(const char*)) {
        return py::str(std::any_cast<const char*>(value));
    }
    throw py::type_error("Unsupported value stored in RankingFunctionAny; expected a Python-managed object");
}

[[nodiscard]] std::vector<std::pair<std::any, rb::Rank>> parse_value_rank_pairs(const py::iterable& pairs)
{
    std::vector<std::pair<std::any, rb::Rank>> result;
    for (auto item : pairs) {
        auto tuple = py::reinterpret_borrow<py::sequence>(item);
        if (py::len(tuple) != 2) {
            throw py::value_error("Expected (value, Rank) pairs");
        }
        auto rank_obj = tuple[1];
        result.emplace_back(py_to_any(tuple[0]), rank_obj.cast<rb::Rank>());
    }
    return result;
}

[[nodiscard]] py::list pairs_to_python_list(const std::vector<std::pair<std::any, rb::Rank>>& pairs)
{
    py::list result;
    for (const auto& [value, rank] : pairs) {
        result.append(py::make_tuple(any_to_py(value), rank));
    }
    return result;
}

template<typename T>
void bind_ranking_function(py::module_& m, const char* class_name, const char* value_doc)
{
    using RF = rb::RankingFunction<T>;

    std::ostringstream class_doc_stream;
    class_doc_stream
        << "Lazy ranking of " << value_doc << " values.\n\n"
        << "This class mirrors the C++ ``RankingFunction<" << value_doc
        << ">`` with full lazy semantics: elements are only materialised when accessed "
        << "via iteration or query helpers. Instances behave like immutable, read-only "
        << "sequences of ``(value, Rank)`` pairs.";
    const std::string class_doc = class_doc_stream.str();

    py::class_<RF>(m, class_name, class_doc.c_str())
        .def(py::init<>(),
            R"pbdoc(
Create an empty ranking.

The resulting ranking has no elements. It can serve as a neutral element
in merge operations and remains fully lazy.
)pbdoc")
        .def("is_empty", &RF::is_empty,
            R"pbdoc(
Check whether the ranking has any elements.

Returns
-------
bool
    ``True`` when the ranking has no elements, ``False`` otherwise.
)pbdoc")
        .def("is_deduplicating", &RF::is_deduplicating,
            R"pbdoc(
Report whether consecutive equal values are deduplicated during iteration.

Deduplication is a construction-time flag that preserves the first
occurrence (lowest rank) of a value while skipping later duplicates.
)pbdoc")
        .def("first",
            [](const RF& self) -> std::optional<std::pair<T, rb::Rank>> {
                return self.first();
            },
            R"pbdoc(
Return the most normal element if one exists.

The result is ``None`` for empty rankings; otherwise a tuple of ``(value, Rank)``.
)pbdoc")
        .def("size", &RF::size,
            R"pbdoc(
Count the number of elements in the ranking.

Notes
-----
This operation forces the entire sequence. It should only be used on
finite rankings, as infinite rankings will never terminate.
)pbdoc")
        .def("__iter__",
            [](const RF& self) {
                return py::make_iterator(self.begin(), self.end());
            },
            py::keep_alive<0, 1>(),
            R"pbdoc(
Iterate lazily over ``(value, Rank)`` pairs.

Only the traversed prefix is forced. Subsequent elements remain lazy until
requested, preserving the underlying promise-based evaluation strategy.
)pbdoc")
        .def("map",
            [](const RF& rf, py::function func, bool deduplicate) {
                auto func_handle = make_py_object_ptr(func);
                if (!func_handle) {
                    throw py::value_error("map expects a callable");
                }
                auto transformer = [func_handle](const T& value) -> T {
                    py::gil_scoped_acquire gil;
                    py::function callable = py::reinterpret_borrow<py::function>(func_handle.get());
                    return callable(value).template cast<T>();
                };
                return rb::map<T>(rf, std::move(transformer), deduplicate);
            },
            py::arg("func"),
            py::arg("deduplicate") = true,
            R"pbdoc(
Map a Python callable across values while preserving ranks lazily.

The callable runs inside a promise and is only evaluated for elements that
are forced (for example via ``first()`` or iteration). Results are memoised
after the first evaluation.
)pbdoc")
        .def("filter",
            [](const RF& rf, py::function predicate, bool deduplicate) {
                auto predicate_handle = make_py_object_ptr(predicate);
                if (!predicate_handle) {
                    throw py::value_error("filter expects a callable predicate");
                }
                auto pred = [predicate_handle](const T& value) {
                    py::gil_scoped_acquire gil;
                    py::function callable = py::reinterpret_borrow<py::function>(predicate_handle.get());
                    return callable(value).template cast<bool>();
                };
                return rb::filter<T>(rf, std::move(pred), deduplicate);
            },
            py::arg("predicate"),
            py::arg("deduplicate") = true,
            R"pbdoc(
Filter values using a Python predicate lazily.

The predicate is evaluated only for elements that need to be inspected to
produce the filtered ranking. Ranks of retained elements remain unchanged.
)pbdoc")
        .def("take",
            [](const RF& rf, std::size_t count, bool deduplicate) {
                return rb::take<T>(rf, count, deduplicate);
            },
            py::arg("count"),
            py::arg("deduplicate") = true,
            R"pbdoc(
Take at most *count* elements from the ranking.

The resulting ranking shares the original laziness: elements beyond the
requested prefix remain unevaluated.
)pbdoc")
        .def("take_while_rank",
            [](const RF& rf, const rb::Rank& max_rank, bool deduplicate) {
                return rb::take_while_rank<T>(rf, max_rank, deduplicate);
            },
            py::arg("max_rank"),
            py::arg("deduplicate") = true,
            R"pbdoc(
Retain elements whose rank is at most ``max_rank``.
)pbdoc")
        .def("merge",
            [](const RF& rf, const RF& other, bool deduplicate) {
                return rb::merge<T>(rf, other, deduplicate);
            },
            py::arg("other"),
            py::arg("deduplicate") = true,
            R"pbdoc(
Merge two rankings by ascending rank while preserving laziness.
)pbdoc")
        .def_static("merge_all",
            [](const std::vector<RF>& rankings, bool deduplicate) {
                return rb::merge_all<T>(rankings, deduplicate);
            },
            py::arg("rankings"),
            py::arg("deduplicate") = true,
            R"pbdoc(
Merge an iterable of rankings in rank order.
)pbdoc")
        .def("merge_apply",
            [](const RF& rf, py::function func, bool deduplicate) {
                auto func_handle = make_py_object_ptr(func);
                if (!func_handle) {
                    throw py::value_error("merge_apply expects a callable");
                }
                auto binder = [func_handle](const T& value) -> RF {
                    py::gil_scoped_acquire gil;
                    py::function callable = py::reinterpret_borrow<py::function>(func_handle.get());
                    return callable(value).template cast<RF>();
                };
                return rb::merge_apply<T>(rf, std::move(binder), deduplicate);
            },
            py::arg("func"),
            py::arg("deduplicate") = true,
            R"pbdoc(
Apply a function returning rankings and merge all results lazily.
)pbdoc")
        .def("observe",
            [](const RF& rf, py::function predicate, bool deduplicate) {
                auto predicate_handle = make_py_object_ptr(predicate);
                if (!predicate_handle) {
                    throw py::value_error("observe expects a callable predicate");
                }
                auto pred = [predicate_handle](const T& value) {
                    py::gil_scoped_acquire gil;
                    py::function callable = py::reinterpret_borrow<py::function>(predicate_handle.get());
                    return callable(value).template cast<bool>();
                };
                return rb::observe<T>(rf, std::move(pred), deduplicate);
            },
            py::arg("predicate"),
            py::arg("deduplicate") = true,
            R"pbdoc(
Condition the ranking on values satisfying ``predicate`` and renormalise
ranks so the most normal surviving element becomes rank 0.
)pbdoc")
        .def("observe_value",
            [](const RF& rf, const T& value, bool deduplicate) {
                return rb::observe<T>(rf, value, deduplicate);
            },
            py::arg("value"),
            py::arg("deduplicate") = true,
            R"pbdoc(
Condition the ranking on equality with ``value`` and renormalise ranks.
)pbdoc")
        .def("most_normal",
            [](const RF& rf) -> std::optional<T> {
                return rb::most_normal<T>(rf);
            },
            R"pbdoc(
Return the value with the lowest rank, or ``None`` if empty.
)pbdoc")
        .def("materialize",
            [](const RF& rf, std::size_t count) {
                return rb::take_n<T>(rf, count);
            },
            py::arg("count"),
            R"pbdoc(
Materialise at most ``count`` elements into a Python list of ``(value, Rank)`` pairs.

Only the requested prefix is forced; the remainder remains lazy.
)pbdoc",
            py::call_guard<py::gil_scoped_release>())
        .def("__bool__", [](const RF& rf) { return !rf.is_empty(); })
        .def_static("from_list",
            [](const std::vector<std::pair<T, rb::Rank>>& pairs, bool deduplicate) {
                return rb::from_list<T>(pairs, deduplicate);
            },
            py::arg("pairs"),
            py::arg("deduplicate") = true,
            R"pbdoc(
Construct a ranking from explicit ``(value, Rank)`` pairs.
)pbdoc")
        .def_static("from_values_uniform",
            [](const std::vector<T>& values, const rb::Rank& rank, bool deduplicate) {
                return rb::from_values_uniform<T>(values, rank, deduplicate);
            },
            py::arg("values"),
            py::arg("rank") = rb::Rank::zero(),
            py::arg("deduplicate") = true,
            R"pbdoc(
Construct a ranking where every value receives the same rank.
)pbdoc")
        .def_static("from_values_sequential",
            [](const std::vector<T>& values, const rb::Rank& start_rank, bool deduplicate) {
                return rb::from_values_sequential<T>(values, start_rank, deduplicate);
            },
            py::arg("values"),
            py::arg("start_rank") = rb::Rank::zero(),
            py::arg("deduplicate") = true,
            R"pbdoc(
Construct a ranking assigning increasing ranks starting at ``start_rank``.
)pbdoc")
        .def_static("from_generator",
            [](py::function generator, std::size_t start_index, bool deduplicate) {
                auto generator_handle = make_py_object_ptr(generator);
                if (!generator_handle) {
                    throw py::value_error("from_generator expects a callable");
                }
                auto gen = [generator_handle](std::size_t index) {
                    py::gil_scoped_acquire gil;
                    py::function callable = py::reinterpret_borrow<py::function>(generator_handle.get());
                    return callable(index).template cast<std::pair<T, rb::Rank>>();
                };
                return rb::from_generator<T>(std::move(gen), start_index, deduplicate);
            },
            py::arg("generator"),
            py::arg("start_index") = 0,
            py::arg("deduplicate") = true,
            R"pbdoc(
Construct a (potentially infinite) ranking from a Python generator function.
)pbdoc")
        .def_static("singleton",
            [](const T& value, const rb::Rank& rank) {
                return rb::singleton<T>(value, rank);
            },
            py::arg("value"),
            py::arg("rank") = rb::Rank::zero(),
            R"pbdoc(
Create a ranking containing a single value at the given rank.
)pbdoc");
}

} // namespace

PYBIND11_MODULE(_ranked_belief_core, m)
{
    m.doc() = R"pbdoc(
Core Python bindings for the ranked belief library.

The module exposes the ``Rank`` type along with specialised ranking
containers for ``int``, ``float`` (double), and ``str`` values. All bindings
mirror the C++ library's lazy semantics, deferring computation until results
are observed from Python.
)pbdoc";

    py::class_<rb::Rank>(m, "Rank", R"pbdoc(
Type-safe rank value used to measure normality or exceptionality.

Ranks are comparable, support arithmetic with overflow checking, and
include special handling for infinity to represent impossible outcomes.
)pbdoc")
        .def(py::init<>(), R"pbdoc(Create a rank with value zero.)pbdoc")
        .def_static("zero", &rb::Rank::zero, R"pbdoc(Return the most normal rank (0).)pbdoc")
        .def_static("infinity", &rb::Rank::infinity, R"pbdoc(Return the infinite rank representing impossibility.)pbdoc")
        .def_static("from_value", &rb::Rank::from_value, py::arg("value"),
            R"pbdoc(Create a rank from a finite non-negative integer value.)pbdoc")
        .def_static("max_finite_value", &rb::Rank::max_finite_value,
            R"pbdoc(Return the largest finite rank representable.)pbdoc")
        .def("is_infinity", &rb::Rank::is_infinity,
            R"pbdoc(Check whether this rank represents infinity.)pbdoc")
        .def("is_finite", &rb::Rank::is_finite,
            R"pbdoc(Check whether this rank is finite.)pbdoc")
        .def("value", &rb::Rank::value,
            R"pbdoc(Return the numeric value of a finite rank.)pbdoc")
        .def("value_or", &rb::Rank::value_or, py::arg("default_value"),
            R"pbdoc(Return the numeric value or ``default_value`` when infinite.)pbdoc")
        .def("min", &rb::Rank::min, py::arg("other"),
            R"pbdoc(Return the smaller of two ranks.)pbdoc")
        .def("max", &rb::Rank::max, py::arg("other"),
            R"pbdoc(Return the larger of two ranks.)pbdoc")
        .def("__repr__",
            [](const rb::Rank& rank) {
                std::ostringstream os;
                if (rank.is_infinity()) {
                    os << "Rank(infinity)";
                } else {
                    os << "Rank(" << rank.value() << ")";
                }
                return os.str();
            })
        .def("__str__",
            [](const rb::Rank& rank) {
                if (rank.is_infinity()) {
                    return std::string("∞");
                }
                return std::to_string(rank.value());
            })
        .def("__int__",
            [](const rb::Rank& rank) {
                if (rank.is_infinity()) {
                    throw py::type_error("Cannot convert infinite rank to int");
                }
                return static_cast<long long>(rank.value());
            })
        .def(py::self + py::self)
        .def(py::self - py::self)
        .def(py::self == py::self)
        .def(py::self != py::self)
        .def(py::self < py::self)
        .def(py::self <= py::self)
        .def(py::self > py::self)
        .def(py::self >= py::self);

    py::class_<rb::RankingFunctionAny>(m, "RankingFunctionAny", R"pbdoc(
Type-erased lazy ranking over arbitrary Python values.

The wrapper preserves the deferred-evaluation semantics of the native C++
implementation while storing elements as Python objects. Operations never
force values prematurely; computation occurs only when results are observed
from Python.
)pbdoc")
        .def(py::init<>(), R"pbdoc(
Create an empty ranking with no elements.
)pbdoc")
        .def_static("singleton",
            [](py::object value, const rb::Rank& rank) {
                auto native = rb::singleton<std::any>(py_to_any(value), rank);
                return rb::RankingFunctionAny{std::move(native)};
            },
            py::arg("value"),
            py::arg("rank") = rb::Rank::zero(),
            R"pbdoc(Construct a ranking containing a single Python value.)pbdoc")
        .def_static("from_list",
            [](py::iterable pairs, bool deduplicate) {
                auto native_pairs = parse_value_rank_pairs(pairs);
                auto native = rb::from_list<std::any>(std::move(native_pairs), deduplicate);
                return rb::RankingFunctionAny{std::move(native)};
            },
            py::arg("pairs"),
            py::arg("deduplicate") = true,
            R"pbdoc(Construct a ranking from explicit ``(value, Rank)`` pairs of Python objects.)pbdoc")
        .def_static("from_generator",
            [](py::function generator, std::size_t start_index, bool deduplicate) {
                auto native = rb::from_generator<std::any>(
                    [generator = std::move(generator)](std::size_t index) {
                        py::gil_scoped_acquire gil;
                        py::object result = generator(index);
                        auto tuple = result.cast<py::sequence>();
                        if (py::len(tuple) != 2) {
                            throw py::value_error("Generator must yield (value, Rank) tuples");
                        }
                        auto rank_obj = tuple[1];
                        return std::make_pair(py_to_any(tuple[0]), rank_obj.cast<rb::Rank>());
                    },
                    start_index,
                    deduplicate);
                return rb::RankingFunctionAny{std::move(native)};
            },
            py::arg("generator"),
            py::arg("start_index") = 0,
            py::arg("deduplicate") = true,
            R"pbdoc(Construct a ranking lazily from a Python generator of ``(value, Rank)`` tuples.)pbdoc")
        .def("is_empty", &rb::RankingFunctionAny::is_empty,
            R"pbdoc(Return ``True`` when the ranking contains no elements.)pbdoc")
        .def("first",
            [](const rb::RankingFunctionAny& self)
                -> std::optional<std::pair<py::object, rb::Rank>> {
                auto rank = self.first_rank();
                if (!rank) {
                    return std::nullopt;
                }
                return std::make_optional(std::make_pair(any_to_py(self.first_value()), *rank));
            },
            R"pbdoc(Return the most normal element as ``(value, Rank)`` or ``None`` if empty.)pbdoc")
        .def("map",
            [](const rb::RankingFunctionAny& self, py::function func, bool deduplicate) {
                auto func_handle = make_py_object_ptr(func);
                if (!func_handle) {
                    throw py::value_error("map expects a callable");
                }
                auto mapper = [func_handle](const std::any& value) -> std::any {
                    if (!func_handle) {
                        throw py::value_error("map callable is no longer available");
                    }
                    py::gil_scoped_acquire gil;
                    py::function callable = py::reinterpret_borrow<py::function>(func_handle.get());
                    py::object transformed = callable(any_to_py(value));
                    return py_to_any(std::move(transformed));
                };
                try {
                    return self.map(mapper, deduplicate);
                } catch (const std::logic_error& error) {
                    throw py::value_error(error.what());
                }
            },
            py::arg("func"),
            py::arg("deduplicate") = true,
            R"pbdoc(Map a Python callable lazily across all values.)pbdoc")
        .def("map_with_rank",
            [](const rb::RankingFunctionAny& self, py::function func, bool deduplicate) {
                auto func_handle = make_py_object_ptr(func);
                if (!func_handle) {
                    throw py::value_error("map_with_rank expects a callable");
                }
                auto mapper = [func_handle](const std::any& value, rb::Rank rank) {
                    if (!func_handle) {
                        throw py::value_error("map_with_rank callable is no longer available");
                    }
                    py::gil_scoped_acquire gil;
                    py::function callable = py::reinterpret_borrow<py::function>(func_handle.get());
                    py::object result = callable(any_to_py(value), rank);
                    auto tuple = result.cast<py::sequence>();
                    if (py::len(tuple) != 2) {
                        throw py::value_error("map_with_rank callback must return (value, Rank)");
                    }
                    auto rank_obj = tuple[1];
                    return std::make_pair(py_to_any(tuple[0]), rank_obj.cast<rb::Rank>());
                };
                try {
                    return self.map_with_rank(mapper, deduplicate);
                } catch (const std::logic_error& error) {
                    throw py::value_error(error.what());
                }
            },
            py::arg("func"),
            py::arg("deduplicate") = true,
            R"pbdoc(Map a callable that can adjust both value and rank lazily.)pbdoc")
        .def("map_with_index",
            [](const rb::RankingFunctionAny& self, py::function func, bool deduplicate) {
                auto func_handle = make_py_object_ptr(func);
                if (!func_handle) {
                    throw py::value_error("map_with_index expects a callable");
                }
                auto mapper = [func_handle](const std::any& value, std::size_t index) -> std::any {
                    if (!func_handle) {
                        throw py::value_error("map_with_index callable is no longer available");
                    }
                    py::gil_scoped_acquire gil;
                    py::function callable = py::reinterpret_borrow<py::function>(func_handle.get());
                    py::object transformed = callable(any_to_py(value), index);
                    return py_to_any(std::move(transformed));
                };
                try {
                    return self.map_with_index(mapper, deduplicate);
                } catch (const std::logic_error& error) {
                    throw py::value_error(error.what());
                }
            },
            py::arg("func"),
            py::arg("deduplicate") = true,
            R"pbdoc(Lazily map values while exposing their zero-based index.)pbdoc")
        .def("filter",
            [](const rb::RankingFunctionAny& self, py::function predicate, bool deduplicate) {
                auto predicate_handle = make_py_object_ptr(predicate);
                if (!predicate_handle) {
                    throw py::value_error("filter expects a callable predicate");
                }
                auto pred = [predicate_handle](const std::any& value) {
                    if (!predicate_handle) {
                        throw py::value_error("filter predicate is no longer available");
                    }
                    py::gil_scoped_acquire gil;
                    py::function callable = py::reinterpret_borrow<py::function>(predicate_handle.get());
                    return callable(any_to_py(value)).template cast<bool>();
                };
                try {
                    return self.filter(pred, deduplicate);
                } catch (const std::logic_error& error) {
                    throw py::value_error(error.what());
                }
            },
            py::arg("predicate"),
            py::arg("deduplicate") = true,
            R"pbdoc(Filter the ranking with a Python predicate while preserving laziness.)pbdoc")
        .def("take",
            [](const rb::RankingFunctionAny& self, std::size_t count) {
                return self.take(count);
            },
            py::arg("count"),
            R"pbdoc(Create a new ranking containing at most ``count`` elements.)pbdoc")
        .def("take_while_rank",
            [](const rb::RankingFunctionAny& self, const rb::Rank& max_rank) {
                return self.take_while_rank(max_rank);
            },
            py::arg("max_rank"),
            R"pbdoc(Keep elements whose rank does not exceed ``max_rank``.)pbdoc")
        .def("merge",
            [](const rb::RankingFunctionAny& self, const rb::RankingFunctionAny& other, bool deduplicate) {
                try {
                    return self.merge(other, deduplicate);
                } catch (const std::logic_error& error) {
                    throw py::value_error(error.what());
                }
            },
            py::arg("other"),
            py::arg("deduplicate") = true,
            R"pbdoc(Merge two rankings, interleaving elements by ascending rank.)pbdoc")
        .def_static("merge_all",
            [](py::iterable rankings, bool deduplicate) {
                std::vector<rb::RankingFunctionAny> native;
                for (auto item : rankings) {
                    native.push_back(py::cast<rb::RankingFunctionAny>(item));
                }
                return rb::RankingFunctionAny::merge_all(native, deduplicate);
            },
            py::arg("rankings"),
            py::arg("deduplicate") = true,
            R"pbdoc(Merge an iterable of rankings into a single ranking.)pbdoc")
        .def("merge_apply",
            [](const rb::RankingFunctionAny& self, py::function func, bool deduplicate) {
                auto func_handle = make_py_object_ptr(func);
                if (!func_handle) {
                    throw py::value_error("merge_apply expects a callable");
                }
                auto native = self.to_any_ranking(deduplicate);
                auto merged = rb::merge_apply<std::any>(
                    native,
                    [func_handle](const std::any& value) {
                        if (!func_handle) {
                            throw py::value_error("merge_apply callable is no longer available");
                        }
                        py::gil_scoped_acquire gil;
                        py::function callable = py::reinterpret_borrow<py::function>(func_handle.get());
                        py::object returned = callable(any_to_py(value));
                        try {
                            auto rf_any = returned.cast<rb::RankingFunctionAny>();
                            return rf_any.to_any_ranking(false);
                        } catch (const py::cast_error&) {
                            auto singleton = rb::singleton<std::any>(py_to_any(returned), rb::Rank::zero());
                            return singleton;
                        }
                    },
                    deduplicate);
                return rb::RankingFunctionAny{std::move(merged)};
            },
            py::arg("func"),
            py::arg("deduplicate") = true,
            R"pbdoc(Apply a ranking-valued function to each element lazily and merge the results.)pbdoc")
        .def("observe",
            [](const rb::RankingFunctionAny& self, py::function predicate, bool deduplicate) {
                auto predicate_handle = make_py_object_ptr(predicate);
                if (!predicate_handle) {
                    throw py::value_error("observe expects a callable predicate");
                }
                auto pred = [predicate_handle](const std::any& value) {
                    if (!predicate_handle) {
                        throw py::value_error("observe predicate is no longer available");
                    }
                    py::gil_scoped_acquire gil;
                    py::function callable = py::reinterpret_borrow<py::function>(predicate_handle.get());
                    return callable(any_to_py(value)).template cast<bool>();
                };
                try {
                    return self.observe(pred, deduplicate);
                } catch (const std::logic_error& error) {
                    throw py::value_error(error.what());
                }
            },
            py::arg("predicate"),
            py::arg("deduplicate") = true,
            R"pbdoc(Condition the ranking on a Python predicate and renormalise ranks.)pbdoc")
        .def("observe_value",
            [](const rb::RankingFunctionAny& self, py::object value, bool deduplicate) {
                try {
                    return self.observe_value(py_to_any(value), deduplicate);
                } catch (const std::logic_error& error) {
                    throw py::value_error(error.what());
                }
            },
            py::arg("value"),
            py::arg("deduplicate") = true,
            R"pbdoc(Condition the ranking on equality with ``value``.)pbdoc")
        .def("take_n",
            [](const rb::RankingFunctionAny& self, std::size_t count) {
                return pairs_to_python_list(self.take_n(count));
            },
            py::arg("count"),
            R"pbdoc(Materialise at most ``count`` elements as ``(value, Rank)`` tuples.)pbdoc")
        .def("materialize",
            [](const rb::RankingFunctionAny& self, std::size_t count) {
                return pairs_to_python_list(self.take_n(count));
            },
            py::arg("count"),
            R"pbdoc(Alias for :py:meth:`take_n`.)pbdoc")
        .def("shift_ranks",
            [](const rb::RankingFunctionAny& self, const rb::Rank& offset, bool deduplicate) {
                return self.shift_ranks(offset, deduplicate);
            },
            py::arg("offset"),
            py::arg("deduplicate") = true,
            R"pbdoc(Return a ranking with every element's rank increased by ``offset``.)pbdoc")
        .def_static("normal_exceptional",
            [](const rb::RankingFunctionAny& normal,
               py::function exceptional,
               const rb::Rank& offset,
               bool deduplicate) {
                auto exceptional_handle = make_py_object_ptr(exceptional);
                if (!exceptional_handle) {
                    throw py::value_error("normal_exceptional expects a callable");
                }

                auto thunk = [exceptional_handle]() -> rb::RankingFunctionAny {
                    if (!exceptional_handle) {
                        throw py::value_error("normal_exceptional callable is no longer available");
                    }
                    py::gil_scoped_acquire gil;
                    py::function callable =
                        py::reinterpret_borrow<py::function>(exceptional_handle.get());
                    py::object result = callable();
                    return result.cast<rb::RankingFunctionAny>();
                };

                return rb::normal_exceptional_any(normal, std::move(thunk), offset, deduplicate);
            },
            py::arg("normal"),
            py::arg("exceptional"),
            py::arg("offset") = rb::Rank::from_value(1),
            py::arg("deduplicate") = true,
            R"pbdoc(Lazily combine a normal ranking with an exceptional fallback.)pbdoc")
        .def_static("defer",
            [](py::function thunk) {
                auto thunk_handle = make_py_object_ptr(thunk);
                if (!thunk_handle) {
                    throw py::value_error("defer expects a callable");
                }
                auto deferred = rb::RankingFunctionAny::defer([thunk_handle]() {
                    py::gil_scoped_acquire gil;
                    py::function callable = py::reinterpret_borrow<py::function>(thunk_handle.get());
                    py::object result = callable();
                    return result.cast<rb::RankingFunctionAny>();
                });
                return deferred;
            },
            py::arg("thunk"),
            R"pbdoc(Lazily evaluate a callable that returns a ranking when forced.)pbdoc")
        .def("__bool__", [](const rb::RankingFunctionAny& self) { return !self.is_empty(); });

    bind_ranking_function<int>(m, "RankingFunctionInt", "int");
    bind_ranking_function<double>(m, "RankingFunctionFloat", "float");
    bind_ranking_function<std::string>(m, "RankingFunctionString", "str");
}
