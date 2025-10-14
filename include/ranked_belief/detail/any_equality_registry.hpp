#ifndef RANKED_BELIEF_DETAIL_ANY_EQUALITY_REGISTRY_HPP
#define RANKED_BELIEF_DETAIL_ANY_EQUALITY_REGISTRY_HPP

#include <any>
#include <cstddef>
#include <mutex>
#include <string>
#include <typeindex>
#include <unordered_map>

namespace ranked_belief::detail {

using AnyEqualityFn = bool (*)(const std::any&, const std::any&);

inline std::unordered_map<std::type_index, AnyEqualityFn>& any_equality_registry()
{
    static auto& registry = *new std::unordered_map<std::type_index, AnyEqualityFn>();
    return registry;
}

inline std::mutex& any_equality_registry_mutex()
{
    static auto& mutex = *new std::mutex();
    return mutex;
}

inline void register_any_equality(std::type_index type, AnyEqualityFn fn)
{
    std::scoped_lock lock(any_equality_registry_mutex());
    any_equality_registry()[type] = fn;
}

template<typename T>
inline bool any_equals_cast(const std::any& lhs, const std::any& rhs)
{
    return std::any_cast<const T&>(lhs) == std::any_cast<const T&>(rhs);
}

template<typename T>
inline void register_any_equality()
{
    register_any_equality(std::type_index(typeid(T)), &any_equals_cast<T>);
}

inline void register_builtin_any_equalities()
{
    static const bool registered = [] {
        register_any_equality<std::nullptr_t>();
        register_any_equality<bool>();
        register_any_equality<int>();
        register_any_equality<long>();
        register_any_equality<long long>();
        register_any_equality<unsigned long>();
        register_any_equality<unsigned long long>();
        register_any_equality<float>();
        register_any_equality<double>();
        register_any_equality<std::string>();
        return true;
    }();
    (void)registered;
}

inline bool any_values_equal(const std::any& lhs, const std::any& rhs)
{
    register_builtin_any_equalities();

    if (!lhs.has_value() && !rhs.has_value()) {
        return true;
    }
    if (lhs.has_value() != rhs.has_value()) {
        return false;
    }
    if (lhs.type() != rhs.type()) {
        return false;
    }

    const auto type = std::type_index(lhs.type());

    AnyEqualityFn fn = nullptr;
    {
        std::scoped_lock lock(any_equality_registry_mutex());
        auto it = any_equality_registry().find(type);
        if (it != any_equality_registry().end()) {
            fn = it->second;
        }
    }

    if (!fn) {
        return false;
    }

    return fn(lhs, rhs);
}

} // namespace ranked_belief::detail

#endif // RANKED_BELIEF_DETAIL_ANY_EQUALITY_REGISTRY_HPP
