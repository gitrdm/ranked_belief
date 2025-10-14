#ifndef RANKED_BELIEF_CONCEPTS_HPP
#define RANKED_BELIEF_CONCEPTS_HPP

#include "rank.hpp"
#include <concepts>
#include <type_traits>

// Core value type requirements
template<typename T>
concept ValueType = std::movable<T> && std::destructible<T>;

template<typename T>
concept EqualityComparableValue = ValueType<T> && std::equality_comparable<T>;

template<typename T>
concept OrderedValue = EqualityComparableValue<T> && std::totally_ordered<T>;

// Function type requirements
template<typename F, typename... Args>
concept Invocable = std::invocable<F, Args...>;

template<typename F, typename T>
concept ValueFactory = Invocable<F> && std::same_as<std::invoke_result_t<F>, T>;

template<typename F, typename T>
concept LazyComputation = ValueFactory<F, T>;

// Iterator and range concepts
// Note: We intentionally do not define a RankingIterator concept to avoid
// name collision with the RankingIterator class template.
// If needed in the future, use a different name like RankingIteratorType.

#endif // RANKED_BELIEF_CONCEPTS_HPP