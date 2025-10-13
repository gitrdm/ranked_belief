#include <pybind11/functional.h>
#include <pybind11/operators.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <sstream>
#include <string>
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

namespace py = pybind11;
namespace rb = ranked_belief;

namespace {

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
                auto transformer = [func = std::move(func)](const T& value) -> T {
                    py::gil_scoped_acquire gil;
                    return func(value).template cast<T>();
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
                auto pred = [predicate = std::move(predicate)](const T& value) {
                    py::gil_scoped_acquire gil;
                    return predicate(value).template cast<bool>();
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
                auto binder = [func = std::move(func)](const T& value) -> RF {
                    py::gil_scoped_acquire gil;
                    return func(value).template cast<RF>();
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
                auto pred = [predicate = std::move(predicate)](const T& value) {
                    py::gil_scoped_acquire gil;
                    return predicate(value).template cast<bool>();
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
                auto gen = [generator = std::move(generator)](std::size_t index) {
                    py::gil_scoped_acquire gil;
                    return generator(index).template cast<std::pair<T, rb::Rank>>();
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

    bind_ranking_function<int>(m, "RankingFunctionInt", "int");
    bind_ranking_function<double>(m, "RankingFunctionFloat", "float");
    bind_ranking_function<std::string>(m, "RankingFunctionString", "str");
}
