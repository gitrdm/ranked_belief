#include <R.h>
#include <Rinternals.h>
#include <R_ext/Rdynload.h>

SEXP rankedbeliefr_singleton_int(SEXP value_sexp);
SEXP rankedbeliefr_from_array_int(SEXP values_sexp, SEXP ranks_sexp);
SEXP rankedbeliefr_take_n_int(SEXP ranking_sexp, SEXP n_sexp);
SEXP rankedbeliefr_first_int(SEXP ranking_sexp);
SEXP rankedbeliefr_is_empty(SEXP ranking_sexp);
SEXP rankedbeliefr_merge_int(SEXP lhs_sexp, SEXP rhs_sexp);
SEXP rankedbeliefr_observe_value_int(SEXP ranking_sexp, SEXP value_sexp);
SEXP rankedbeliefr_free(SEXP ranking_sexp);

static const R_CallMethodDef CallEntries[] = {
    {"rankedbeliefr_singleton_int", (DL_FUNC) &rankedbeliefr_singleton_int, 1},
    {"rankedbeliefr_from_array_int", (DL_FUNC) &rankedbeliefr_from_array_int, 2},
    {"rankedbeliefr_take_n_int", (DL_FUNC) &rankedbeliefr_take_n_int, 2},
    {"rankedbeliefr_first_int", (DL_FUNC) &rankedbeliefr_first_int, 1},
    {"rankedbeliefr_is_empty", (DL_FUNC) &rankedbeliefr_is_empty, 1},
    {"rankedbeliefr_merge_int", (DL_FUNC) &rankedbeliefr_merge_int, 2},
    {"rankedbeliefr_observe_value_int", (DL_FUNC) &rankedbeliefr_observe_value_int, 2},
    {"rankedbeliefr_free", (DL_FUNC) &rankedbeliefr_free, 1},
    {NULL, NULL, 0}
};

void R_init_rankedBeliefR(DllInfo *dll) {
    R_registerRoutines(dll, NULL, CallEntries, NULL, NULL);
    R_useDynamicSymbols(dll, FALSE);
}
