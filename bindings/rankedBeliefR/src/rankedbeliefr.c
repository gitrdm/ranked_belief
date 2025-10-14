#include <R.h>
#include <Rinternals.h>
#include <R_ext/Arith.h>
#include <R_ext/Rdynload.h>

#include "ranked_belief/c_api.h"

static const char *ranked_belief_class = "ranked_belief_ranking";

static const char *status_message(rb_status status) {
    switch (status) {
    case RB_STATUS_OK:
        return "ok";
    case RB_STATUS_INVALID_ARGUMENT:
        return "invalid argument";
    case RB_STATUS_ALLOCATION_FAILURE:
        return "allocation failure";
    case RB_STATUS_CALLBACK_ERROR:
        return "callback error";
    case RB_STATUS_INSUFFICIENT_BUFFER:
        return "insufficient buffer";
    case RB_STATUS_INTERNAL_ERROR:
        return "internal error";
    default:
        return "unknown status";
    }
}

static void raise_status_error(rb_status status, const char *fn_name) {
    if (status == RB_STATUS_OK) {
        return;
    }
    Rf_error("%s failed: %s", fn_name, status_message(status));
}

static void ranking_finalizer(SEXP ext) {
    rb_ranking_t *ptr = (rb_ranking_t *) R_ExternalPtrAddr(ext);
    if (ptr) {
        rb_ranking_free(ptr);
        R_ClearExternalPtr(ext);
    }
}

static SEXP make_ranking_external(rb_ranking_t *ptr) {
    if (!ptr) {
        Rf_error("Failed to allocate ranking handle");
    }
    SEXP ext = PROTECT(R_MakeExternalPtr(ptr, R_NilValue, R_NilValue));
    R_RegisterCFinalizerEx(ext, ranking_finalizer, TRUE);
    SEXP cls = PROTECT(allocVector(STRSXP, 1));
    SET_STRING_ELT(cls, 0, mkChar(ranked_belief_class));
    classgets(ext, cls);
    UNPROTECT(2);
    return ext;
}

static rb_ranking_t *expect_ranking(SEXP ext, int allow_null) {
    if (ext == R_NilValue) {
        if (allow_null) {
            return NULL;
        }
        Rf_error("Ranking handle cannot be NULL");
    }
    if (!Rf_inherits(ext, ranked_belief_class)) {
        Rf_error("Object is not a ranked_belief_ranking");
    }
    rb_ranking_t *ptr = (rb_ranking_t *) R_ExternalPtrAddr(ext);
    if (!ptr) {
        Rf_error("The ranking pointer has been released");
    }
    return ptr;
}

static void ensure_length(SEXP vector, R_xlen_t expected, const char *arg) {
    if (XLENGTH(vector) != expected) {
        Rf_error("%s must have length %lld", arg, (long long) expected);
    }
}

SEXP rankedbeliefr_singleton_int(SEXP value_sexp) {
    if (TYPEOF(value_sexp) != INTSXP || XLENGTH(value_sexp) != 1) {
        Rf_error("`value` must be an integer scalar");
    }
    int value = INTEGER(value_sexp)[0];
    rb_ranking_t *handle = NULL;
    rb_status status = rb_singleton_int(value, &handle);
    raise_status_error(status, "rb_singleton_int");
    return make_ranking_external(handle);
}

SEXP rankedbeliefr_from_array_int(SEXP values_sexp, SEXP ranks_sexp) {
    if (TYPEOF(values_sexp) != INTSXP) {
        Rf_error("`values` must be an integer vector");
    }
    const int *values = INTEGER(values_sexp);
    size_t count = (size_t) XLENGTH(values_sexp);
    const uint64_t *ranks_ptr = NULL;
    uint64_t *ranks_buffer = NULL;

    if (ranks_sexp != R_NilValue) {
        if (!(TYPEOF(ranks_sexp) == INTSXP || TYPEOF(ranks_sexp) == REALSXP)) {
            Rf_error("`ranks` must be integer, numeric, or NULL");
        }
        ensure_length(ranks_sexp, XLENGTH(values_sexp), "`ranks`");
        ranks_buffer = (uint64_t *) R_alloc((size_t) XLENGTH(ranks_sexp), sizeof(uint64_t));
        if (TYPEOF(ranks_sexp) == INTSXP) {
            const int *iranks = INTEGER(ranks_sexp);
            for (R_xlen_t i = 0; i < XLENGTH(ranks_sexp); ++i) {
                if (iranks[i] < 0) {
                    Rf_error("ranks must be non-negative");
                }
                ranks_buffer[i] = (uint64_t) iranks[i];
            }
        } else {
            const double *dranks = REAL(ranks_sexp);
            for (R_xlen_t i = 0; i < XLENGTH(ranks_sexp); ++i) {
                if (!R_finite(dranks[i]) || dranks[i] < 0) {
                    Rf_error("ranks must be finite and non-negative");
                }
                ranks_buffer[i] = (uint64_t) dranks[i];
            }
        }
        ranks_ptr = ranks_buffer;
    }

    rb_ranking_t *handle = NULL;
    rb_status status = rb_from_array_int(values, ranks_ptr, count, &handle);
    raise_status_error(status, "rb_from_array_int");
    return make_ranking_external(handle);
}

SEXP rankedbeliefr_take_n_int(SEXP ranking_sexp, SEXP n_sexp) {
    rb_ranking_t *ranking = expect_ranking(ranking_sexp, 0);
    if (TYPEOF(n_sexp) != INTSXP || XLENGTH(n_sexp) != 1) {
        Rf_error("`n` must be an integer scalar");
    }
    int n = INTEGER(n_sexp)[0];
    if (n < 0) {
        Rf_error("`n` must be non-negative");
    }
    size_t request = (size_t) n;
    size_t extracted = 0;
    int *values_buf = NULL;
    uint64_t *ranks_buf = NULL;
    if (request > 0) {
        values_buf = (int *) R_alloc(request, sizeof(int));
        ranks_buf = (uint64_t *) R_alloc(request, sizeof(uint64_t));
        rb_status status = rb_take_n_int(ranking, request, values_buf, ranks_buf, request, &extracted);
        raise_status_error(status, "rb_take_n_int");
    }

    SEXP values_vec = PROTECT(allocVector(INTSXP, (R_xlen_t) extracted));
    SEXP ranks_vec = PROTECT(allocVector(REALSXP, (R_xlen_t) extracted));
    for (size_t i = 0; i < extracted; ++i) {
        INTEGER(values_vec)[i] = values_buf[i];
        REAL(ranks_vec)[i] = (double) ranks_buf[i];
    }

    SEXP df = PROTECT(allocVector(VECSXP, 2));
    SET_VECTOR_ELT(df, 0, values_vec);
    SET_VECTOR_ELT(df, 1, ranks_vec);

    SEXP names = PROTECT(allocVector(STRSXP, 2));
    SET_STRING_ELT(names, 0, mkChar("value"));
    SET_STRING_ELT(names, 1, mkChar("rank"));
    setAttrib(df, R_NamesSymbol, names);

    SEXP rownames = PROTECT(allocVector(INTSXP, 2));
    INTEGER(rownames)[0] = NA_INTEGER;
    INTEGER(rownames)[1] = -((int) extracted);
    setAttrib(df, R_RowNamesSymbol, rownames);

    SEXP cls = PROTECT(allocVector(STRSXP, 1));
    SET_STRING_ELT(cls, 0, mkChar("data.frame"));
    setAttrib(df, R_ClassSymbol, cls);

    UNPROTECT(6);
    return df;
}

SEXP rankedbeliefr_first_int(SEXP ranking_sexp) {
    rb_ranking_t *ranking = expect_ranking(ranking_sexp, 0);
    int value = 0;
    uint64_t rank = 0;
    int has_value = 0;
    rb_status status = rb_first_int(ranking, &value, &rank, &has_value);
    raise_status_error(status, "rb_first_int");
    if (!has_value) {
        return R_NilValue;
    }
    SEXP out = PROTECT(allocVector(VECSXP, 2));
    SET_VECTOR_ELT(out, 0, ScalarInteger(value));
    SET_VECTOR_ELT(out, 1, ScalarReal((double) rank));
    SEXP names = PROTECT(allocVector(STRSXP, 2));
    SET_STRING_ELT(names, 0, mkChar("value"));
    SET_STRING_ELT(names, 1, mkChar("rank"));
    setAttrib(out, R_NamesSymbol, names);
    UNPROTECT(2);
    return out;
}

SEXP rankedbeliefr_is_empty(SEXP ranking_sexp) {
    rb_ranking_t *ranking = expect_ranking(ranking_sexp, 0);
    int is_empty = 0;
    rb_status status = rb_is_empty(ranking, &is_empty);
    raise_status_error(status, "rb_is_empty");
    return ScalarLogical(is_empty != 0);
}

SEXP rankedbeliefr_merge_int(SEXP lhs_sexp, SEXP rhs_sexp) {
    rb_ranking_t *lhs = expect_ranking(lhs_sexp, 1);
    rb_ranking_t *rhs = expect_ranking(rhs_sexp, 1);
    rb_ranking_t *handle = NULL;
    rb_status status = rb_merge_int(lhs, rhs, &handle);
    raise_status_error(status, "rb_merge_int");
    return make_ranking_external(handle);
}

SEXP rankedbeliefr_observe_value_int(SEXP ranking_sexp, SEXP value_sexp) {
    rb_ranking_t *ranking = expect_ranking(ranking_sexp, 0);
    if (TYPEOF(value_sexp) != INTSXP || XLENGTH(value_sexp) != 1) {
        Rf_error("`value` must be an integer scalar");
    }
    int value = INTEGER(value_sexp)[0];
    rb_ranking_t *handle = NULL;
    rb_status status = rb_observe_value_int(ranking, value, &handle);
    raise_status_error(status, "rb_observe_value_int");
    return make_ranking_external(handle);
}

SEXP rankedbeliefr_free(SEXP ranking_sexp) {
    if (ranking_sexp == R_NilValue) {
        return R_NilValue;
    }
    if (!Rf_inherits(ranking_sexp, ranked_belief_class)) {
        Rf_error("Object is not a ranked_belief_ranking");
    }
    rb_ranking_t *ptr = (rb_ranking_t *) R_ExternalPtrAddr(ranking_sexp);
    if (ptr) {
        rb_ranking_free(ptr);
        R_ClearExternalPtr(ranking_sexp);
    }
    return R_NilValue;
}
