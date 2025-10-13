# C++ Port Design for Ranked Belief Library

## Executive Summary

This document outlines the high-level design for porting the Racket-based ranked programming library to C++ as **ranked_belief**. The library implements a ranked programming paradigm based on ranking theory, where uncertainty is measured using degrees of surprise on an integer scale from 0 to ∞ (similar to probabilistic programming but using ranks instead of probabilities).

## Core Concepts from Racket Implementation

### 1. Ranking Functions
A **ranking function** represents a (potentially infinite) ordered sequence of values, each associated with a rank (integer 0 to ∞). Lower ranks represent "more normal" values, higher ranks represent "more exceptional" values.

### 2. Lazy Evaluation Architecture
The Racket implementation heavily relies on **lazy evaluation** using Scheme's `delay` and `force` primitives. This is **critical** for:
- Handling infinite ranking sequences efficiently
- Avoiding unnecessary computation
- Supporting recursive ranking definitions
- Enabling compositional operations without materializing entire sequences

### 3. Data Structure: Ranking Chain
The core data structure is a **lazy linked list** where each element contains:
- **value-promise**: A delayed computation that produces the value
- **rank**: An integer (0 to ∞) representing the surprise level
- **successor-promise**: A delayed computation that produces the next element

The chain terminates with a special element having rank = +∞.

## C++ Design Strategy

### Architecture Overview

```
┌─────────────────────────────────────────────────────────┐
│                    Public API Layer                      │
│  (User-facing functions: nrm_exc, observe, rlet, etc.)  │
└──────────────────────┬──────────────────────────────────┘
                       │
┌──────────────────────▼──────────────────────────────────┐
│              Type System & Utilities                     │
│    (RankingFunction, Rank types, type traits)           │
└──────────────────────┬──────────────────────────────────┘
                       │
┌──────────────────────▼──────────────────────────────────┐
│                Core Engine Layer                         │
│  (RankingElement, lazy evaluation, merge, filter, etc.) │
└─────────────────────────────────────────────────────────┘
```

## Key Design Components

### 1. Lazy Evaluation Implementation

**Challenge**: C++ does not have built-in lazy evaluation like Racket's `delay`/`force`.

**Solution**: Implement a custom `Promise<T>` class with lazy semantics.

This approach provides:
- Template class `Promise<T>` with lazy evaluation semantics
- `force()` method to trigger evaluation on-demand
- Automatic memoization to avoid re-computation
- Move semantics for efficiency
- Thread-safe evaluation using `std::call_once`

**Implementation interface**:
```
template<typename T>
class Promise {
    - Constructor accepts std::function<T()> or lambda
    - force() -> const T& (evaluates once, caches result)
    - Wraps a computation (lambda/function)
    - Memoizes result on first force()
    - Thread-safe evaluation via std::call_once
    - Move-only semantics (non-copyable)
    - RAII management of captured state
};
```

**Why this approach**:
- Direct control over evaluation semantics
- Optimal performance through inlining
- Type-safe compile-time checking
- Better error messages than std::function alternatives
- Explicit ownership semantics

### 2. Core Ranking Element Structure

```
template<typename T>
class RankingElement {
    Promise<T> value_promise;
    Rank rank;
    Promise<RankingElement<T>> successor_promise;
    
    - is_terminal() -> bool
    - value() -> T  (forces value_promise)
    - rank() -> Rank
    - successor() -> RankingElement<T> (forces successor_promise)
};
```

**Design considerations**:
- Use `std::shared_ptr` for managing element lifetime
- Rank type: `uint64_t` with special value for infinity (e.g., UINT64_MAX)
- Template parameter `T` for value type (supports any type)

### 3. Ranking Function Wrapper

```
template<typename T>
class RankingFunction {
    Promise<RankingElement<T>> root_element_promise;
    
    Public Interface:
    - begin() / end() -> Iterator support
    - filter(predicate) -> RankingFunction<T>
    - map(transform) -> RankingFunction<U>
    - merge(other) -> RankingFunction<T>
    - observe(predicate) -> RankingFunction<T>
    - normalize() -> RankingFunction<T>
    - to_vector(limit) -> std::vector<std::pair<T, Rank>>
};
```

### 4. Lazy Operations Implementation

All operations must preserve laziness:

#### **map-value** (Transform values)
- Returns new `Promise<RankingElement<U>>` 
- Transformation wrapped in lambda, not executed immediately
- Recursively wraps successor

#### **filter-ranking** (Conditional filtering)
- Returns new `Promise<RankingElement<T>>`
- Predicate evaluation delayed until force
- Skips non-matching elements lazily

#### **merge** (Combine rankings)
- Interleaves two ranking chains by rank order
- Uses lazy comparison - only forces needed elements
- Critical for `merge-apply` used in ranked function calls

#### **merge-apply** (Flat-map operation)
- Maps function returning ranking, then merges
- Essential for `$` (ranked application) operator
- Must preserve laziness through entire chain

### 4a. Interaction Between Ranked and Non-Ranked Values

A critical design feature is **automatic lifting** of non-ranked values to ranked functions. This is handled by an **autocast** mechanism:

#### **Autocast Mechanism**

**Concept**: Any value can be used where a ranking function is expected.

**Behavior**:
```cpp
// In Racket:
// - If value is already a ranking function, use as-is
// - If value is a regular value, wrap as singleton ranking with rank 0

template<typename T>
RankingFunction<T> autocast(const T& value) {
    if constexpr (is_ranking_function<T>::value) {
        return value;  // Already a ranking function
    } else {
        // Wrap as singleton: single value with rank 0
        return truth(value);
    }
}
```

**Key Interaction Points**:

1. **In Operations**:
   ```cpp
   // User can write:
   auto rf1 = nrm_exc(42, 100);        // Ranking function
   auto rf2 = observe([](int x) { return x > 50; }, rf1);
   
   // OR mix with regular values:
   auto rf3 = nrm_exc(rf1, 999);       // rf1 autocast to ranking
   auto rf4 = merge(rf1, 42);           // 42 autocast to singleton ranking
   ```

2. **In Ranked Application (`$` operator)**:
   
   **Three cases**:
   
   a. **Ranking over functions**: Function itself is uncertain
   ```cpp
   auto funcs = either_of({std::plus<int>(), std::minus<int>()});
   auto args = nrm_exc(1, 2);
   auto result = ranked_apply(funcs, args, args);
   // Result: ranking over results of different function choices
   ```
   
   b. **Primitive/pure function**: Known function, uncertain arguments
   ```cpp
   auto add = [](int a, int b) { return a + b; };  // Regular function
   auto x = nrm_exc(1, 2);
   auto y = nrm_exc(3, 4);
   auto result = ranked_apply(add, x, y);
   // Cartesian product of arguments, function returns regular values
   ```
   
   c. **Ranking-returning function**: Function returns ranking
   ```cpp
   auto stochastic_add = [](int a, int b) {
       return nrm_exc(a + b, a + b + 1);  // Returns ranking!
   };
   auto x = nrm_exc(1, 2);
   auto y = nrm_exc(3, 4);
   auto result = ranked_apply(stochastic_add, x, y);
   // Results merged: combines argument uncertainty + result uncertainty
   ```

#### **Design Implications for C++**

**Type Detection**:
```cpp
// Use type traits to detect ranking functions
template<typename T>
struct is_ranking_function : std::false_type {};

template<typename T>
struct is_ranking_function<RankingFunction<T>> : std::true_type {};

// SFINAE for autocast
template<typename T>
auto autocast(T&& value) -> std::enable_if_t<
    is_ranking_function<std::decay_t<T>>::value,
    RankingFunction<typename std::decay_t<T>::value_type>
> {
    return std::forward<T>(value);
}

template<typename T>
auto autocast(T&& value) -> std::enable_if_t<
    !is_ranking_function<std::decay_t<T>>::value,
    RankingFunction<std::decay_t<T>>
> {
    return truth(std::forward<T>(value));
}
```

**Function Return Type Detection**:
```cpp
// Detect if function returns RankingFunction
template<typename Func, typename... Args>
struct returns_ranking {
    using return_type = std::invoke_result_t<Func, Args...>;
    static constexpr bool value = 
        is_ranking_function<return_type>::value;
};

// Ranked application uses this for optimization
template<typename Func, typename... Args>
auto ranked_apply(Func f, RankingFunction<Args>... args) {
    if constexpr (returns_ranking<Func, Args...>::value) {
        // Use merge-apply (flatten nested rankings)
        return merge_apply_impl(f, args...);
    } else {
        // Simple map (no flattening needed)
        return map_apply_impl(f, args...);
    }
}
```

**Primitive Function Detection**:
```cpp
// Optional optimization: detect pure functions
template<typename Func>
struct is_primitive_function {
    // Conservative: assume not primitive unless marked
    static constexpr bool value = false;
};

// User can mark functions as primitive
#define RANKED_PRIMITIVE_FUNCTION(func) \
    template<> struct is_primitive_function<decltype(func)> \
    { static constexpr bool value = true; };

// Example:
auto add = [](int a, int b) { return a + b; };
RANKED_PRIMITIVE_FUNCTION(add);
```

#### **API Convenience**

All public APIs accept `RankingFunction<T> OR T`:

```cpp
// Flexible API using autocast
template<typename T>
auto observe(Predicate pred, auto&& rf_or_value) {
    auto rf = autocast(std::forward<decltype(rf_or_value)>(rf_or_value));
    return observe_impl(pred, rf);
}

// User can write:
observe(pred, ranking_func);    // Direct
observe(pred, 42);              // Autocast to singleton
observe(pred, std::vector{1,2,3}); // Autocast complex value
```

#### **Lambda Support**

Support for lambda returning rankings:

```cpp
// rlet with lambda that may return ranking or value
template<typename T, typename Func>
auto rlet(RankingFunction<T> rf, Func continuation) {
    using ReturnType = std::invoke_result_t<Func, T>;
    
    if constexpr (is_ranking_function<ReturnType>::value) {
        // Lambda returns ranking - use merge-apply
        return merge_apply(rf, [&](T val) {
            return continuation(val);  // Returns ranking
        });
    } else {
        // Lambda returns value - use map
        return rf.map([&](T val) {
            return continuation(val);  // Returns value
        });
    }
}
```

**Usage**:
```cpp
// Example 1: Continuation returns ranking
auto result = rlet(nrm_exc(1, 2), [](int x) {
    return nrm_exc(x, x * 2);  // Returns ranking
});

// Example 2: Continuation returns value
auto result = rlet(nrm_exc(1, 2), [](int x) {
    return x * 2;  // Returns value, autocast to singleton
});
```

#### **Performance Considerations**

**Optimization strategy**:
- Compile-time detection (zero runtime cost)
- Template specialization for different cases
- Inline autocast when possible
- Avoid unnecessary wrapping/unwrapping

**Potential Issues**:
- Template bloat from multiple instantiations
- Complex error messages from SFINAE
- Ambiguity with implicit conversions

**Mitigation**:
- Use concepts (C++20) for cleaner constraints
- Explicit conversion operators where needed
- Clear documentation of autocast behavior

### 4b. Autocast and FFI/Extension Compatibility

The autocast mechanism must work across three critical boundaries:
1. **C API (FFI)** for Python/R bindings
2. **Extension modules** (causal, propagation, constraints, etc.)
3. **Cross-language operations** (Python/R → C++ → Python/R)

#### **Challenge: Type Erasure for FFI**

**Problem**: Templates don't cross language boundaries. FFI needs type-erased interface.

**Solution**: Multi-layer approach

```
┌─────────────────────────────────────────────────┐
│           Python/R (Dynamic Types)              │
└─────────────────┬───────────────────────────────┘
                  │
┌─────────────────▼───────────────────────────────┐
│      C API (Type-Erased, Opaque Handles)        │
│  - rp_ranking_t (void* to type-erased wrapper)  │
│  - Type tags for runtime dispatch               │
└─────────────────┬───────────────────────────────┘
                  │
┌─────────────────▼───────────────────────────────┐
│    Type Erasure Layer (std::any / variant)      │
│  - RankingFunctionAny wraps RankingFunction<T>  │
│  - Dynamic type checking and casting            │
└─────────────────┬───────────────────────────────┘
                  │
┌─────────────────▼───────────────────────────────┐
│     Template Layer (Type-Safe C++ Core)         │
│  - RankingFunction<T> with full type safety     │
│  - Compile-time autocast and optimizations      │
└─────────────────────────────────────────────────┘
```

#### **Type-Erased Wrapper Design**

```cpp
// Type-erased wrapper for FFI
class RankingFunctionAny {
private:
    std::any impl_;  // Holds RankingFunction<T>
    const std::type_info* type_info_;
    
    // Function pointers for type-erased operations
    struct VTable {
        void (*iterate)(const std::any&, 
                       void (*callback)(const void*, uint64_t));
        std::any (*map)(const std::any&, 
                       void* func_ptr, void* context);
        std::any (*filter)(const std::any&, 
                          bool (*pred)(const void*));
        std::any (*merge)(const std::any&, const std::any&);
        // ... other operations
    };
    const VTable* vtable_;

public:
    // Construction from typed RankingFunction
    template<typename T>
    RankingFunctionAny(RankingFunction<T> rf)
        : impl_(std::move(rf))
        , type_info_(&typeid(T))
        , vtable_(get_vtable<T>())
    {}
    
    // Autocast support for type-erased layer
    template<typename T>
    static RankingFunctionAny from_value(T value) {
        return RankingFunctionAny(truth(std::move(value)));
    }
    
    // Check if already a ranking or needs autocast
    template<typename T>
    static RankingFunctionAny autocast(T&& value) {
        if constexpr (std::is_same_v<std::decay_t<T>, 
                                     RankingFunctionAny>) {
            return std::forward<T>(value);
        } else if constexpr (is_ranking_function_v<std::decay_t<T>>) {
            return RankingFunctionAny(std::forward<T>(value));
        } else {
            return from_value(std::forward<T>(value));
        }
    }
    
    // Type checking
    template<typename T>
    bool holds_type() const {
        return *type_info_ == typeid(T);
    }
    
    // Safe casting
    template<typename T>
    std::optional<RankingFunction<T>> try_cast() const {
        if (holds_type<T>()) {
            return std::any_cast<RankingFunction<T>>(impl_);
        }
        return std::nullopt;
    }
    
    // Type-erased operations (delegate to vtable)
    RankingFunctionAny filter(bool (*predicate)(const void*)) const {
        return RankingFunctionAny(vtable_->filter(impl_, predicate));
    }
    
    // ... other operations
};
```

#### **C API with Autocast Support**

```c
// C API (c_api.h)
typedef struct rp_ranking* rp_ranking_t;

// Type tags for polymorphism
typedef enum {
    RP_TYPE_INT,
    RP_TYPE_DOUBLE,
    RP_TYPE_STRING,
    RP_TYPE_BOOL,
    RP_TYPE_OBJECT  // Opaque user object
} rp_type_tag_t;

// Construction with autocast
rp_ranking_t rp_from_int(int64_t value);
rp_ranking_t rp_from_double(double value);
rp_ranking_t rp_from_string(const char* value);

// Operations accept ranking OR value (autocast)
rp_ranking_t rp_nrm_exc_int(int64_t normal, int64_t exceptional, 
                             uint64_t rank);
rp_ranking_t rp_nrm_exc_ranking(rp_ranking_t normal, 
                                rp_ranking_t exceptional, 
                                uint64_t rank);

// Polymorphic observe (works with any type)
rp_ranking_t rp_observe(rp_ranking_t rf, 
                        bool (*predicate)(const void*, rp_type_tag_t));

// Ranked application with autocast
rp_ranking_t rp_ranked_apply(void* func_ptr,
                             rp_ranking_t* args,
                             size_t num_args,
                             rp_type_tag_t return_type);
```

**C API Implementation**:
```cpp
// c_api.cpp
struct rp_ranking {
    RankingFunctionAny impl;
    rp_type_tag_t type_tag;
};

extern "C" {

rp_ranking_t rp_from_int(int64_t value) {
    auto* rf = new rp_ranking{
        RankingFunctionAny::from_value(value),
        RP_TYPE_INT
    };
    return rf;
}

rp_ranking_t rp_nrm_exc_ranking(rp_ranking_t normal, 
                                rp_ranking_t exceptional,
                                uint64_t rank) {
    // Both arguments already rankings (no autocast needed)
    auto result = nrm_exc_any(normal->impl, exceptional->impl, rank);
    return new rp_ranking{std::move(result), normal->type_tag};
}

rp_ranking_t rp_observe(rp_ranking_t rf,
                        bool (*predicate)(const void*, rp_type_tag_t)) {
    // Autocast already done (rf is ranking)
    auto result = rf->impl.filter([=](const void* val) {
        return predicate(val, rf->type_tag);
    });
    return new rp_ranking{std::move(result), rf->type_tag};
}

} // extern "C"
```

#### **Python Bindings with Autocast**

```python
# Python wrapper with automatic type handling
class RankingFunction:
    def __init__(self, _handle):
        self._handle = _handle  # C API handle
    
    @staticmethod
    def _autocast(value):
        """Convert Python value to ranking if needed."""
        if isinstance(value, RankingFunction):
            return value
        elif isinstance(value, int):
            return RankingFunction(_ranked_belief.from_int(value))
        elif isinstance(value, float):
            return RankingFunction(_ranked_belief.from_double(value))
        elif isinstance(value, str):
            return RankingFunction(_ranked_belief.from_string(value))
        else:
            # Pickle complex objects
            return RankingFunction(_ranked_belief.from_object(pickle.dumps(value)))
    
    def observe(self, predicate):
        """Observe with automatic autocast."""
        return RankingFunction(
            _ranked_belief.observe(self._handle, predicate)
        )
    
    def __or__(self, other):
        """Merge operator with autocast."""
        other_rf = RankingFunction._autocast(other)
        return RankingFunction(
            _ranked_belief.merge(self._handle, other_rf._handle)
        )

def nrm_exc(normal, exceptional, rank=1):
    """Create ranking with autocast support."""
    normal_rf = RankingFunction._autocast(normal)
    exceptional_rf = RankingFunction._autocast(exceptional)
    return RankingFunction(
        _ranked_belief.nrm_exc_ranking(
            normal_rf._handle, 
            exceptional_rf._handle, 
            rank
        )
    )

# Usage - seamless mixing
rf1 = nrm_exc(1, 2)           # Both ints, autocast
rf2 = nrm_exc(rf1, 100)       # Mix ranking and int
rf3 = rf1 | 50                # Merge with int via __or__
```

#### **pybind11 Alternative (More Pythonic)**

```cpp
// Direct Python binding with pybind11
PYBIND11_MODULE(_ranked_belief, m) {
    py::class_<RankingFunctionAny>(m, "RankingFunction")
        .def("observe", [](RankingFunctionAny& self, py::function pred) {
            return self.filter([pred](const void* val) {
                return pred(py::cast(val)).cast<bool>();
            });
        })
        .def("__or__", [](RankingFunctionAny& self, py::object other) {
            // Autocast in binding layer
            auto other_rf = py_autocast(other);
            return merge(self, other_rf);
        });
    
    // Autocast helper
    m.def("_autocast", [](py::object value) -> RankingFunctionAny {
        if (py::isinstance<RankingFunctionAny>(value)) {
            return value.cast<RankingFunctionAny>();
        } else if (py::isinstance<py::int_>(value)) {
            return RankingFunctionAny::from_value(value.cast<int64_t>());
        } else if (py::isinstance<py::float_>(value)) {
            return RankingFunctionAny::from_value(value.cast<double>());
        } else if (py::isinstance<py::str>(value)) {
            return RankingFunctionAny::from_value(value.cast<std::string>());
        } else {
            // Wrap as Python object
            return RankingFunctionAny::from_value(value);
        }
    });
    
    // Python-side autocast wrapper
    m.def("nrm_exc", [](py::object normal, py::object exceptional, 
                        uint64_t rank) {
        return nrm_exc(py_autocast(normal), py_autocast(exceptional), rank);
    });
}
```

#### **Extension Module Compatibility**

Extensions must work with both typed and type-erased interfaces:

```cpp
namespace ranked_belief::causal {

// Template version (for C++ users)
template<typename T>
class CausalModel {
    std::unordered_map<Variable, RankingFunction<T>> equations;
    
public:
    template<typename U>
    auto intervene(Variable v, U value) -> CausalModel<T> {
        // Autocast value to RankingFunction<T>
        auto rf = autocast(std::forward<U>(value));
        // ... intervention logic
    }
};

// Type-erased version (for FFI/extensions)
class CausalModelAny {
    std::unordered_map<Variable, RankingFunctionAny> equations;
    
public:
    auto intervene(Variable v, RankingFunctionAny value) -> CausalModelAny {
        // Value already autocast if needed
        // ... intervention logic
    }
    
    // Convenience: autocast from raw value
    template<typename T>
    auto intervene_value(Variable v, T value) -> CausalModelAny {
        return intervene(v, RankingFunctionAny::from_value(value));
    }
};

} // namespace causal
```

#### **Cross-Extension Autocast**

Extensions can pass rankings to each other:

```cpp
// Constraint module using causal model results
auto causal_result = causal_model.query(variables);  // Returns RankingFunction
auto constraint_system = constraints::ConstraintSystem();
constraint_system.add_soft(
    causal_result,  // Autocast to constraint preferences
    rank=2
);

// Belief propagation using constraint solutions
auto solutions = constraint_system.solve();  // Returns RankingFunction
auto network = propagation::RankingNetwork();
network.set_evidence(variable, solutions);  // Autocast as evidence
```

#### **Performance Considerations**

**FFI Overhead**:
- Type erasure has runtime cost
- Minimize boundary crossings
- Batch operations where possible

**Optimization Strategy**:
```cpp
// Fast path for common types (int, double, string)
template<>
struct FastPath<int64_t> {
    static constexpr bool value = true;
    // Specialized implementation avoids std::any
};

// Slow path for complex types
template<typename T>
struct FastPath {
    static constexpr bool value = false;
    // Uses std::any / type erasure
};
```

**Type Safety**:
- C++ core: Full compile-time safety
- FFI boundary: Runtime type checking
- Python/R: Duck typing with validation

#### **Design Summary**

**Autocast works at three levels**:

1. **C++ Template Level** (compile-time):
   - Full type safety
   - Zero overhead
   - SFINAE/concepts for dispatch

2. **Type-Erased Level** (C++ runtime):
   - RankingFunctionAny wrapper
   - Dynamic type checking
   - Minimal overhead for common types

3. **FFI Level** (cross-language):
   - C API with type tags
   - Explicit memory management
   - Language-specific wrappers

**All three levels support autocast**:
- Regular values → singleton rankings
- Rankings pass through unchanged
- Functions dispatch based on return type

This design ensures seamless interaction across:
- Pure C++ usage (template benefits)
- Python/R bindings (dynamic typing)
- Extension modules (type-erased interface)
- Cross-extension composition

### 5. Type System Considerations

**Generic Value Support**:
```
- Use templates for type-safe operations
- Value type T must satisfy:
  * CopyConstructible or MoveConstructible (required)
  * EqualityComparable (operator==) (required for deduplication)
  * std::hash specialization (required for deduplication)
  
- For deduplication support, T must provide:
  * operator==(const T&, const T&) for equality comparison
  * std::hash<T> specialization for hash-based tracking
  
- Types not supporting hashing can:
  * Disable deduplication
  * Provide custom hash function
  * Use linear search fallback (slower)
```

**Rank Type** (type-safe class):
```
class Rank {
private:
    uint64_t value;
    bool is_infinite;
    
public:
    // Constructors
    constexpr Rank(uint64_t v) : value(v), is_infinite(false) {}
    static constexpr Rank infinity() { return Rank(true); }
    
    // Queries
    constexpr bool is_inf() const { return is_infinite; }
    constexpr uint64_t get_value() const { return value; }
    
    // Comparison operators (with proper infinity semantics)
    constexpr bool operator==(const Rank& other) const;
    constexpr bool operator!=(const Rank& other) const;
    constexpr bool operator<(const Rank& other) const;
    constexpr bool operator<=(const Rank& other) const;
    constexpr bool operator>(const Rank& other) const;
    constexpr bool operator>=(const Rank& other) const;
    
    // Arithmetic with saturation
    constexpr Rank operator+(const Rank& other) const;
    constexpr Rank operator-(const Rank& other) const;
    
    // Stream output
    friend std::ostream& operator<<(std::ostream& os, const Rank& r);
    
private:
    constexpr Rank(bool inf) : value(0), is_infinite(inf) {}
};

// Constants
inline constexpr Rank RANK_ZERO{0};
inline constexpr Rank RANK_INFINITY = Rank::infinity();
```

**Rationale for type-safe Rank**:
- Prevents accidental mixing of ranks and regular integers
- Explicit infinity handling without magic values
- Saturating arithmetic prevents overflow bugs
- Clear semantics for rank operations
- Better compiler error messages

### 6. Memory Management Strategy

**Challenges**:
- Lazy chains can be long or infinite
- Shared structure between operations
- Avoid memory leaks in recursive definitions

**Solution**:
```
- std::shared_ptr for RankingElement nodes
- std::weak_ptr for circular references (if needed)
- Promise class manages captured lambda lifetime
- RAII principles throughout
- Optional: custom allocator for element pool
```

### 7. API Design

#### **Construction APIs**:
```
// Single value
auto truth(T value) -> RankingFunction<T>

// Normal/exceptional choice
auto nrm_exc(T normal, T exceptional, Rank exc_rank = 1) 
    -> RankingFunction<T>

// Multiple alternatives
auto either_of(std::vector<T> values) -> RankingFunction<T>

// From pairs
auto construct_ranking(std::vector<std::pair<T, Rank>> pairs)
    -> RankingFunction<T>

// Failure (empty ranking)
auto failure() -> RankingFunction<T>
```

#### **Conditioning APIs**:
```
// Standard conditioning
auto observe(Predicate pred, RankingFunction<T> rf)
    -> RankingFunction<T>

// Result-oriented conditioning
auto observe_r(Rank x, Predicate pred, RankingFunction<T> rf)
    -> RankingFunction<T>

// Evidence-oriented conditioning  
auto observe_e(Rank x, Predicate pred, RankingFunction<T> rf)
    -> RankingFunction<T>
```

#### **Manipulation APIs**:
```
// Ranked let binding (monadic bind)
template<typename T, typename U>
auto rlet(RankingFunction<T> rf, 
          std::function<RankingFunction<U>(T)> continuation)
    -> RankingFunction<U>

// Ranked application (applicative functor)
template<typename Func, typename... Args>
auto ranked_apply(Func f, RankingFunction<Args>... args)
    -> RankingFunction<result_type>

// Cut at rank
auto cut(Rank max_rank, RankingFunction<T> rf)
    -> RankingFunction<T>

// Limit element count
auto limit(size_t count, RankingFunction<T> rf)
    -> RankingFunction<T>
```

#### **Query APIs**:
```
// Get rank of first element matching predicate
auto rank_of(Predicate pred, RankingFunction<T> rf) -> Rank

// Convert to materialized structures
auto to_hash(RankingFunction<T> rf) 
    -> std::unordered_map<T, Rank>

auto to_assoc(RankingFunction<T> rf)
    -> std::vector<std::pair<T, Rank>>

auto to_stream(RankingFunction<T> rf)
    -> /* lazy iterator/range */
```

#### **Display APIs**:
```
// Print first N elements
void pr_first(size_t n, RankingFunction<T> rf, std::ostream& os)

// Print up to rank R
void pr_until(Rank r, RankingFunction<T> rf, std::ostream& os)

// Print all (careful with infinite rankings!)
void pr_all(RankingFunction<T> rf, std::ostream& os)
```

### 8. Deduplication Strategy

The Racket implementation supports deduplication (removing higher-ranked duplicates).

**C++ Design**:
```
- Enabled by default (matching Racket behavior)
- Controllable via dedup parameter in operations
- Use std::unordered_set<T> to track seen values during iteration
- Set propagates through lazy chain
- Trade-off: memory vs. duplicate removal

Deduplication Control:
1. Global setting: set_global_dedup(bool enabled)
2. Per-operation control via optional parameter:
   - observe(pred, rf, dedup = true)
   - filter(pred, rf, dedup = true)
   - map(func, rf, dedup = true)
3. RankingFunction method: rf.with_dedup(bool enabled)
```

**Implementation Details**:
```
template<typename T>
class RankingFunction {
    bool dedup_enabled = true;  // Default: enabled
    
public:
    // Control deduplication
    RankingFunction<T> with_dedup(bool enabled) const;
    RankingFunction<T> without_dedup() const;
    
    // Check current setting
    bool is_dedup_enabled() const;
};

// Global setting (affects new constructions)
void set_global_dedup(bool enabled);
bool get_global_dedup();

// All operations respect dedup flag:
template<typename T>
auto filter(Predicate pred, RankingFunction<T> rf, 
            std::optional<bool> dedup = std::nullopt)
    -> RankingFunction<T>;
// If dedup=nullopt, use rf's setting; otherwise override
```

**Requirements for Deduplication**:
- Type T must be hashable (std::hash<T> specialization)
- Type T must be equality comparable (operator==)
- For custom types, provide both or disable deduplication

**Memory Consideration**:
- Dedup set grows with unique values seen
- For infinite rankings with many unique values, consider disabling
- Set is shared across chain via std::shared_ptr for efficiency

### 9. Iterator Support

Enable range-based for loops and STL algorithm compatibility:

```
template<typename T>
class RankingIterator {
    - Forward iterator semantics
    - Dereferences to std::pair<T, Rank>
    - Lazy evaluation on increment
    - Terminates at infinite rank
};

Usage:
for (auto [value, rank] : ranking_function) {
    // Process elements
}
```

### 10. Error Handling

```
- Use exceptions for invalid operations
  * InvalidRankException
  * EmptyRankingException
  * TypeMismatchException (for dynamic scenarios)
  
- Assertions for debugging
  * Rank order verification
  * Promise state validation
  
- Optional: std::expected<T, Error> for error returns
```

## Advanced Features

### 1. Recursive Definitions

Support self-referential rankings like:
```cpp
RankingFunction<int> fun(int x) {
    return nrm_exc(x, [=]() { return fun(x * 2); });
}
```

**Implementation**:
- Lambda capture for recursion
- Lazy evaluation prevents infinite recursion
- Stack safety via trampolining if needed

### 2. Join Operations

Cartesian product of rankings:
```cpp
auto join(RankingFunction<T> a, RankingFunction<U> b)
    -> RankingFunction<std::tuple<T, U>>
```

**Implementation**:
- Nested lazy iteration
- Rank addition for combined surprise
- Used in `rlet` implementation

### 3. Correctness Checking

Optional validation that ranks are in ascending order:
```cpp
auto check_correctness(RankingFunction<T> rf) 
    -> RankingFunction<T>
```

## Extensibility & Future Features

### Design Principles for Extensibility

The core library design must be modular and extensible to support advanced features without major refactoring:

**1. Separation of Concerns**:
```
Core Layer (stable):
- Promise/lazy evaluation
- RankingElement/RankingFunction
- Basic operations (map, filter, merge)

Extension Layer (plugin architecture):
- Causal reasoning
- Belief propagation
- Constraint solving
- Advanced formalisms
```

**2. Plugin Architecture**:
```cpp
namespace ranked_belief {
    // Core library
    namespace core { /* stable API */ }
    
    // Extension modules (optional)
    namespace causal { /* causal inference */ }
    namespace propagation { /* belief propagation */ }
    namespace constraints { /* constraint solving */ }
    namespace c_representations { /* C-representations */ }
    namespace optimization { /* optimization & CP */ }
}
```

**3. Extension Points**:
```cpp
// Core must provide hooks for extensions
class RankingFunction<T> {
public:
    // Extension point: attach metadata
    template<typename MetaT>
    void attach_metadata(const std::string& key, MetaT data);
    
    // Extension point: custom operations
    template<typename OpT>
    auto apply_operation(OpT operation) -> RankingFunction<U>;
    
    // Extension point: observers/callbacks
    void add_observer(std::function<void(const RankingElement<T>&)>);
};
```

### Planned Extension Modules

#### 1. Causal Reasoning Module

**Purpose**: Complete causal inference toolkit using ranking-theoretic foundations

**Design Requirements**:
```
Core Dependencies:
- RankingFunction as base
- Conditional ranking operations
- Interventional semantics

New Components:
- CausalModel<T>: Structural causal model representation
- InterventionOperator: do-calculus implementation
- CounterfactualQuery: counterfactual reasoning
- CausalGraph: DAG structure with causal dependencies
```

**Interface Sketch**:
```cpp
namespace ranked_belief::causal {

class CausalModel {
    // Structural equations with ranking functions
    using StructuralEquation = 
        std::function<RankingFunction<Value>(Context)>;
    
    // DAG of causal relationships
    CausalGraph graph;
    std::unordered_map<Variable, StructuralEquation> equations;
    
public:
    // Observational queries
    auto observe(Variable v, Value val) -> CausalModel;
    
    // Interventional queries (do-calculus)
    auto intervene(Variable v, Value val) -> CausalModel;
    
    // Counterfactual queries
    auto counterfactual(
        std::vector<Intervention> interventions,
        std::vector<Observation> observations
    ) -> RankingFunction<Outcome>;
    
    // Identification
    bool is_identifiable(Query q) const;
    auto identify(Query q) -> std::optional<Expression>;
};

// Pearl's do-operator
template<typename T>
auto do_intervention(CausalModel& model, Variable v, T value)
    -> CausalModel;

// Counterfactual reasoning
template<typename T>
auto counterfactual_query(
    CausalModel& model,
    std::vector<Assignment> actual_world,
    std::vector<Assignment> hypothetical
) -> RankingFunction<T>;

} // namespace causal
```

**Integration Strategy**:
- Extend RankingFunction with causal metadata
- Separate library: `libranked_causal.so`
- Optional dependency in main library

#### 2. Belief Propagation Module

**Purpose**: Efficient inference in large ranking networks using Shenoy's valuation-based algorithm

**Design Requirements**:
```
Core Dependencies:
- RankingFunction for local potentials
- Efficient merge operations
- Graph algorithms

New Components:
- RankingNetwork: Network structure (Markov network, Bayesian network)
- ValuationAlgebra: Shenoy's abstract framework
- MessagePassing: Belief propagation algorithm
- JunctionTree: Tree decomposition for exact inference
```

**Interface Sketch**:
```cpp
namespace ranked_belief::propagation {

class RankingNetwork {
    struct Node {
        std::string id;
        std::vector<std::string> domain;
        RankingFunction<std::vector<Value>> potential;
    };
    
    struct Edge {
        std::string from, to;
        // Optional: edge potentials
    };
    
    std::vector<Node> nodes;
    std::vector<Edge> edges;
    
public:
    // Build network
    void add_node(std::string id, RankingFunction<Value> potential);
    void add_edge(std::string from, std::string to);
    
    // Query interface
    auto query(std::vector<Variable> query_vars,
               std::vector<Evidence> evidence = {})
        -> RankingFunction<std::vector<Value>>;
    
    // MAP inference
    auto most_plausible_explanation(std::vector<Evidence> evidence)
        -> std::unordered_map<Variable, Value>;
};

// Belief propagation algorithms
class BeliefPropagation {
public:
    // Shenoy's valuation-based algorithm
    static auto shenoy_algorithm(
        const RankingNetwork& network,
        std::vector<Variable> query_vars
    ) -> RankingFunction<std::vector<Value>>;
    
    // Junction tree algorithm
    static auto junction_tree_algorithm(
        const RankingNetwork& network,
        std::vector<Variable> query_vars
    ) -> RankingFunction<std::vector<Value>>;
    
    // Loopy belief propagation (approximate)
    static auto loopy_bp(
        const RankingNetwork& network,
        size_t max_iterations = 100
    ) -> RankingNetwork;
};

// Valuation algebra framework
template<typename T>
concept ValuationAlgebra requires {
    // Combination (join)
    T combine(const T& v1, const T& v2);
    
    // Marginalization (project)
    T marginalize(const T& v, std::vector<Variable> vars);
    
    // Neutral element
    T neutral();
};

} // namespace propagation
```

**Integration Strategy**:
- Graph library integration (Boost.Graph or custom)
- Separate library: `libranked_propagation.so`
- Optimized for large-scale networks

#### 3. Constraint Solving Module

**Purpose**: SMT-powered constraint reasoning with Z3 integration

**Design Requirements**:
```
Core Dependencies:
- RankingFunction for solution preferences
- Observe/filter for constraints

External Dependencies:
- Z3 SMT solver
- Optional: other solvers (CVC5, Yices)

New Components:
- ConstraintSystem: Logical constraints + ranking preferences
- SMTSolver: Interface to Z3
- RankedSolutions: Solutions ordered by rank
```

**Interface Sketch**:
```cpp
namespace ranked_belief::constraints {

// Constraint types
using Constraint = std::variant<
    LinearConstraint,
    NonlinearConstraint,
    LogicalConstraint,
    CustomConstraint
>;

class ConstraintSystem {
    std::vector<Constraint> hard_constraints;  // Must satisfy
    RankingFunction<Constraint> soft_constraints;  // Preferences
    
public:
    // Add constraints
    void add_hard(Constraint c);
    void add_soft(Constraint c, Rank preference_rank);
    
    // Solve
    auto solve() -> RankingFunction<Solution>;
    
    // Optimization
    auto optimize(ObjectiveFunction obj) 
        -> RankingFunction<Solution>;
    
    // Minimal unsatisfiable core
    auto unsat_core() -> std::vector<Constraint>;
};

// Z3 integration
class Z3Solver {
    z3::context ctx;
    z3::solver solver;
    
public:
    // Encode ranking function as SMT
    void encode_ranking(const RankingFunction<Value>& rf);
    
    // Solve with ranked preferences
    auto solve_ranked() -> RankingFunction<z3::model>;
    
    // MaxSAT/MaxSMT for soft constraints
    auto max_sat(std::vector<Constraint> soft,
                 std::vector<Constraint> hard)
        -> RankingFunction<Solution>;
};

// Constraint programming
template<typename Domain>
class CSPSolver {
public:
    // Variable declaration
    Variable<Domain> add_variable(std::string name, Domain domain);
    
    // Constraint propagation
    void propagate();
    
    // Backtracking search with ranking
    auto search() -> RankingFunction<Assignment>;
};

} // namespace constraints
```

**Integration Strategy**:
- Z3 as optional dependency
- CMake option: `RANKED_WITH_Z3=ON`
- Separate library: `libranked_constraints.so`

#### 4. C-Representations Module

**Purpose**: Kern-Isberner's formalism for structured knowledge bases

**Design Requirements**:
```
Core Dependencies:
- RankingFunction as semantic interpretation
- Conditional operations

New Components:
- ConditionalKnowledgeBase: (If A then normally B) structure
- CRepresentation: Minimum information principle
- InferenceEngine: Entailment checking
```

**Interface Sketch**:
```cpp
namespace ranked_belief::c_representations {

// Conditional belief: (B|A)[r]
template<typename T>
struct Conditional {
    std::function<bool(T)> antecedent;   // A
    std::function<bool(T)> consequent;   // B
    Rank rank;                            // r (default = 0 for normality)
};

class KnowledgeBase {
    std::vector<Conditional<Value>> conditionals;
    
public:
    // Add conditional
    void add(Conditional<Value> cond);
    
    // Compute C-representation (minimum information principle)
    auto c_representation() -> RankingFunction<Value>;
    
    // Inference
    bool entails(Conditional<Value> query) const;
    auto query(std::function<bool(Value)> formula)
        -> RankingFunction<Value>;
    
    // Consistency checking
    bool is_consistent() const;
    
    // Revision operators
    auto revise(Conditional<Value> new_belief) -> KnowledgeBase;
};

// System Z implementation
class SystemZ {
public:
    static auto compute_ranking(const KnowledgeBase& kb)
        -> RankingFunction<Value>;
    
    // Z-ordering of conditionals
    static auto z_ordering(const KnowledgeBase& kb)
        -> std::vector<std::vector<Conditional<Value>>>;
};

// Principle of minimum information
template<typename T>
auto minimum_cross_entropy(
    const KnowledgeBase& kb,
    const RankingFunction<T>& prior
) -> RankingFunction<T>;

} // namespace c_representations
```

**Integration Strategy**:
- Pure extension of core library
- Separate header: `ranked_programming/c_representations.hpp`
- May share library with core

#### 5. Optimization & Constraint Programming Module

**Purpose**: Optimization and constraint programming with ranked preferences

**Design Requirements**:
```
Core Dependencies:
- RankingFunction for objective preferences
- Constraint system from constraints module

New Components:
- RankedOptimizer: Multi-objective optimization
- ConstraintProgram: CP with ranking
- ParetianOptimality: Trade-offs between objectives
```

**Interface Sketch**:
```cpp
namespace ranked_belief::optimization {

// Multi-objective with rankings
class RankedOptimizer {
public:
    // Add objective with importance rank
    void add_objective(ObjectiveFunction f, Rank importance);
    
    // Lexicographic optimization (by rank order)
    auto optimize_lexicographic() -> RankingFunction<Solution>;
    
    // Pareto frontier with ranking
    auto pareto_frontier() -> RankingFunction<Solution>;
    
    // Satisficing: solutions above threshold
    auto satisficing(std::unordered_map<ObjectiveFunction, Threshold> thresholds)
        -> RankingFunction<Solution>;
};

// Constraint programming
class RankedCP {
    ConstraintSystem constraints;
    std::vector<std::pair<ObjectiveFunction, Rank>> objectives;
    
public:
    // Search strategies
    enum class Strategy {
        DepthFirst,
        BestFirst,
        LimitedDiscrepancy
    };
    
    // Branch and bound with ranking
    auto branch_and_bound(Strategy strategy = Strategy::BestFirst)
        -> RankingFunction<Solution>;
    
    // Large neighborhood search
    auto lns(size_t neighborhood_size, size_t iterations)
        -> RankingFunction<Solution>;
};

// Integration with constraint module
template<typename T>
auto optimize_under_constraints(
    const ConstraintSystem& constraints,
    ObjectiveFunction objective
) -> RankingFunction<T>;

} // namespace optimization
```

**Integration Strategy**:
- Builds on constraints module
- Optional external solvers (CPLEX, Gurobi)
- Separate library: `libranked_optimization.so`

### Module Organization for Extensions

```
ranked_belief/
├── include/
│   ├── ranked_belief/
│   │   ├── core.hpp                    // Core (stable)
│   │   ├── ranking_function.hpp        // Core (stable)
│   │   ├── operations.hpp              // Core (stable)
│   │   ├── api.hpp                     // Core (stable)
│   │   ├── extensions/                 // Extension headers
│   │   │   ├── causal.hpp              // Causal reasoning
│   │   │   ├── propagation.hpp         // Belief propagation
│   │   │   ├── constraints.hpp         // Constraint solving
│   │   │   ├── c_representations.hpp   // C-representations
│   │   │   └── optimization.hpp        // Optimization
│   │   └── all_extensions.hpp          // Convenience include
├── src/
│   ├── core/                           // Core implementation
│   └── extensions/                     // Extension implementations
│       ├── causal/
│       ├── propagation/
│       ├── constraints/
│       ├── c_representations/
│       └── optimization/
├── bindings/
│   ├── python/
│   │   ├── core/                       // Core Python bindings
│   │   └── extensions/                 // Extension bindings
│   └── r/
│       ├── core/                       // Core R bindings
│       └── extensions/                 // Extension bindings
└── CMakeLists.txt
```

### CMake Configuration for Modularity

```cmake
# Core library (always built)
add_library(ranked_belief_core ...)

# Extension options (optional)
option(RANKED_BUILD_CAUSAL "Build causal reasoning module" OFF)
option(RANKED_BUILD_PROPAGATION "Build belief propagation module" OFF)
option(RANKED_BUILD_CONSTRAINTS "Build constraint solving module" OFF)
option(RANKED_BUILD_CREP "Build C-representations module" OFF)
option(RANKED_BUILD_OPTIMIZATION "Build optimization module" OFF)

# External dependencies (optional)
option(RANKED_WITH_Z3 "Enable Z3 SMT solver" OFF)
option(RANKED_WITH_BOOST "Enable Boost.Graph" OFF)

# Conditional compilation
if(RANKED_BUILD_CAUSAL)
    add_library(ranked_belief_causal ...)
    target_link_libraries(ranked_belief_causal 
        ranked_belief_core)
endif()

# Extension packages (Python/R)
if(RANKED_BUILD_PYTHON_BINDINGS)
    # Core bindings always built
    pybind11_add_module(ranked_belief_core_py ...)
    
    # Extension bindings (optional)
    if(RANKED_BUILD_CAUSAL)
        pybind11_add_module(ranked_belief_causal_py ...)
    endif()
endif()
```

### API Stability Guarantee

```
Core API (ranked_belief/core.hpp):
- Semantic versioning
- Stability guarantee: No breaking changes in minor versions
- ABI stability for C API

Extension APIs:
- Experimental in v1.x (subject to change)
- Stabilize in v2.0
- Clear deprecation policy
```

### Documentation Structure for Extensions

```
docs/
├── core/
│   ├── getting_started.md
│   ├── api_reference.md
│   └── tutorials/
├── extensions/
│   ├── overview.md                    // When to use each extension
│   ├── causal/
│   │   ├── guide.md
│   │   ├── examples/
│   │   └── api.md
│   ├── propagation/
│   ├── constraints/
│   ├── c_representations/
│   └── optimization/
└── integration/
    └── combining_extensions.md        // Using multiple extensions together
```

### Testing Strategy for Extensions

```
tests/
├── core/                              // Core tests (always run)
├── extensions/
│   ├── causal/
│   │   ├── unit/
│   │   ├── integration/
│   │   └── benchmarks/
│   └── ...
└── integration/                       // Cross-extension tests
    └── causal_with_constraints_test.cpp
```

### Performance Considerations for Extensions

- **Lazy evaluation preserved**: Extensions should not force evaluation unnecessarily
- **Zero-cost abstractions**: Use templates where appropriate
- **Optional optimizations**: SIMD, parallelization via compile flags
- **Profiling hooks**: Extensions can add performance monitoring
- **Incremental computation**: Cache intermediate results where possible

## Module Organization

```
ranked_belief/
├── include/
│   ├── ranked_belief/
│   │   ├── core.hpp              // Core types, Promise, RankingElement
│   │   ├── ranking_function.hpp  // RankingFunction wrapper
│   │   ├── operations.hpp        // map, filter, merge, etc.
│   │   ├── api.hpp               // High-level user API
│   │   ├── iterators.hpp         // Iterator support
│   │   ├── utilities.hpp         // Display, conversion, etc.
│   │   └── c_api.h               // C API for language bindings
│   └── ranked_belief.hpp         // Single include header
├── src/
│   ├── core.cpp                  // Promise implementation
│   ├── ranking_function.cpp      // RankingFunction methods
│   ├── operations.cpp            // Operation implementations
│   ├── utilities.cpp             // Display functions
│   └── c_api.cpp                 // C API implementation
├── bindings/
│   ├── python/
│   │   ├── setup.py              // Python package setup
│   │   ├── ranked_belief/
│   │   │   ├── __init__.py       // Python module
│   │   │   ├── _core.pyx         // Cython bindings
│   │   │   └── ranking.py        // Python wrapper classes
│   │   └── tests/
│   │       └── test_*.py         // Python tests
│   └── r/
│       ├── DESCRIPTION           // R package metadata
│       ├── NAMESPACE             // R exports
│       ├── R/
│       │   └── ranking.R         // R wrapper functions
│       ├── src/
│       │   └── r_bindings.cpp    // Rcpp bindings
│       └── tests/
│           └── testthat/         // R tests
├── tests/
│   ├── test_core.cpp
│   ├── test_operations.cpp
│   ├── test_api.cpp
│   └── examples/                 // Port of Racket examples
│       ├── boolean_circuit.cpp
│       ├── hidden_markov.cpp
│       ├── recursion.cpp
│       └── ...
├── benchmarks/
│   └── performance_tests.cpp
├── docs/
│   ├── API.md
│   ├── python_guide.md           // Python binding documentation
│   └── r_guide.md                // R binding documentation
└── CMakeLists.txt
```

## C++ Version Selection

### Requirement: **C++20**

**Rationale**: Modern features essential for clean template library design

### Detailed Analysis by Version

#### **C++14** (Not Recommended)

**Pros**:
- Widest compiler support (ancient systems)
- Minimal learning curve for legacy codebases

**Cons**:
- ❌ No `std::optional` (critical for error handling)
- ❌ No structured bindings (API ergonomics suffer)
- ❌ No `if constexpr` (template code becomes complex)
- ❌ No fold expressions (variadic templates harder)
- ❌ Generic lambdas require workarounds
- ❌ `auto` return type deduction limited

**Verdict**: Too restrictive for modern library design. Would require significant boilerplate.

---

#### **C++17** ✅ (Minimum Supported)

**Pros**:
- ✅ `std::optional` - Clean error handling without exceptions
- ✅ Structured bindings - `auto [value, rank] = element;`
- ✅ `if constexpr` - Critical for autocast dispatch
- ✅ Fold expressions - Simplify variadic templates
- ✅ `std::any` - Type erasure for FFI layer
- ✅ `std::variant` - Sum types for error handling
- ✅ Inline variables - Header-only constants
- ✅ `std::invoke` - Generic callable invocation
- ✅ Strong compiler support (GCC 7+, Clang 5+, MSVC 2017+)

**Cons**:
- ❌ No concepts - SFINAE still needed (complex errors)
- ❌ No ranges - Iterator code more verbose
- ❌ No coroutines - Can't use for lazy evaluation
- ❌ No `std::format` - Must use older I/O
- ❌ No designated initializers - Struct init less clear

**Use Cases**:
```cpp
// std::optional for safe operations
std::optional<RankingElement<T>> try_get_next() const;

// Structured bindings for ergonomics
for (auto [value, rank] : ranking_function) { }

// if constexpr for autocast
if constexpr (is_ranking_function_v<T>) { }

// std::any for type erasure (FFI)
class RankingFunctionAny {
    std::any impl_;
};
```

**Verdict**: ⚠️ **Not sufficient**. Missing concepts and ranges which are critical for library usability.

---

#### **C++20** ✅✅ (Required)

**Pros**:
- ✅✅ **Concepts** - Clean constraints, readable errors
  ```cpp
  template<Rankable T>
  auto observe(Predicate<T> pred, RankingFunction<T> rf);
  ```
- ✅✅ **Ranges** - Lazy evaluation, composition
  ```cpp
  rf | std::views::filter(pred) | std::views::take(10)
  ```
- ✅ **Three-way comparison** - `operator<=>` for Rank
- ✅ **Designated initializers** - Clear struct construction
- ✅ **`consteval`** - Compile-time computation guarantees
- ✅ **`constinit`** - Safe static initialization
- ✅ **Lambda template parameters** - More generic lambdas
- ✅ **`std::span`** - Safe array views
- ✅ **`std::format`** - Modern string formatting (C++20/23)
- ✅ Coroutines - *Could* use for lazy evaluation (experimental)

**Cons**:
- ⚠️ Requires modern compiler (GCC 10+, Clang 13+, MSVC 2019 16.11+)
- ⚠️ Some legacy systems may need compiler upgrades
- ⚠️ Ranges library implementation varies between compilers

**Use Cases**:
```cpp
// Concepts - Self-documenting, clear errors
template<typename T>
concept Rankable = requires(T t) {
    { std::hash<T>{}(t) } -> std::convertible_to<size_t>;
    { t == t } -> std::convertible_to<bool>;
};

template<Rankable T>
auto observe(auto pred, RankingFunction<T> rf) -> RankingFunction<T>;

// Ranges - Lazy composition
auto result = ranking_function
    | std::views::filter([](auto x) { return x.rank() < 10; })
    | std::views::transform([](auto x) { return x.value(); })
    | std::views::take(5);

// Three-way comparison for Rank
class Rank {
    auto operator<=>(const Rank&) const = default;
};

// Designated initializers
RankingElement elem{
    .value_promise = Promise([](){ return 42; }),
    .rank = Rank{0},
    .successor_promise = terminate_promise
};
```

**Verdict**: ✅✅ **Required standard**. Concepts and ranges are essential for the design.

**Why C++20 is non-negotiable**:
1. **Concepts eliminate SFINAE hell** - Template-heavy library would be unmaintainable without concepts
2. **Ranges match lazy evaluation** - Natural fit for infinite ranking sequences
3. **Better error messages** - Critical for user-facing API
4. **Modern tooling** - IDE support, static analysis work better
5. **Industry adoption** - C++20 is mature enough (2025) for production use

---

#### **C++23** (Future-Proofing)

**Pros**:
- ✅ `std::expected<T, E>` - Better than `std::optional` for errors
- ✅ `std::flat_map` - Better performance for small maps
- ✅ `std::generator` - Coroutine support formalized
- ✅ `std::print` - Better formatting
- ✅ `if consteval` - Distinguish compile/runtime contexts
- ✅ Deducing `this` - Better CRTP patterns

**Cons**:
- ❌ Very limited compiler support (GCC 13+, Clang 16+, MSVC 2022 17.6+)
- ❌ Not ready for production (as of Oct 2025)
- ❌ May require cutting-edge compilers

**Verdict**: ⏳ **Too early**. Consider for v2.0+ after ecosystem matures.

---

### Implementation Strategy

#### **Core Library Configuration**
```cmake
# CMakeLists.txt
cmake_minimum_required(VERSION 3.20)
project(ranked_belief 
    VERSION 1.0.0
    LANGUAGES CXX
)

# C++20 required
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Verify compiler support
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 10.0)
    message(FATAL_ERROR "GCC 10 or higher required (found ${CMAKE_CXX_COMPILER_VERSION})")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 13.0)
    message(FATAL_ERROR "Clang 13 or higher required (found ${CMAKE_CXX_COMPILER_VERSION})")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 19.29)
    message(FATAL_ERROR "MSVC 2019 16.11 or higher required (found ${CMAKE_CXX_COMPILER_VERSION})")
endif()

# Check for required features
include(CheckCXXSourceCompiles)
check_cxx_source_compiles("
    #include <concepts>
    template<typename T>
    concept Hashable = requires(T t) { std::hash<T>{}(t); };
    int main() {}
" HAVE_CONCEPTS)

if(NOT HAVE_CONCEPTS)
    message(FATAL_ERROR "C++20 concepts support required but not available")
endif()

check_cxx_source_compiles("
    #include <ranges>
    int main() {
        auto v = std::views::filter([](int x){ return x > 0; });
    }
" HAVE_RANGES)

if(NOT HAVE_RANGES)
    message(WARNING "C++20 ranges support incomplete - some features may be limited")
endif()
```

#### **Code Style: Pure C++20**

```cpp
// ranked_belief/config.hpp
// No version checking needed - C++20 required

// ranked_belief/concepts.hpp
export module ranked_belief:concepts;

import <concepts>;
import <type_traits>;

namespace ranked_belief {

// Core concepts
template<typename T>
concept Rankable = requires(T t) {
    { std::hash<T>{}(t) } -> std::convertible_to<size_t>;
    { t == t } -> std::convertible_to<bool>;
};

template<typename F, typename T>
concept Predicate = requires(F f, T t) {
    { f(t) } -> std::convertible_to<bool>;
};

template<typename T>
concept RankingType = requires(T t) {
    { t.rank() } -> std::convertible_to<Rank>;
    { t.value() } -> Rankable;
};

} // namespace ranked_belief

// ranked_belief/autocast.hpp
export module ranked_belief:autocast;

import :concepts;
import :ranking_function;

namespace ranked_belief {

// Clean concept-based autocast
template<typename T>
auto autocast(T&& value) -> RankingFunction<std::decay_t<T>>
    requires (!RankingType<T>)
{
    return truth(std::forward<T>(value));
}

template<RankingType T>
auto autocast(T&& value) -> T {
    return std::forward<T>(value);
}

} // namespace ranked_belief

// ranked_belief/ranking_function.hpp
export module ranked_belief:ranking_function;

import <ranges>;
import <concepts>;
import :concepts;

namespace ranked_belief {

template<Rankable T>
class RankingFunction {
public:
    // Range interface (C++20 ranges)
    auto view() const {
        return std::views::transform(/*...*/);
    }
    
    // Pipe operator support
    template<std::invocable<RankingFunction<T>> F>
    auto operator|(F&& func) const {
        return std::forward<F>(func)(*this);
    }
};

} // namespace ranked_belief
```

#### **Compiler Requirements**

**Minimum Versions for C++20 Support**:

| Compiler | Minimum Version | Recommended | Notes |
|----------|----------------|-------------|-------|
| GCC      | 10.0           | 11.0+       | Full concepts/ranges from 10.0 |
| Clang    | 13.0           | 15.0+       | Ranges fully stable from 13.0 |
| MSVC     | 2019 16.11     | 2022 17.0+  | /std:c++20 flag required |
| AppleClang | 14.0         | 15.0+       | Based on Clang 13+ |

**Verification**:
```cpp
// Compile-time checks in config.hpp
namespace ranked_belief {
    // Verify C++20
    static_assert(__cplusplus >= 202002L, 
        "C++20 or later required");
    
    // Verify concepts
    static_assert(__cpp_concepts >= 201907L,
        "C++20 concepts support required");
    
    // Verify ranges
    static_assert(__cpp_lib_ranges >= 201911L,
        "C++20 ranges library required");
}
```

### Design Benefits from C++20

#### **Concepts Enable Clean API**:
```cpp
// Self-documenting, constraint-checked
template<Rankable T>
auto observe(Predicate<T> auto pred, RankingFunction<T> rf) 
    -> RankingFunction<T>;

// Clear error messages when constraints not met
error: constraints not satisfied for template 'observe'
note: concept 'Rankable<MyType>' was not satisfied
note: required for 'std::hash<MyType>' to be valid
    requires { std::hash<T>{}(t) } -> std::convertible_to<size_t>;
                ^~~~~~~~~~~~~~~
```

#### **Ranges Enable Natural Composition**:
```cpp
// Lazy, composable operations
auto result = ranking_function
    | std::views::filter(pred)
    | std::views::transform(func)
    | std::views::take(10);

// Works seamlessly with algorithms
auto max_rank = std::ranges::max_element(
    ranking_function,
    {}, 
    &RankingElement::rank
);
```

#### **Three-Way Comparison Simplifies Rank**:
```cpp
class Rank {
    uint64_t value_;
    bool is_infinite_;
    
    // All comparison operators in one line
    auto operator<=>(const Rank& other) const = default;
};
```

#### **Designated Initializers Improve Clarity**:
```cpp
// Clear, self-documenting construction
auto elem = RankingElement{
    .value_promise = Promise([](){ return 42; }),
    .rank = Rank{0},
    .successor_promise = next_promise
};
```

### Final Decision

**For ranked_belief v1.0+**:

✅✅ **Required: C++20**
- Concepts for all template constraints
- Ranges for lazy evaluation
- Modules for better compile times (optional)
- Three-way comparison for value types

❌ **No Support for C++17 or earlier**
- Concepts are essential for maintainability
- Ranges are essential for ergonomics
- SFINAE would make codebase unmaintainable
- Error messages would be unusable for users

**Migration path for C++17 users**: Upgrade compiler (widely available in 2025)

### Documentation Strategy

```markdown
## Requirements

**C++20 compiler required**:
- GCC 10.0 or higher (recommended: 11+)
- Clang 13.0 or higher (recommended: 15+)
- MSVC 2019 16.11 or higher (recommended: 2022)
- AppleClang 14.0 or higher

### Why C++20?

ranked_belief is a template-heavy library that relies on modern C++20 features:

- **Concepts**: Provide clear, self-documenting constraints and readable error messages
- **Ranges**: Enable natural lazy evaluation and composition
- **Modules**: Improve compile times (optional, gradually adopted)
- **Better tooling**: Modern IDEs and static analyzers understand C++20 better

### Checking Your Compiler

```bash
# GCC
g++ --version  # Need 10.0+

# Clang
clang++ --version  # Need 13.0+

# Test compilation
echo 'import <concepts>; int main(){}' | g++ -std=c++20 -x c++ -
```

### Installation Help

If your system compiler is too old:

- **Ubuntu/Debian**: `apt install g++-11` or higher
- **Fedora/RHEL**: `dnf install gcc-c++` (11+ in recent releases)
- **macOS**: `brew install llvm` for latest Clang
- **Windows**: Download Visual Studio 2022 Community
```

## Build System

```cmake
CMake-based build:
- Header-only core (templates)
- Shared library for language bindings
- C API layer for FFI compatibility
- **C++20 required** - GCC 10+, Clang 13+, MSVC 2019 16.11+, AppleClang 14+
- CMake 3.20+ required (for C++20 feature detection)

Build Targets:
1. ranked_belief (header-only) - C++ direct use
2. libranked_belief.so/.dylib/.dll - Shared library for bindings
3. Python wheel package
4. R package (.tar.gz)

CMake Configuration:
```cmake
cmake_minimum_required(VERSION 3.20)
project(ranked_belief VERSION 1.0.0 LANGUAGES CXX)

# C++20 required, no fallback
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Verify compiler support
include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-std=c++20" COMPILER_SUPPORTS_CXX20)
if(NOT COMPILER_SUPPORTS_CXX20)
    message(FATAL_ERROR "C++20 support required. Please upgrade your compiler.")
endif()
```

CMake Options:
- RANKED_BUILD_TESTS=ON (build test suite)
- RANKED_BUILD_EXAMPLES=ON (build examples)
- RANKED_BUILD_PYTHON=ON (build Python bindings)
- RANKED_BUILD_R=ON (build R bindings)
- RANKED_USE_MODULES=OFF (experimental C++20 modules support)

Installation:
- C++ library via CMake install (header-only + shared lib)
- Python: pip install ranked-belief (includes compiled extension)
- R: install.packages("ranked.belief")

Compiler Detection:
The build system automatically verifies:
- C++20 standard support
- Concepts feature availability
- Ranges library availability
- Three-way comparison support
```

## Performance Considerations

### 1. Lazy Evaluation Overhead
- **Trade-off**: Small per-element overhead vs. infinite sequences
- **Mitigation**: Inline Promise operations, compiler optimization

### 2. Memory Usage
- **Issue**: Long chains retain all elements
- **Mitigation**: 
  * Weak references where possible
  * Streaming API for one-pass operations
  * User control over materialization

### 3. Caching Strategy
- **Promise memoization**: Store computed values
- **Thread safety**: Use std::call_once for concurrent access
- **Optional**: Disable caching for memory-constrained scenarios

## Thread Safety

```
Design options:
1. Thread-compatible (not thread-safe): User manages synchronization
2. Thread-safe Promise evaluation: std::call_once in force()
3. Immutable ranking functions: Safe to share across threads

Recommendation: Option 2 + 3 (thread-safe lazy eval, immutable RF)
```

## Testing Strategy

```
1. Unit tests for each operation
2. Property-based testing (correctness properties)
3. Port all Racket examples as integration tests
4. Performance benchmarks
5. Memory leak detection (valgrind)
6. Fuzzing for robustness
```

## Migration Path from Racket

```
Racket Construct          ->  C++ Equivalent
─────────────────────────────────────────────────────
(delay expr)              ->  Promise([](){ return expr; })
(force promise)           ->  promise.force()
(nrm/exc a b)             ->  nrm_exc(a, b)
(observe pred rf)         ->  observe(pred, rf)
($ f x y)                 ->  ranked_apply(f, x, y)
(rlet ((x rf)) body)      ->  rlet(rf, [](auto x) { return body; })
(pr rf)                   ->  pr_first(10, rf, std::cout)
autocast (implicit)       ->  autocast() (may be implicit via templates)
```

### Autocast Examples

**Racket**:
```racket
; Autocast happens implicitly
(define rf1 (nrm/exc 1 2))
(define result (observe (λ (x) (> x 0)) rf1))  ; rf1 is ranking
(define result2 (observe (λ (x) (> x 0)) 5))   ; 5 autocast to singleton
```

**C++**:
```cpp
// Autocast via template overloads
auto rf1 = nrm_exc(1, 2);
auto result = observe([](int x) { return x > 0; }, rf1);   // rf1 is ranking
auto result2 = observe([](int x) { return x > 0; }, 5);    // 5 autocast

// Explicit if needed
auto result3 = observe([](int x) { return x > 0; }, truth(5));
```

### Ranked Application Examples

**Racket**:
```racket
; Case 1: Regular function
($ + (nrm/exc 1 2) (nrm/exc 3 4))

; Case 2: Function returning ranking
(define (stoch-add a b) (nrm/exc (+ a b) (+ a b 1)))
($ stoch-add (nrm/exc 1 2) (nrm/exc 3 4))

; Case 3: Ranking over functions
($ (either/or + -) (nrm/exc 1 2) (nrm/exc 3 4))
```

**C++**:
```cpp
// Case 1: Regular function (pure)
ranked_apply(std::plus<int>(), nrm_exc(1, 2), nrm_exc(3, 4));

// Case 2: Function returning ranking
auto stoch_add = [](int a, int b) { return nrm_exc(a + b, a + b + 1); };
ranked_apply(stoch_add, nrm_exc(1, 2), nrm_exc(3, 4));

// Case 3: Ranking over functions
auto ops = either_of({std::plus<int>(), std::minus<int>()});
ranked_apply(ops, nrm_exc(1, 2), nrm_exc(3, 4));
```

## Language Bindings

### Overview
The library will provide first-class support for Python and R through well-designed bindings.

### C API Layer

**Purpose**: Stable ABI for language bindings

```c
// C API for FFI compatibility (c_api.h)
typedef struct rp_ranking* rp_ranking_t;
typedef uint64_t rp_rank_t;
typedef struct rp_element* rp_element_t;

// Core operations
rp_ranking_t rp_nrm_exc(const char* normal, const char* exceptional, rp_rank_t rank);
rp_ranking_t rp_observe(rp_ranking_t rf, bool (*predicate)(const char*));
rp_ranking_t rp_filter(rp_ranking_t rf, bool (*predicate)(const char*));
rp_ranking_t rp_merge(rp_ranking_t rf1, rp_ranking_t rf2);

// Iteration
rp_element_t rp_first(rp_ranking_t rf);
rp_element_t rp_next(rp_element_t elem);
const char* rp_element_value(rp_element_t elem);
rp_rank_t rp_element_rank(rp_element_t elem);
bool rp_element_is_terminal(rp_element_t elem);

// Memory management
void rp_ranking_free(rp_ranking_t rf);
void rp_element_free(rp_element_t elem);

// Type-specific constructors for numeric, string, bool
rp_ranking_t rp_nrm_exc_int(int64_t normal, int64_t exceptional, rp_rank_t rank);
rp_ranking_t rp_nrm_exc_double(double normal, double exceptional, rp_rank_t rank);
```

**Design Considerations**:
- C-compatible types only
- Opaque handles for objects
- Explicit memory management
- Type-erased values (polymorphic)
- Error codes for error handling

### Python Bindings

**Strategy**: Use **pybind11** for modern C++ integration (preferred over Cython for template-heavy code)

**Architecture**:
```python
# Python API design
from ranked_belief import *

# Construction
rf1 = nrm_exc("A", "B")
rf2 = nrm_exc(1, 2, rank=3)
rf3 = either_of([1, 2, 3, 4])

# Operations (Pythonic API)
result = rf1.observe(lambda x: x == "A")
result = rf1.filter(lambda x: len(x) > 2)
result = rf1.map(lambda x: x.upper())

# Iteration
for value, rank in rf1:
    print(f"{value}: {rank}")

# Display
rf1.pr()           # Print first 10
rf1.pr_until(5)    # Print up to rank 5
rf1.pr_all()       # Print all (careful!)

# Conversion
dict_form = rf1.to_dict()      # {value: rank}
list_form = rf1.to_list()      # [(value, rank), ...]
df = rf1.to_dataframe()        # pandas DataFrame

# Ranked let
result = rlet(rf1, lambda x: 
    rlet(rf2, lambda y: 
        truth(x + y)))
```

**Implementation via pybind11**:
```cpp
// Python bindings with pybind11
PYBIND11_MODULE(_ranked_belief, m) {
    py::class_<RankingFunction<py::object>>(m, "RankingFunction")
        .def("observe", &RankingFunction<py::object>::observe)
        .def("filter", &RankingFunction<py::object>::filter)
        .def("map", &RankingFunction<py::object>::map)
        .def("__iter__", [](RankingFunction<py::object>& rf) {
            return py::make_iterator(rf.begin(), rf.end());
        })
        .def("pr", &pr_first_py, py::arg("n") = 10)
        .def("to_dict", &to_dict_py);
    
    m.def("nrm_exc", &nrm_exc_py);
    m.def("observe", &observe_py);
    m.def("rlet", &rlet_py);
}
```

**Features**:
- Pythonic naming conventions (snake_case)
- Lambda support for predicates/transforms
- NumPy array support
- Pandas DataFrame integration
- Pickle support for serialization
- Type hints (PEP 484)

### R Bindings

**Strategy**: Use **Rcpp** for seamless R/C++ integration

**Architecture**:
```r
# R API design
library(ranked.belief)

# Construction
rf1 <- nrm_exc("A", "B")
rf2 <- nrm_exc(1L, 2L, rank = 3L)
rf3 <- either_of(c(1, 2, 3, 4))

# Operations (R-style API)
result <- observe(rf1, function(x) x == "A")
result <- filter_rf(rf1, function(x) nchar(x) > 2)
result <- map_rf(rf1, toupper)

# Piping support (magrittr/dplyr style)
library(magrittr)
result <- rf1 %>% 
  observe(function(x) x == "A") %>%
  map_rf(toupper)

# Display
pr(rf1)              # Print first 10
pr_until(rf1, 5)     # Print up to rank 5
pr_all(rf1)          # Print all

# Conversion
df <- as.data.frame(rf1)    # data.frame with value, rank columns
list_form <- as.list(rf1)    # named list
vec <- as.vector(rf1)        # just values

# Ranked let
result <- rlet(rf1, function(x) {
  rlet(rf2, function(y) {
    truth(x + y)
  })
})
```

**Implementation via Rcpp**:
```cpp
// R bindings with Rcpp
// [[Rcpp::export]]
SEXP nrm_exc_r(SEXP normal, SEXP exceptional, int rank = 1) {
    // Type dispatch based on SEXPTYPE
    // Return wrapped RankingFunction
}

// [[Rcpp::export]]
SEXP observe_r(SEXP rf, Rcpp::Function predicate) {
    // Extract RankingFunction, apply predicate
}

// S3 methods
// print.RankingFunction
// as.data.frame.RankingFunction
// as.list.RankingFunction
```

**Features**:
- R naming conventions (dots for generic.method)
- S3 class system integration
- data.frame/tibble support
- %>% pipe operator compatibility
- R function support for predicates
- Native R vector types

### Binding Maintenance Strategy

**Type Handling**:
```
C++ Templates  ->  Type erasure  ->  Dynamic typing in Python/R

Strategy:
1. Common types (int, double, string, bool) via specialized paths
2. Complex types serialized/wrapped
3. User objects via pickling (Python) / serialization (R)
```

**Memory Management**:
```
- C++: RAII with smart pointers
- Python: Reference counting, automatic GC
- R: PROTECT/UNPROTECT, automatic GC
- C API: Explicit free functions

Approach:
- Wrap C++ objects in language-specific handles
- Finalizers for automatic cleanup
- Explicit dispose() methods for manual control
```

**Error Handling**:
```
C++ Exceptions  ->  Language-specific exceptions

- Python: Translate to Python exceptions via pybind11
- R: Translate to R errors via Rcpp
- C API: Return error codes, set error state
```

**Performance Considerations**:
- Minimize crossing language boundaries
- Batch operations where possible
- Lazy evaluation preserved across boundary
- Zero-copy when possible (shared memory buffers)

### Documentation & Examples

Each binding includes:
- Installation instructions
- API reference
- Tutorial with examples
- Migration guide from Racket
- Performance guidelines
- Type handling documentation

## Open Design Questions

### Core Library

1. **Should Promise be copyable or move-only?**
   - Move-only is safer but less flexible
   - Copyable requires shared state (thread sync)
   - **Binding impact**: Copyable may be easier for FFI
   - **Extension impact**: Affects how extensions can manipulate promises
   
2. **Template-heavy vs. runtime polymorphism?**
   - Templates: Zero-cost abstractions, complex compilation
   - Virtual: Easier compilation, runtime overhead
   - **Binding impact**: Type erasure layer needed either way
   - **Extension impact**: Templates allow extensions to be header-only
   
3. **Should we support dynamic typing (like Racket)?**
   - Use std::variant or std::any?
   - Or keep strictly statically typed?
   - **Binding impact**: Dynamic typing essential for Python/R
   - **Extension impact**: Affects how extensions handle heterogeneous data
   
4. **Value serialization format?**
   - JSON for cross-language compatibility?
   - MessagePack for efficiency?
   - Custom binary format?
   - **Extension impact**: Causal models, networks need serialization

### Extension Architecture

5. **Extension loading mechanism?**
   - Header-only extensions (compile-time)
   - Dynamic plugin loading (runtime)
   - Hybrid approach?
   - **Trade-off**: Flexibility vs. performance vs. complexity

6. **Extension interdependencies?**
   - Can extensions depend on each other?
   - Directed acyclic graph of dependencies?
   - Core provides dependency injection framework?
   
7. **Language binding strategy for extensions?**
   - Each extension provides own Python/R bindings?
   - Core binding framework used by extensions?
   - **Recommendation**: Core framework + extension-specific wrappers

8. **Version compatibility for extensions?**
   - Extensions tied to specific core version?
   - ABI stability requirements?
   - Plugin API versioning scheme?

### Specific Extension Questions

9. **Graph library choice for propagation module?**
   - Boost.Graph (heavy dependency)
   - Custom lightweight graph (more work)
   - Header-only graph library (e.g., networkit)
   
10. **SMT solver flexibility in constraints module?**
    - Z3 only (simpler)
    - Abstract solver interface (more flexible)
    - **Recommendation**: Abstract interface with Z3 as default

11. **Performance targets for large-scale inference?**
    - How many nodes in ranking network?
    - What latency requirements?
    - GPU acceleration needed?

### Language Binding Questions

12. **Python binding approach?**
    - pybind11 (modern, template-friendly)
    - Cython (Python-focused, manual)
    - SWIG (multi-language, outdated)
    - **Recommendation**: pybind11 for core + extensions

13. **Extension discoverability in Python/R?**
    - Automatic discovery of installed extensions?
    - Explicit imports?
    ```python
    # Option A: Explicit
    from ranked_belief.extensions import causal
    
    # Option B: Automatic
    import ranked_belief as rb
    if rb.has_extension('causal'):
        # Use causal features
    ```

## Next Steps

### Phase 1: Core C++ Library (Weeks 1-4)
1. **Prototype Phase**:
   - Implement Promise class
   - Implement basic RankingElement
   - Test lazy evaluation mechanics

2. **Core Implementation**:
   - Implement all operations from rp-core.rkt
   - Port basic API functions
   - Write unit tests

3. **API Layer**:
   - Implement user-facing C++ API
   - Add convenience functions
   - Create examples

4. **Validation**:
   - Port all Racket examples
   - Verify output matches
   - Performance benchmarking

### Phase 2: C API & Bindings Infrastructure (Weeks 5-6)
1. **C API Layer**:
   - Design stable C interface
   - Implement type erasure
   - Test FFI compatibility

2. **Shared Library Build**:
   - CMake configuration for shared lib
   - Symbol export management
   - Cross-platform testing

### Phase 3: Python Bindings (Weeks 7-9)
1. **pybind11 Integration**:
   - Implement core bindings
   - Python wrapper classes
   - Type conversions

2. **Python API Design**:
   - Pythonic interface
   - NumPy/Pandas integration
   - Comprehensive tests

3. **Python Packaging**:
   - setup.py configuration
   - Wheel building (manylinux)
   - PyPI preparation

### Phase 4: R Bindings (Weeks 10-12)
1. **Rcpp Integration**:
   - Implement core bindings
   - S3 class system
   - Type conversions

2. **R API Design**:
   - R-style interface
   - data.frame integration
   - Comprehensive tests

3. **R Packaging**:
   - CRAN-compliant structure
   - Documentation (roxygen2)
   - CRAN submission prep

### Phase 5: Documentation & Release (Weeks 13-14)
1. **Documentation**:
   - C++ API reference (Doxygen)
   - Python API reference (Sphinx)
   - R API reference (pkgdown)
   - Tutorials for each language
   - Migration guide from Racket

2. **Release Preparation**:
   - Version 1.0.0 release
   - GitHub releases
   - PyPI publication
   - CRAN submission
   - Announcement blog post

### Phase 6+: Extension Development (Future Roadmap)

**Phase 6: Causal Reasoning (Weeks 15-20)**
- Structural causal models
- Do-calculus implementation
- Counterfactual reasoning
- Python/R bindings
- Tutorial and examples

**Phase 7: Belief Propagation (Weeks 21-26)**
- Ranking network structure
- Shenoy's algorithm
- Junction tree implementation
- Large-scale optimization
- Python/R bindings

**Phase 8: Constraint Solving (Weeks 27-32)**
- Z3 integration
- SMT encoding for rankings
- MaxSAT/MaxSMT solver
- Python/R bindings
- Integration examples

**Phase 9: C-Representations (Weeks 33-36)**
- Knowledge base structure
- System Z implementation
- Inference engine
- Python/R bindings
- Epistemic reasoning examples

**Phase 10: Optimization (Weeks 37-42)**
- Multi-objective optimization
- Constraint programming
- Integration with other extensions
- Python/R bindings
- Real-world applications

**Version Roadmap**:
- v1.0: Core library + Python/R bindings
- v1.1: Causal reasoning extension
- v1.2: Belief propagation extension
- v1.3: Constraint solving extension
- v1.4: C-representations extension
- v1.5: Optimization extension
- v2.0: Stabilized extension APIs, performance optimizations

## Conclusion

The C++ port requires careful attention to **lazy evaluation** which is fundamental to the design. The recommended approach uses template-based `Promise` class combined with `RankingElement` linked list structure. This preserves the compositional, lazy nature of the Racket implementation while providing type safety and performance characteristics expected in C++.

As a **shared library with Python and R bindings**, the design must balance:
- **C++ Performance**: Zero-cost abstractions for native users
- **FFI Compatibility**: Clean C API for language bindings
- **Language Idioms**: Pythonic and R-style interfaces for each binding
- **Type Safety**: Static typing in C++, dynamic typing across FFI
- **Memory Safety**: RAII in C++, automatic GC in Python/R

The design prioritizes:
- **Correctness**: Faithful port of semantics across all languages
- **Efficiency**: Zero-cost abstractions in C++, minimal overhead in bindings
- **Usability**: Native idioms for C++, Python, and R users
- **Safety**: RAII, smart pointers, automatic memory management
- **Maintainability**: Single C++ implementation, thin binding layers
- **Accessibility**: Easy installation via CMake, pip, and install.packages()
- **Extensibility**: Modular architecture supporting advanced features without core refactoring

### Extensibility Strategy

The design explicitly accommodates future extensions through:

1. **Stable Core API**: Core ranking operations remain stable while extensions build on top
2. **Plugin Architecture**: Extensions as optional modules with clear boundaries
3. **Minimal Dependencies**: Core has no external dependencies; extensions opt-in to heavy deps
4. **Compositional Design**: Extensions can be combined (e.g., causal + constraints)
5. **Versioned Extensions**: Independent versioning allows rapid iteration
6. **Language Bindings**: Each extension provides bindings, leveraging core framework

This modular approach enables:
- Adding sophisticated features (causal inference, belief propagation, SMT solving)
- Maintaining a lightweight core for basic use cases
- Supporting advanced research applications
- Evolving extensions independently without breaking the core
- Easy maintenance and testing of each component
