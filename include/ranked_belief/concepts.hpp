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
template<typename T>
concept RankingIterator = requires(T it) {
    { *it } -> std::convertible_to<std::pair<typename T::value_type::first_type, ::ranked_belief::Rank>>;
    { ++it } -> std::same_as<T&>;
    { it == it } -> std::convertible_to<bool>;
};

#endif // RANKED_BELIEF_CONCEPTS_HPP