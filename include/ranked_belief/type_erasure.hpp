/**
 * @file type_erasure.hpp
 * @brief Type-erased façade for `RankingFunction`.
 *
 * Provides the `RankingFunctionAny` wrapper which hides the concrete value
 * type stored inside a `RankingFunction` while preserving the lazy semantics of
 * the underlying computations. The wrapper exposes a reduced API that operates
 * on `std::any` values and reuses the existing lazy operations (map/filter/
 * merge/observe/merge-apply/etc.) under the hood.
 */

#pragma once

#include "ranked_belief/operations/filter.hpp"
#include "ranked_belief/operations/map.hpp"
#include "ranked_belief/operations/merge.hpp"
#include "ranked_belief/operations/merge_apply.hpp"
#include "ranked_belief/operations/nrm_exc.hpp"
#include "ranked_belief/operations/observe.hpp"
#include "ranked_belief/ranking_function.hpp"

#include <any>
#include <concepts>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <typeinfo>
#include <utility>
#include <vector>

namespace ranked_belief {

namespace detail {

template<typename T>
concept EqualityComparable = requires(const T& lhs, const T& rhs) {
	{ lhs == rhs } -> std::convertible_to<bool>;
};

} // namespace detail

/**
 * @brief Lazily-typed ranking function wrapper using `std::any` as the public
 *        interchange value.
 *
 * The wrapper implements a type-erased façade that can be used by FFI layers
 * or dynamically typed consumers. Each operation delegates to the underlying
 * `RankingFunction<T>` instance while translating values to and from
 * `std::any`. The lazy semantics and memoisation guarantees of the library are
 * preserved because every method is expressed in terms of the existing lazy
 * primitives (`map`, `filter`, `merge`, `merge_apply`, `observe`, etc.).
 *
 * @note Operations that fundamentally rely on value equality (for example the
 *       equality-based `observe`) require the stored value type to be equality
 *       comparable. If an operation cannot be expressed safely on a
 *       type-erased value, a `std::logic_error` is thrown instead.
 */
class RankingFunctionAny {
public:
	/** @brief Construct an empty ranking. */
	RankingFunctionAny();

	/**
	 * @brief Construct from a concrete ranking function.
	 *
	 * @tparam T Underlying value type.
	 * @param rf Ranking function to wrap.
	 */
	template<typename T>
	explicit RankingFunctionAny(RankingFunction<T> rf)
		: impl_{make_shared(std::make_unique<Model<T>>(std::move(rf)))} {}

	RankingFunctionAny(const RankingFunctionAny&) = default;
	RankingFunctionAny(RankingFunctionAny&&) noexcept = default;
	RankingFunctionAny& operator=(const RankingFunctionAny&) = default;
	RankingFunctionAny& operator=(RankingFunctionAny&&) noexcept = default;
	~RankingFunctionAny() = default;

	/**
	 * @brief True when the underlying ranking has no elements.
	 */
	[[nodiscard]] bool is_empty() const;

	/**
	 * @brief Obtain the first value (most plausible) as `std::any`.
	 *
	 * @throws std::logic_error if the ranking is empty.
	 */
	[[nodiscard]] std::any first_value() const;

	/**
	 * @brief Obtain the rank of the first value if present.
	 */
	[[nodiscard]] std::optional<Rank> first_rank() const;

	/**
	 * @brief Apply a transformation lazily to every value.
	 *
	 * The transformation receives each value wrapped in `std::any` and must
	 * return another `std::any`. The resulting ranking stores values as
	 * `std::any` as well (deduplication defaults to disabled because equality
	 * between `std::any` instances is not defined).
	 */
	[[nodiscard]] RankingFunctionAny map(
		const std::function<std::any(const std::any&)>& func,
		bool deduplicate = false) const;

	/**
	 * @brief Map with rank-aware transformation.
	 */
	[[nodiscard]] RankingFunctionAny map_with_rank(
		const std::function<std::pair<std::any, Rank>(const std::any&, Rank)>& func,
		bool deduplicate = false) const;

	/**
	 * @brief Map with index-aware transformation.
	 */
	[[nodiscard]] RankingFunctionAny map_with_index(
		const std::function<std::any(const std::any&, std::size_t)>& func,
		bool deduplicate = false) const;

	/**
	 * @brief Filter the ranking using a predicate over `std::any`.
	 */
	[[nodiscard]] RankingFunctionAny filter(
		const std::function<bool(const std::any&)>& predicate,
		bool deduplicate = true) const;

	/**
	 * @brief Take the first @p n elements lazily.
	 */
	[[nodiscard]] RankingFunctionAny take(std::size_t n) const;

	/**
	 * @brief Take elements lazily while their ranks are below the threshold.
	 */
	[[nodiscard]] RankingFunctionAny take_while_rank(Rank max_rank) const;

	/**
	 * @brief Merge two rankings while preserving lazy semantics.
	 */
	[[nodiscard]] RankingFunctionAny merge(
		const RankingFunctionAny& other,
		bool deduplicate = true) const;

	/**
	 * @brief Merge an arbitrary collection of rankings.
	 */
	[[nodiscard]] static RankingFunctionAny merge_all(
		const std::vector<RankingFunctionAny>& rankings,
		bool deduplicate = true);

	/**
	 * @brief Applicative merge (lazy bind) operating on type-erased values.
	 *
	 * The @p functions ranking must contain values of type
	 * `std::function<RankingFunctionAny(const std::any&)>`. Each function is
	 * invoked lazily with the underlying value (wrapped in `std::any`) and must
	 * return another `RankingFunctionAny`. The returned ranking contains
	 * `std::any` values.
	 */
	[[nodiscard]] RankingFunctionAny merge_apply(
		const RankingFunctionAny& functions,
		bool deduplicate = false) const;

	/**
	 * @brief Condition the ranking on a predicate applied to `std::any` values.
	 */
	[[nodiscard]] RankingFunctionAny observe(
		const std::function<bool(const std::any&)>& predicate,
		bool deduplicate = true) const;

	/**
	 * @brief Condition on equality with a specific value.
	 *
	 * @throws std::bad_any_cast if the stored type does not match the argument.
	 * @throws std::logic_error if the stored type is not equality comparable.
	 */
	[[nodiscard]] RankingFunctionAny observe_value(
		const std::any& value,
		bool deduplicate = true) const;

	/**
	 * @brief Materialise the first @p n elements as `std::any`/`Rank` pairs.
	 */
	[[nodiscard]] std::vector<std::pair<std::any, Rank>> take_n(std::size_t n) const;

	/**
	 * @brief Attempt to retrieve the underlying typed ranking.
	 *
	 * @tparam T Expected value type.
	 * @throws std::bad_any_cast if the stored type differs.
	 */
	template<typename T>
	[[nodiscard]] RankingFunction<T> cast() const
	{
		auto model = dynamic_cast<const Model<T>*>(impl_.get());
		if (!model) {
			throw std::bad_any_cast();
		}
		return model->rf;
	}

	/**
	 * @brief Convert to a `RankingFunction<std::any>` representation.
	 *
	 * Deduplication is disabled because equality between `std::any` instances
	 * cannot be determined generically. This helper is mainly intended for
	 * diagnostic or bridge scenarios.
	 */
	[[nodiscard]] RankingFunction<std::any> to_any_ranking(bool deduplicate = false) const;

private:
	struct Concept {
		virtual ~Concept() = default;

		[[nodiscard]] virtual std::unique_ptr<Concept> clone() const = 0;
		[[nodiscard]] virtual bool is_empty() const = 0;
		[[nodiscard]] virtual std::any first_value() const = 0;
		[[nodiscard]] virtual std::optional<Rank> first_rank() const = 0;
		[[nodiscard]] virtual std::unique_ptr<Concept> map(
			const std::function<std::any(const std::any&)>& func,
			bool deduplicate) const = 0;
		[[nodiscard]] virtual std::unique_ptr<Concept> map_with_rank(
			const std::function<std::pair<std::any, Rank>(const std::any&, Rank)>& func,
			bool deduplicate) const = 0;
		[[nodiscard]] virtual std::unique_ptr<Concept> map_with_index(
			const std::function<std::any(const std::any&, std::size_t)>& func,
			bool deduplicate) const = 0;
		[[nodiscard]] virtual std::unique_ptr<Concept> filter(
			const std::function<bool(const std::any&)>& predicate,
			bool deduplicate) const = 0;
		[[nodiscard]] virtual std::unique_ptr<Concept> take(std::size_t n) const = 0;
		[[nodiscard]] virtual std::unique_ptr<Concept> take_while_rank(Rank max_rank) const = 0;
		[[nodiscard]] virtual std::unique_ptr<Concept> merge(
			const Concept& other,
			bool deduplicate) const = 0;
		[[nodiscard]] virtual std::unique_ptr<Concept> merge_apply(
			const Concept& functions,
			bool deduplicate) const = 0;
		[[nodiscard]] virtual std::unique_ptr<Concept> observe(
			const std::function<bool(const std::any&)>& predicate,
			bool deduplicate) const = 0;
		[[nodiscard]] virtual std::unique_ptr<Concept> observe_value(
			const std::any& value,
			bool deduplicate) const = 0;
		[[nodiscard]] virtual std::vector<std::pair<std::any, Rank>> take_n(std::size_t n) const = 0;
		[[nodiscard]] virtual RankingFunction<std::any> to_any_ranking(bool deduplicate) const = 0;
	};

	template<typename T>
	struct Model;

	[[nodiscard]] static std::shared_ptr<const Concept> make_shared(std::unique_ptr<Concept> impl);
	[[nodiscard]] static std::shared_ptr<const Concept> empty_impl();

	explicit RankingFunctionAny(std::shared_ptr<const Concept> impl)
		: impl_{std::move(impl)} {}

	std::shared_ptr<const Concept> impl_;
};

	template<typename T>
	struct RankingFunctionAny::Model final : Concept {
		explicit Model(RankingFunction<T> ranking)
			: rf{std::move(ranking)} {}

		[[nodiscard]] std::unique_ptr<Concept> clone() const override
		{
			return std::make_unique<Model<T>>(rf);
		}

		[[nodiscard]] bool is_empty() const override
		{
			return rf.is_empty();
		}

		[[nodiscard]] std::any first_value() const override
		{
			auto first = rf.first();
			if (!first) {
				throw std::logic_error{"RankingFunctionAny::first_value called on empty ranking"};
			}
			return std::any{first->first};
		}

		[[nodiscard]] std::optional<Rank> first_rank() const override
		{
			auto first = rf.first();
			if (!first) {
				return std::nullopt;
			}
			return first->second;
		}

		[[nodiscard]] std::unique_ptr<Concept> map(
			const std::function<std::any(const std::any&)>& func,
			bool deduplicate) const override
		{
			if (deduplicate) {
				throw std::logic_error{
					"RankingFunctionAny::map cannot deduplicate std::any results"};
			}

			auto mapped = ranked_belief::map(
				rf,
				[&func](const T& value) -> std::any {
					return func(std::any{value});
				},
				false);
			return std::make_unique<Model<std::any>>(std::move(mapped));
		}

		[[nodiscard]] std::unique_ptr<Concept> map_with_rank(
			const std::function<std::pair<std::any, Rank>(const std::any&, Rank)>& func,
			bool deduplicate) const override
		{
			if (deduplicate) {
				throw std::logic_error{
					"RankingFunctionAny::map_with_rank cannot deduplicate std::any results"};
			}

			auto mapped = ranked_belief::map_with_rank(
				rf,
				[&func](const T& value, Rank rank) {
					return func(std::any{value}, rank);
				},
				false);
			return std::make_unique<Model<std::any>>(std::move(mapped));
		}

		[[nodiscard]] std::unique_ptr<Concept> map_with_index(
			const std::function<std::any(const std::any&, std::size_t)>& func,
			bool deduplicate) const override
		{
			if (deduplicate) {
				throw std::logic_error{
					"RankingFunctionAny::map_with_index cannot deduplicate std::any results"};
			}

			auto mapped = ranked_belief::map_with_index(
				rf,
				[&func](const T& value, std::size_t index) -> std::any {
					return func(std::any{value}, index);
				},
				false);
			return std::make_unique<Model<std::any>>(std::move(mapped));
		}

		[[nodiscard]] std::unique_ptr<Concept> filter(
			const std::function<bool(const std::any&)>& predicate,
			bool deduplicate) const override
		{
			auto filtered = ranked_belief::filter(
				rf,
				[&predicate](const T& value) { return predicate(std::any{value}); },
				deduplicate);
			return std::make_unique<Model<T>>(std::move(filtered));
		}

		[[nodiscard]] std::unique_ptr<Concept> take(std::size_t n) const override
		{
			auto taken = ranked_belief::take(rf, n);
			return std::make_unique<Model<T>>(std::move(taken));
		}

		[[nodiscard]] std::unique_ptr<Concept> take_while_rank(Rank max_rank) const override
		{
			auto taken = ranked_belief::take_while_rank(rf, max_rank);
			return std::make_unique<Model<T>>(std::move(taken));
		}

		[[nodiscard]] std::unique_ptr<Concept> merge(
			const Concept& other,
			bool deduplicate) const override
		{
			if (auto other_model = dynamic_cast<const Model<T>*>(&other)) {
				auto merged = ranked_belief::merge(rf, other_model->rf, deduplicate);
				return std::make_unique<Model<T>>(std::move(merged));
			}

			if (deduplicate) {
				throw std::logic_error{
					"RankingFunctionAny::merge cannot deduplicate heterogeneous results"};
			}

			auto merged_any = ranked_belief::merge(
				to_any_ranking(false),
				other.to_any_ranking(false),
				false);
			return std::make_unique<Model<std::any>>(std::move(merged_any));
		}

		[[nodiscard]] std::unique_ptr<Concept> merge_apply(
			const Concept& functions,
			bool deduplicate) const override
		{
			if (deduplicate) {
				throw std::logic_error{
					"RankingFunctionAny::merge_apply cannot deduplicate std::any results"};
			}

			using AnyFunction = std::function<RankingFunctionAny(const std::any&)>;

			if (auto func_model = dynamic_cast<const Model<AnyFunction>*>(&functions)) {
				auto result = ranked_belief::merge_apply(
					rf,
					[func_rf = func_model->rf](const T& value) {
						return ranked_belief::merge_apply(
							func_rf,
							[&value](const AnyFunction& fn) {
								return fn(std::any{value}).to_any_ranking(false);
							},
							false);
					},
					false);
				return std::make_unique<Model<std::any>>(std::move(result));
			}

			throw std::logic_error{
				"RankingFunctionAny::merge_apply requires functions of type std::function<RankingFunctionAny(const std::any&)>"};
		}

		[[nodiscard]] std::unique_ptr<Concept> observe(
			const std::function<bool(const std::any&)>& predicate,
			bool deduplicate) const override
		{
			auto observed = ranked_belief::observe(
				rf,
				[&predicate](const T& value) { return predicate(std::any{value}); },
				deduplicate);
			return std::make_unique<Model<T>>(std::move(observed));
		}

		[[nodiscard]] std::unique_ptr<Concept> observe_value(
			const std::any& value,
			bool deduplicate) const override
		{
			if constexpr (!detail::EqualityComparable<T>) {
				throw std::logic_error{
					"RankingFunctionAny::observe_value requires equality comparable stored type"};
			} else {
				const auto& typed = std::any_cast<const T&>(value);
				auto observed = ranked_belief::observe(rf, typed, deduplicate);
				return std::make_unique<Model<T>>(std::move(observed));
			}
		}

		[[nodiscard]] std::vector<std::pair<std::any, Rank>> take_n(std::size_t n) const override
		{
			auto values = ranked_belief::take_n(rf, n);
			std::vector<std::pair<std::any, Rank>> result;
			result.reserve(values.size());
			for (auto& [value, rank] : values) {
				result.emplace_back(std::any{value}, rank);
			}
			return result;
		}

		[[nodiscard]] RankingFunction<std::any> to_any_ranking(bool /*deduplicate*/) const override
		{
			return ranked_belief::map(
				rf,
				[](const T& value) -> std::any { return std::any{value}; },
				false);
		}

		RankingFunction<T> rf;
	};

	template<>
	struct RankingFunctionAny::Model<std::any> final : Concept {
		explicit Model(RankingFunction<std::any> ranking)
			: rf{std::move(ranking)} {}

		[[nodiscard]] std::unique_ptr<Concept> clone() const override
		{
			return std::make_unique<Model<std::any>>(rf);
		}

		[[nodiscard]] bool is_empty() const override
		{
			return rf.is_empty();
		}

		[[nodiscard]] std::any first_value() const override
		{
			auto first = rf.first();
			if (!first) {
				throw std::logic_error{"RankingFunctionAny::first_value called on empty ranking"};
			}
			return first->first;
		}

		[[nodiscard]] std::optional<Rank> first_rank() const override
		{
			auto first = rf.first();
			if (!first) {
				return std::nullopt;
			}
			return first->second;
		}

		[[nodiscard]] std::unique_ptr<Concept> map(
			const std::function<std::any(const std::any&)>& func,
			bool deduplicate) const override
		{
			if (deduplicate) {
				throw std::logic_error{
					"RankingFunctionAny::map cannot deduplicate std::any results"};
			}
			auto mapped = ranked_belief::map(
				rf,
				[&func](const std::any& value) { return func(value); },
				false);
			return std::make_unique<Model<std::any>>(std::move(mapped));
		}

		[[nodiscard]] std::unique_ptr<Concept> map_with_rank(
			const std::function<std::pair<std::any, Rank>(const std::any&, Rank)>& func,
			bool deduplicate) const override
		{
			if (deduplicate) {
				throw std::logic_error{
					"RankingFunctionAny::map_with_rank cannot deduplicate std::any results"};
			}
			auto mapped = ranked_belief::map_with_rank(
				rf,
				[&func](const std::any& value, Rank rank) {
					return func(value, rank);
				},
				false);
			return std::make_unique<Model<std::any>>(std::move(mapped));
		}

		[[nodiscard]] std::unique_ptr<Concept> map_with_index(
			const std::function<std::any(const std::any&, std::size_t)>& func,
			bool deduplicate) const override
		{
			if (deduplicate) {
				throw std::logic_error{
					"RankingFunctionAny::map_with_index cannot deduplicate std::any results"};
			}
			auto mapped = ranked_belief::map_with_index(
				rf,
				[&func](const std::any& value, std::size_t index) {
					return func(value, index);
				},
				false);
			return std::make_unique<Model<std::any>>(std::move(mapped));
		}

		[[nodiscard]] std::unique_ptr<Concept> filter(
			const std::function<bool(const std::any&)>& predicate,
			bool deduplicate) const override
		{
			if (deduplicate) {
				throw std::logic_error{
					"RankingFunctionAny::filter cannot deduplicate std::any results"};
			}
			auto filtered = ranked_belief::filter(
				rf,
				[&predicate](const std::any& value) { return predicate(value); },
				false);
			return std::make_unique<Model<std::any>>(std::move(filtered));
		}

		[[nodiscard]] std::unique_ptr<Concept> take(std::size_t n) const override
		{
			auto taken = ranked_belief::take(rf, n);
			return std::make_unique<Model<std::any>>(std::move(taken));
		}

		[[nodiscard]] std::unique_ptr<Concept> take_while_rank(Rank max_rank) const override
		{
			auto taken = ranked_belief::take_while_rank(rf, max_rank);
			return std::make_unique<Model<std::any>>(std::move(taken));
		}

		[[nodiscard]] std::unique_ptr<Concept> merge(
			const Concept& other,
			bool deduplicate) const override
		{
			if (deduplicate) {
				throw std::logic_error{
					"RankingFunctionAny::merge cannot deduplicate std::any results"};
			}
			auto merged = ranked_belief::merge(
				rf,
				other.to_any_ranking(false),
				false);
			return std::make_unique<Model<std::any>>(std::move(merged));
		}

		[[nodiscard]] std::unique_ptr<Concept> merge_apply(
			const Concept& /*functions*/,
			bool /*deduplicate*/) const override
		{
			throw std::logic_error{
				"RankingFunctionAny::merge_apply requires function rankings with concrete types"};
		}

		[[nodiscard]] std::unique_ptr<Concept> observe(
			const std::function<bool(const std::any&)>& predicate,
			bool deduplicate) const override
		{
			if (deduplicate) {
				throw std::logic_error{
					"RankingFunctionAny::observe cannot deduplicate std::any results"};
			}
			auto observed = ranked_belief::observe(
				rf,
				[&predicate](const std::any& value) { return predicate(value); },
				false);
			return std::make_unique<Model<std::any>>(std::move(observed));
		}

		[[nodiscard]] std::unique_ptr<Concept> observe_value(
			const std::any& /*value*/,
			bool /*deduplicate*/) const override
		{
			throw std::logic_error{
				"RankingFunctionAny::observe_value is unavailable for std::any payloads"};
		}

		[[nodiscard]] std::vector<std::pair<std::any, Rank>> take_n(std::size_t n) const override
		{
			return ranked_belief::take_n(rf, n);
		}

		[[nodiscard]] RankingFunction<std::any> to_any_ranking(bool /*deduplicate*/) const override
		{
			return rf;
		}

		RankingFunction<std::any> rf;
	};

	inline std::shared_ptr<const RankingFunctionAny::Concept> RankingFunctionAny::make_shared(
		std::unique_ptr<Concept> impl)
	{
		if (!impl) {
			return empty_impl();
		}
		return std::shared_ptr<const Concept>(std::move(impl));
	}

	inline std::shared_ptr<const RankingFunctionAny::Concept> RankingFunctionAny::empty_impl()
	{
		static const std::shared_ptr<const Concept> empty = [] {
			auto ptr = std::make_shared<Model<std::any>>(RankingFunction<std::any>());
			return std::static_pointer_cast<const Concept>(ptr);
		}();
		return empty;
	}

// -- Inline implementations -------------------------------------------------

inline RankingFunctionAny::RankingFunctionAny()
	: impl_{empty_impl()} {}

inline bool RankingFunctionAny::is_empty() const
{
	return impl_->is_empty();
}

inline std::any RankingFunctionAny::first_value() const
{
	return impl_->first_value();
}

inline std::optional<Rank> RankingFunctionAny::first_rank() const
{
	return impl_->first_rank();
}

inline RankingFunctionAny RankingFunctionAny::map(
	const std::function<std::any(const std::any&)>& func,
	bool deduplicate) const
{
	return RankingFunctionAny{make_shared(impl_->map(func, deduplicate))};
}

inline RankingFunctionAny RankingFunctionAny::map_with_rank(
	const std::function<std::pair<std::any, Rank>(const std::any&, Rank)>& func,
	bool deduplicate) const
{
	return RankingFunctionAny{make_shared(impl_->map_with_rank(func, deduplicate))};
}

inline RankingFunctionAny RankingFunctionAny::map_with_index(
	const std::function<std::any(const std::any&, std::size_t)>& func,
	bool deduplicate) const
{
	return RankingFunctionAny{make_shared(impl_->map_with_index(func, deduplicate))};
}

inline RankingFunctionAny RankingFunctionAny::filter(
	const std::function<bool(const std::any&)>& predicate,
	bool deduplicate) const
{
	return RankingFunctionAny{make_shared(impl_->filter(predicate, deduplicate))};
}

inline RankingFunctionAny RankingFunctionAny::take(std::size_t n) const
{
	return RankingFunctionAny{make_shared(impl_->take(n))};
}

inline RankingFunctionAny RankingFunctionAny::take_while_rank(Rank max_rank) const
{
	return RankingFunctionAny{make_shared(impl_->take_while_rank(max_rank))};
}

inline RankingFunctionAny RankingFunctionAny::merge(
	const RankingFunctionAny& other,
	bool deduplicate) const
{
	return RankingFunctionAny{make_shared(impl_->merge(*other.impl_, deduplicate))};
}

inline RankingFunctionAny RankingFunctionAny::merge_all(
	const std::vector<RankingFunctionAny>& rankings,
	bool deduplicate)
{
	if (rankings.empty()) {
		return RankingFunctionAny{};
	}

	if (deduplicate) {
		throw std::logic_error{
			"RankingFunctionAny::merge_all cannot deduplicate std::any results"};
	}

	std::vector<RankingFunction<std::any>> as_any;
	as_any.reserve(rankings.size());
	for (const auto& rf : rankings) {
		as_any.emplace_back(rf.to_any_ranking(false));
	}

	auto merged = ranked_belief::merge_all(as_any, false);
	return RankingFunctionAny{make_shared(std::make_unique<Model<std::any>>(std::move(merged)))};
}

inline RankingFunctionAny RankingFunctionAny::merge_apply(
	const RankingFunctionAny& functions,
	bool deduplicate) const
{
	return RankingFunctionAny{make_shared(impl_->merge_apply(*functions.impl_, deduplicate))};
}

inline RankingFunctionAny RankingFunctionAny::observe(
	const std::function<bool(const std::any&)>& predicate,
	bool deduplicate) const
{
	return RankingFunctionAny{make_shared(impl_->observe(predicate, deduplicate))};
}

inline RankingFunctionAny RankingFunctionAny::observe_value(
	const std::any& value,
	bool deduplicate) const
{
	return RankingFunctionAny{make_shared(impl_->observe_value(value, deduplicate))};
}

inline std::vector<std::pair<std::any, Rank>> RankingFunctionAny::take_n(std::size_t n) const
{
	return impl_->take_n(n);
}

inline RankingFunction<std::any> RankingFunctionAny::to_any_ranking(bool deduplicate) const
{
	return impl_->to_any_ranking(deduplicate);
}

} // namespace ranked_belief

