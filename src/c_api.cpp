#include "ranked_belief/c_api.h"

#include "ranked_belief/constructors.hpp"
#include "ranked_belief/operations/filter.hpp"
#include "ranked_belief/operations/map.hpp"
#include "ranked_belief/operations/merge.hpp"
#include "ranked_belief/operations/nrm_exc.hpp"
#include "ranked_belief/operations/observe.hpp"

#include <exception>
#include <limits>
#include <memory>
#include <new>
#include <stdexcept>
#include <utility>
#include <vector>

struct rb_ranking_t {
    std::shared_ptr<ranked_belief::RankingFunction<int>> ranking;
};

namespace {

struct callback_error final : std::exception {
    explicit callback_error(rb_status status) noexcept
        : status(status) {}

    [[nodiscard]] const char *what() const noexcept override
    {
        return "rb_callback_error";
    }

    rb_status status;
};

[[nodiscard]] std::unique_ptr<rb_ranking_t> make_handle(ranked_belief::RankingFunction<int> rf)
{
    auto handle = std::make_unique<rb_ranking_t>();
    handle->ranking = std::make_shared<ranked_belief::RankingFunction<int>>(std::move(rf));
    return handle;
}

[[nodiscard]] const ranked_belief::RankingFunction<int> *unwrap(const rb_ranking_t *handle)
{
    return handle && handle->ranking ? handle->ranking.get() : nullptr;
}

[[nodiscard]] ranked_belief::RankingFunction<int> unwrap_or_empty(const rb_ranking_t *handle)
{
    if (handle && handle->ranking) {
        return *handle->ranking;
    }
    return ranked_belief::RankingFunction<int>();
}

[[nodiscard]] constexpr ranked_belief::Deduplication to_deduplication(bool deduplicate) noexcept
{
    return deduplicate ? ranked_belief::Deduplication::Enabled : ranked_belief::Deduplication::Disabled;
}

[[nodiscard]] rb_status translate_exception(const std::bad_alloc &)
{
    return RB_STATUS_ALLOCATION_FAILURE;
}

[[nodiscard]] rb_status translate_exception(const callback_error &err)
{
    return err.status == RB_STATUS_OK ? RB_STATUS_CALLBACK_ERROR : err.status;
}

[[nodiscard]] rb_status translate_exception(const std::logic_error &)
{
    return RB_STATUS_INVALID_ARGUMENT;
}

[[nodiscard]] rb_status translate_exception(const std::exception &)
{
    return RB_STATUS_INTERNAL_ERROR;
}

} // namespace

rb_status rb_singleton_int(int value, rb_ranking_t **out_ranking)
{
    if (!out_ranking) {
        return RB_STATUS_INVALID_ARGUMENT;
    }
    *out_ranking = nullptr;

    try {
        auto ranking = ranked_belief::singleton<int>(value, ranked_belief::Rank::zero());
        auto handle = make_handle(std::move(ranking));
        *out_ranking = handle.release();
        return RB_STATUS_OK;
    } catch (const std::bad_alloc &ex) {
        return translate_exception(ex);
    } catch (const std::logic_error &ex) {
        return translate_exception(ex);
    } catch (const std::exception &ex) {
        return translate_exception(ex);
    }
}

rb_status rb_from_array_int(const int *values, const uint64_t *ranks, size_t count, rb_ranking_t **out_ranking)
{
    if (!out_ranking) {
        return RB_STATUS_INVALID_ARGUMENT;
    }
    *out_ranking = nullptr;

    if (count == 0) {
        try {
            auto handle = make_handle(ranked_belief::RankingFunction<int>());
            *out_ranking = handle.release();
            return RB_STATUS_OK;
        } catch (const std::bad_alloc &ex) {
            return translate_exception(ex);
        } catch (const std::logic_error &ex) {
            return translate_exception(ex);
        } catch (const std::exception &ex) {
            return translate_exception(ex);
        }
    }

    if (!values) {
        return RB_STATUS_INVALID_ARGUMENT;
    }

    try {
        std::vector<std::pair<int, ranked_belief::Rank>> pairs;
        pairs.reserve(count);
        for (size_t idx = 0; idx < count; ++idx) {
            const auto value = values[idx];
            ranked_belief::Rank rank = ranks ? ranked_belief::Rank::from_value(ranks[idx])
                                             : ranked_belief::Rank::from_value(static_cast<uint64_t>(idx));
            pairs.emplace_back(value, rank);
        }

    auto ranking = ranked_belief::from_list(pairs, ranked_belief::Deduplication::Enabled);
        auto handle = make_handle(std::move(ranking));
        *out_ranking = handle.release();
        return RB_STATUS_OK;
    } catch (const std::bad_alloc &ex) {
        return translate_exception(ex);
    } catch (const std::logic_error &ex) {
        return translate_exception(ex);
    } catch (const std::exception &ex) {
        return translate_exception(ex);
    }
}

rb_status rb_map_int(const rb_ranking_t *ranking, rb_map_callback_t callback, void *context, rb_ranking_t **out_ranking)
{
    if (!out_ranking || !callback) {
        return RB_STATUS_INVALID_ARGUMENT;
    }
    *out_ranking = nullptr;

    const auto *source = unwrap(ranking);
    if (!source) {
        return RB_STATUS_INVALID_ARGUMENT;
    }

    try {
        auto mapped = ranked_belief::map(
            *source,
            [callback, context](const int &value) {
                int output = 0;
                const rb_status status = callback(value, context, &output);
                if (status != RB_STATUS_OK) {
                    throw callback_error(status);
                }
                return output;
            },
            to_deduplication(source->is_deduplicating()));

        auto handle = make_handle(std::move(mapped));
        *out_ranking = handle.release();
        return RB_STATUS_OK;
    } catch (const callback_error &ex) {
        return translate_exception(ex);
    } catch (const std::bad_alloc &ex) {
        return translate_exception(ex);
    } catch (const std::logic_error &ex) {
        return translate_exception(ex);
    } catch (const std::exception &ex) {
        return translate_exception(ex);
    }
}

rb_status rb_filter_int(const rb_ranking_t *ranking, rb_filter_callback_t callback, void *context, rb_ranking_t **out_ranking)
{
    if (!out_ranking || !callback) {
        return RB_STATUS_INVALID_ARGUMENT;
    }
    *out_ranking = nullptr;

    const auto *source = unwrap(ranking);
    if (!source) {
        return RB_STATUS_INVALID_ARGUMENT;
    }

    try {
        auto filtered = ranked_belief::filter(
            *source,
            [callback, context](const int &value) {
                int keep = 0;
                const rb_status status = callback(value, context, &keep);
                if (status != RB_STATUS_OK) {
                    throw callback_error(status);
                }
                return keep != 0;
            },
            to_deduplication(source->is_deduplicating()));

        auto handle = make_handle(std::move(filtered));
        *out_ranking = handle.release();
        return RB_STATUS_OK;
    } catch (const callback_error &ex) {
        return translate_exception(ex);
    } catch (const std::bad_alloc &ex) {
        return translate_exception(ex);
    } catch (const std::logic_error &ex) {
        return translate_exception(ex);
    } catch (const std::exception &ex) {
        return translate_exception(ex);
    }
}

rb_status rb_merge_int(const rb_ranking_t *lhs, const rb_ranking_t *rhs, rb_ranking_t **out_ranking)
{
    if (!out_ranking) {
        return RB_STATUS_INVALID_ARGUMENT;
    }
    *out_ranking = nullptr;

    try {
        auto lhs_copy = unwrap_or_empty(lhs);
        auto rhs_copy = unwrap_or_empty(rhs);
        bool deduplicate = true;
        if (lhs && lhs->ranking && !lhs->ranking->is_deduplicating()) {
            deduplicate = false;
        }
        if (rhs && rhs->ranking && !rhs->ranking->is_deduplicating()) {
            deduplicate = false;
        }

        auto merged = ranked_belief::merge(lhs_copy, rhs_copy, to_deduplication(deduplicate));
        auto handle = make_handle(std::move(merged));
        *out_ranking = handle.release();
        return RB_STATUS_OK;
    } catch (const std::bad_alloc &ex) {
        return translate_exception(ex);
    } catch (const std::logic_error &ex) {
        return translate_exception(ex);
    } catch (const std::exception &ex) {
        return translate_exception(ex);
    }
}

rb_status rb_observe_value_int(const rb_ranking_t *ranking, int value, rb_ranking_t **out_ranking)
{
    if (!out_ranking) {
        return RB_STATUS_INVALID_ARGUMENT;
    }
    *out_ranking = nullptr;

    const auto *source = unwrap(ranking);
    if (!source) {
        return RB_STATUS_INVALID_ARGUMENT;
    }

    try {
    auto observed = ranked_belief::observe(*source, value, to_deduplication(source->is_deduplicating()));
        auto handle = make_handle(std::move(observed));
        *out_ranking = handle.release();
        return RB_STATUS_OK;
    } catch (const std::bad_alloc &ex) {
        return translate_exception(ex);
    } catch (const std::logic_error &ex) {
        return translate_exception(ex);
    } catch (const std::exception &ex) {
        return translate_exception(ex);
    }
}

rb_status rb_is_empty(const rb_ranking_t *ranking, int *out_is_empty)
{
    if (!ranking || !ranking->ranking || !out_is_empty) {
        return RB_STATUS_INVALID_ARGUMENT;
    }

    *out_is_empty = ranking->ranking->is_empty() ? 1 : 0;
    return RB_STATUS_OK;
}

rb_status rb_first_int(const rb_ranking_t *ranking, int *out_value, uint64_t *out_rank, int *out_has_value)
{
    if (!ranking || !ranking->ranking || !out_value || !out_rank || !out_has_value) {
        return RB_STATUS_INVALID_ARGUMENT;
    }

    try {
        auto first = ranking->ranking->first();
        if (!first.has_value()) {
            *out_has_value = 0;
            *out_rank = 0;
            return RB_STATUS_OK;
        }

        *out_has_value = 1;
        *out_value = first->first;
        *out_rank = first->second.is_infinity()
            ? std::numeric_limits<uint64_t>::max()
            : first->second.value();
        return RB_STATUS_OK;
    } catch (const callback_error &ex) {
        *out_has_value = 0;
        return translate_exception(ex);
    } catch (const std::bad_alloc &ex) {
        *out_has_value = 0;
        return translate_exception(ex);
    } catch (const std::logic_error &ex) {
        *out_has_value = 0;
        return translate_exception(ex);
    } catch (const std::exception &ex) {
        *out_has_value = 0;
        return translate_exception(ex);
    }
}

rb_status rb_take_n_int(const rb_ranking_t *ranking,
                        size_t n,
                        int *out_values,
                        uint64_t *out_ranks,
                        size_t buffer_size,
                        size_t *out_count)
{
    if (!ranking || !ranking->ranking || !out_values || !out_ranks || !out_count) {
        return RB_STATUS_INVALID_ARGUMENT;
    }

    *out_count = 0;

    if (buffer_size < n) {
        return RB_STATUS_INSUFFICIENT_BUFFER;
    }

    try {
        auto pairs = ranked_belief::take_n(*ranking->ranking, n);
        const size_t count = pairs.size();
        for (size_t idx = 0; idx < count; ++idx) {
            out_values[idx] = pairs[idx].first;
            out_ranks[idx] = pairs[idx].second.is_infinity()
                ? std::numeric_limits<uint64_t>::max()
                : pairs[idx].second.value();
        }
    *out_count = count;
    return RB_STATUS_OK;
    } catch (const callback_error &ex) {
        *out_count = 0;
        return translate_exception(ex);
    } catch (const std::bad_alloc &ex) {
        *out_count = 0;
        return translate_exception(ex);
    } catch (const std::logic_error &ex) {
        *out_count = 0;
        return translate_exception(ex);
    } catch (const std::exception &ex) {
        *out_count = 0;
        return translate_exception(ex);
    }
}

void rb_ranking_free(rb_ranking_t *ranking)
{
    delete ranking;
}
