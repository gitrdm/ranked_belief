/**
 * @file promise.hpp
 * @brief Lazy evaluation mechanism for ranked belief computations.
 *
 * The Promise<T> class provides deferred computation with memoization, essential for
 * representing infinite ranking sequences efficiently. Once forced, the computation
 * result is cached for subsequent accesses.
 *
 * This implementation is thread-safe, ensuring that the computation function is
 * executed exactly once even under concurrent access.
 *
 * Design decisions:
 * - Move-only semantics (no copying) to avoid expensive computation duplication
 * - Thread-safe memoization using std::call_once
 * - Exception safety: exceptions from computations are propagated and re-thrown
 * - Const-correct: force() is logically const (memoization is implementation detail)
 */

#ifndef RANKED_BELIEF_PROMISE_HPP
#define RANKED_BELIEF_PROMISE_HPP

#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <utility>

namespace ranked_belief {

/**
 * @class Promise
 * @brief A lazy computation that produces a value of type T when forced.
 *
 * Promise<T> encapsulates a deferred computation that is executed at most once,
 * with the result cached for subsequent accesses. This is essential for lazy
 * evaluation in ranking functions, particularly for infinite sequences.
 *
 * Thread Safety: Multiple threads can safely call force() concurrently. The
 * computation will be executed exactly once, and all threads will receive the
 * same result.
 *
 * Exception Safety: If the computation throws an exception during the first
 * force(), that exception is captured and will be re-thrown on all subsequent
 * force() calls. This ensures consistent behavior across all accessors.
 *
 * @tparam T The type of value produced by the promise.
 *
 * Example:
 * @code
 * Promise<int> p([]() { return expensive_computation(); });
 * int result = p.force();  // Computation happens here
 * int cached = p.force();  // Returns cached result
 * @endcode
 *
 * @invariant After a successful force(), value_ contains the result.
 * @invariant After a failed force(), exception_ contains the exception.
 * @invariant computation_ is cleared after the first force() attempt.
 */
template<typename T>
class Promise {
public:
    /**
     * @brief Construct a promise from a computation function.
     *
     * The computation will not be executed until force() is called.
     *
     * @param computation A callable that produces a value of type T.
     * @throws std::invalid_argument if computation is null/empty.
     */
    explicit Promise(std::function<T()> computation) : computation_(std::move(computation)) {
        if (!computation_) {
            throw std::invalid_argument("Promise computation function cannot be null");
        }
    }

    /**
     * @brief Construct a promise from an already-computed value.
     *
     * This creates a promise that is already forced with the given value.
     * Useful for wrapping concrete values in promise-based APIs.
     *
     * @param value The pre-computed value to wrap.
     */
    explicit Promise(T value)
        : computation_(nullptr), value_(std::make_unique<T>(std::move(value))) {
        // Mark as already forced by setting flag
        std::call_once(force_flag_, []() {});
    }

    /**
     * @brief Move constructor.
     *
     * Note: std::once_flag is not movable, so we cannot properly move promises
     * after they've been forced. The moved-from promise will be in a valid but
     * potentially unusable state.
     *
     * @param other The promise to move from.
     */
    Promise(Promise&& other) noexcept
        : computation_(std::move(other.computation_)),
          value_(std::move(other.value_)),
          exception_(std::move(other.exception_)) {
        // Note: force_flag_ cannot be moved (std::once_flag is not movable)
        // This means a moved-from promise that was already forced will have
        // issues if force() is called on it again
    }

    /**
     * @brief Move assignment operator.
     *
     * Note: std::once_flag is not movable, so this assignment only moves the
     * computation, value, and exception. The force state is not transferred.
     *
     * @param other The promise to move from.
     * @return Reference to this promise.
     */
    Promise& operator=(Promise&& other) noexcept {
        if (this != &other) {
            computation_ = std::move(other.computation_);
            value_ = std::move(other.value_);
            exception_ = std::move(other.exception_);
            // Note: force_flag_ cannot be moved/assigned
        }
        return *this;
    }

    // Disable copying to prevent expensive computation duplication
    Promise(const Promise&) = delete;
    Promise& operator=(const Promise&) = delete;

    /**
     * @brief Force evaluation of the promise and return the result.
     *
     * On the first call, executes the computation function and caches the result.
     * Subsequent calls return the cached result without re-executing the computation.
     *
     * Thread Safety: Safe to call from multiple threads concurrently. The computation
     * will be executed exactly once.
     *
     * @return Reference to the computed value.
     * @throws Any exception thrown by the computation function (re-thrown on all calls).
     * @throws std::logic_error if the promise was moved from.
     */
    T& force() {
        std::call_once(force_flag_, [this]() { execute_computation(); });

        if (exception_) {
            std::rethrow_exception(exception_);
        }

        if (!value_) {
            throw std::logic_error("Promise is in invalid state (possibly moved from)");
        }

        return *value_;
    }

    /**
     * @brief Force evaluation and return const reference to the result.
     *
     * Const version of force(). Despite being const, this method may execute the
     * computation and update internal state (memoization is an implementation detail).
     *
     * @return Const reference to the computed value.
     * @throws Any exception thrown by the computation function.
     * @throws std::logic_error if the promise was moved from.
     */
    const T& force() const {
        std::call_once(force_flag_, [this]() { execute_computation(); });

        if (exception_) {
            std::rethrow_exception(exception_);
        }

        if (!value_) {
            throw std::logic_error("Promise is in invalid state (possibly moved from)");
        }

        return *value_;
    }

    /**
     * @brief Check if the promise has been forced.
     *
     * @return true if force() has been called (successfully or not), false otherwise.
     */
    [[nodiscard]] bool is_forced() const noexcept {
        // Check if the once_flag has been invoked by attempting to call with empty lambda
        // Since we can't directly query once_flag, we check if value or exception is set
        return value_ != nullptr || exception_ != nullptr;
    }

    /**
     * @brief Check if the promise has a value (forced successfully).
     *
     * @return true if the promise has been forced and has a value, false otherwise.
     */
    [[nodiscard]] bool has_value() const noexcept { return value_ != nullptr; }

    /**
     * @brief Check if the promise has an exception (forced with failure).
     *
     * @return true if the promise was forced and threw an exception, false otherwise.
     */
    [[nodiscard]] bool has_exception() const noexcept { return exception_ != nullptr; }

private:
    /**
     * @brief Execute the computation and store result or exception.
     *
     * This method is called exactly once by std::call_once. It captures either
     * the successful result or any thrown exception for future access.
     *
     * Special case: If the promise was moved from an already-forced promise,
     * value_ will already be set and computation_ will be null. In this case,
     * we do nothing (the value is already available).
     */
    void execute_computation() const {
        // If value is already set (from move of forced promise), don't execute
        if (const_cast<const Promise*>(this)->value_) {
            return;
        }

        try {
            if (!computation_) {
                throw std::logic_error("Cannot force a promise without a computation function");
            }
            // Execute computation and store result
            // Use const_cast since this is memoization (implementation detail)
            const_cast<Promise*>(this)->value_ = std::make_unique<T>(computation_());
            // Clear computation to free resources
            const_cast<Promise*>(this)->computation_ = nullptr;
        } catch (...) {
            // Capture exception for re-throwing
            const_cast<Promise*>(this)->exception_ = std::current_exception();
            // Clear computation even on failure
            const_cast<Promise*>(this)->computation_ = nullptr;
        }
    }

    std::function<T()> computation_;           ///< The deferred computation
    mutable std::unique_ptr<T> value_;         ///< Cached result (nullptr until forced)
    mutable std::exception_ptr exception_;     ///< Cached exception if computation failed
    mutable std::once_flag force_flag_;        ///< Ensures single execution
};

/**
 * @brief Helper function to create a promise from a computation.
 *
 * Provides type deduction for convenient promise creation.
 *
 * @tparam F The type of the computation function.
 * @param computation The computation to defer.
 * @return A promise that will execute the computation when forced.
 *
 * Example:
 * @code
 * auto p = make_promise([]() { return 42; });
 * @endcode
 */
template<typename F>
[[nodiscard]] auto make_promise(F&& computation)
    -> Promise<std::invoke_result_t<std::decay_t<F>>> {
    using T = std::invoke_result_t<std::decay_t<F>>;
    return Promise<T>(std::function<T()>(std::forward<F>(computation)));
}

/**
 * @brief Helper function to create an already-forced promise from a value.
 *
 * Provides type deduction for convenient promise creation from values.
 *
 * @tparam T The type of the value (deduced).
 * @param value The value to wrap in a promise.
 * @return A promise that is already forced with the given value.
 *
 * Example:
 * @code
 * auto p = make_promise_value(42);
 * @endcode
 */
template<typename T>
[[nodiscard]] Promise<std::decay_t<T>> make_promise_value(T&& value) {
    return Promise<std::decay_t<T>>(std::forward<T>(value));
}

}  // namespace ranked_belief

#endif  // RANKED_BELIEF_PROMISE_HPP
