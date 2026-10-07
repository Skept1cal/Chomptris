#include "vector.h"
#include <stdlib.h>
#include <string.h>



#define MIN_VECTOR_SIZE 2



#ifdef INCLUDE_HVEC

#define HVEC_TYPES_ARR_BEGIN HVEC_TYPE_NORMAL
#define HVEC_TYPES_ARR_END   HVEC_TYPE_SVECTOR
const HVEC_TYPES HVEC_TYPES_ARR[] = {
    [HVEC_TYPE_NORMAL]  = HVEC_TYPE_NORMAL,
    [HVEC_TYPE_STRUCT]  = HVEC_TYPE_STRUCT,
    [HVEC_TYPE_HVECTOR] = HVEC_TYPE_HVECTOR,
    [HVEC_TYPE_SVECTOR] = HVEC_TYPE_SVECTOR
};



HVector *crthvec(size_t size, HVEC_TYPES type, void (*freefnc)(void *)) {
    if (
        !type                                  ||
        type < HVEC_TYPES_ARR_BEGIN            ||
        type > HVEC_TYPES_ARR_END              ||
        (type == HVEC_TYPE_STRUCT && !freefnc)
    ) return NULL;

    HVector* hvec = malloc(sizeof(HVector));
    if (!hvec) return NULL;

    hvec->arr = calloc(MAX(size, MIN_VECTOR_SIZE), sizeof(void*));
    if (!hvec->arr) {
        free(hvec);
        return NULL;
    }

    hvec->max       = MAX(size, MIN_VECTOR_SIZE);
    hvec->len       = 0;
    hvec->type      = type;
    hvec->freestrct = freefnc;

    return hvec;
}



int freehvec(HVector *hvec) {
    if (!hvec) return 1;

    if (hvec->arr) {
        switch (hvec->type) {
            case HVEC_TYPE_STRUCT: {
                for (int i = 0; i < hvec->len; i++) {
                    hvec->freestrct(hvec->arr[i]);
                    hvec->arr[i] = NULL;
                }
                break;
            }
            case HVEC_TYPE_HVECTOR: {
                for (int i = 0; i < hvec->len; i++) {
                    freehvec(hvec->arr[i]);
                    hvec->arr[i] = NULL;
                }
                break;
            }
            case HVEC_TYPE_SVECTOR: {
                #ifndef INCLUDE_SVEC
                return 1;
                #else
                for (int i = 0; i < hvec->len; i++) {
                    freesvec(hvec->arr[i]);
                    hvec->arr[i] = NULL;
                }
                break;
                #endif
            }
            default: {
                for (int i = 0; i < hvec->len; i++) {
                    free(hvec->arr[i]);
                    hvec->arr[i] = NULL;
                }
            }
        }

        free(hvec->arr);
        hvec->arr = NULL;
    }

    free(hvec);

    return 0;
}

int hvecrecalloc(HVector *hvec) {
    if (!hvec || !hvec->arr) return 1;

    void** temp = realloc(hvec->arr, hvec->max * 2 * sizeof(void*));
    if (!temp) return 1;

    hvec->arr  = temp;
    hvec->max *= 2;

    memset(hvec->arr + (hvec->max - hvec->len), 0, (hvec->max - hvec->len) * sizeof(void*));

    return 0;
}

int hvecfreeind(HVector *hvec, size_t ind) {
    if (!hvec || !hvec->arr || ind >= hvec->len || !hvec->arr[ind]) return 1;

    switch (hvec->type) {
        case HVEC_TYPE_STRUCT: hvec->freestrct(hvec->arr[ind]); break;
        case HVEC_TYPE_HVECTOR:       freehvec(hvec->arr[ind]); break;
        case HVEC_TYPE_SVECTOR: {
            #ifndef INCLUDE_SVEC
            return 1;
            #else
            freesvec(hvec->arr[ind]);
            break;
            #endif
        }
        default:                          free(hvec->arr[ind]); break;
    }
    hvec->arr[ind] = NULL;

    return 0;
}



int hvecswap(HVector *hvec, size_t ind, void *item) {
    if (!hvec || !hvec->arr || ind >= hvec->len) return 1;

    hvecfreeind(hvec, ind);
    hvec->arr[ind] = item;

    return 0;
}

int hvecinsrt(HVector *hvec, size_t ind, void *item) {
    if (!hvec || !hvec->arr || ind > hvec->len) return 1;

    if (hvec->len == hvec->max) {
        if (hvecrecalloc(hvec) != 0) return 1;
    }

    if (ind < hvec->len) memmove(hvec->arr + ind + 1, hvec->arr + ind, (hvec->len - ind) * sizeof(void*));
    hvec->arr[ind] = item;

    hvec->len++;

    return 0;
}

int hvecinsrtsf(HVector *hvec, size_t ind, void *item) {
    if (!hvec || !hvec->arr || ind > hvec->len) return 1;

    if (ind < hvec->len) return hvecinsrt(hvec, ind, item);
    else                 return  hvecpush(hvec, item);

    return 0;
}

int hvecpush(HVector *hvec, void *item) {
    if (!hvec || !hvec->arr) return 1;

    if (hvec->len == hvec->max) {
        if (hvecrecalloc(hvec) != 0) return 1;
    }

    hvec->arr[hvec->len++] = item;

    return 0;
}



int hvecrmv(HVector *hvec, size_t ind) {
    if (!hvec || !hvec->arr || ind >= hvec->len || hvec->len == 0) return 1;

    hvecfreeind(hvec, ind);

    // Must subtract an extra 1 from the number of elements to be moved,
    // as otherwise we'd count the removed element as well.
    memmove(hvec->arr + ind, hvec->arr + ind + 1, (hvec->len - ind - 1) * sizeof(void*));
    hvec->arr[--hvec->len] = NULL;

    return 0;
}

int hvecpop(HVector *hvec) {
    if (!hvec || !hvec->arr || hvec->len == 0) return 1;

    if (hvecfreeind(hvec, hvec->len - 1) != 0) return 1;
    hvec->len--;

    return 0;
}

#endif



#ifdef INCLUDE_SVEC

#define SVEC_TYPES_ARR_BEGIN SVEC_TYPE_NORMAL
#define SVEC_TYPES_ARR_END   SVEC_TYPE_STRUCT
const SVEC_TYPES SVEC_TYPES_ARR[] = {
    [SVEC_TYPE_NORMAL] = SVEC_TYPE_NORMAL,
    [SVEC_TYPE_STRUCT] = SVEC_TYPE_STRUCT,
};

#define SVEC_IND_START(svec, ind) ( ((char*)svec->arr) + (ind) * svec->elemSize )



SVector *crtsvec(size_t size, size_t elemSize, SVEC_TYPES type, void (*freefnc)(void *)) {
    if (
        !type                                  ||
        type < SVEC_TYPES_ARR_BEGIN            ||
        type > SVEC_TYPES_ARR_END              ||
        (type == SVEC_TYPE_STRUCT && !freefnc) ||
        elemSize == 0
    ) return NULL;

    SVector* svec = malloc(sizeof(SVector));
    if (!svec) return NULL;

    svec->arr = calloc(MAX(size, MIN_VECTOR_SIZE), elemSize);
    if (!svec->arr) {
        free(svec);
        return NULL;
    }

    svec->elemSize  = elemSize;
    svec->max       = MAX(size, MIN_VECTOR_SIZE);
    svec->len       = 0;
    svec->type      = type;
    svec->freestrct = freefnc;

    return svec;
}



int freesvec(SVector *svec) {
    if (!svec) return 1;

    if (svec->arr) {
        if (svec->type == SVEC_TYPE_STRUCT) {
            for (int i = 0; i < svec->len; i++) {
                svec->freestrct(SVEC_IND_START(svec, i));
            }
        }

        free(svec->arr);
        svec->arr = NULL;
    }

    free(svec);

    return 0;
}

int svecrecalloc(SVector *svec) {
    if (!svec || !svec->arr) return 1;

    void* temp = realloc(svec->arr, svec->max * 2 * svec->elemSize);
    if (!temp) return 1;

    svec->arr  = temp;
    svec->max *= 2;

    memset(SVEC_IND_START(svec, svec->max - svec->len), 0, (svec->max - svec->len) * svec->elemSize);

    return 0;
}

int svecfreeind(SVector *svec, size_t ind) {
    if (!svec || !svec->arr || ind >= svec->len) return 1;

    if (svec->type == SVEC_TYPE_STRUCT) {
        svec->freestrct(SVEC_IND_START(svec, ind));
        memset(SVEC_IND_START(svec, ind), 0, svec->elemSize);
    }

    return 0;
}



int svecswap(SVector *svec, size_t ind, void *item) {
    if (!svec || !svec->arr || ind >= svec->len || !item) return 1;

    svecfreeind(svec, ind);
    memcpy(SVEC_IND_START(svec, ind), item, svec->elemSize);

    return 0;
}

int svecinsrt(SVector *svec, size_t ind, void *item) {
    if (!svec || !svec->arr || ind > svec->len || !item) return 1;

    if (svec->len == svec->max) {
        if (svecrecalloc(svec) != 0) return 1;
    }

    if (ind < svec->len) memmove(SVEC_IND_START(svec, ind + 1), SVEC_IND_START(svec, ind), (svec->len - ind) * svec->elemSize);
    memcpy(SVEC_IND_START(svec, ind), item, svec->elemSize);

    svec->len++;

    return 0;
}

int svecinsrtsf(SVector *svec, size_t ind, void *item) {
    if (!svec || !svec->arr || ind > svec->len || !item) return 1;

    if (ind < svec->len) return svecinsrt(svec, ind, item);
    else                 return  svecpush(svec, item);

    return 0;
}

int svecpush(SVector *svec, void *item) {
    if (!svec || !svec->arr || !item) return 1;

    if (svec->len == svec->max) {
        if (svecrecalloc(svec) != 0) return 1;
    }

    memcpy(SVEC_IND_START(svec, svec->len++), item, svec->elemSize);

    return 0;
}



int svecrmv(SVector *svec, size_t ind) {
    if (!svec || !svec->arr || ind >= svec->len || svec->len == 0) return 1;

    svecfreeind(svec, ind);

    // Must subtract an extra 1 from the number of elements to be moved,
    // as otherwise we'd count the removed element as well.
    memmove(SVEC_IND_START(svec, ind), SVEC_IND_START(svec, ind + 1), (svec->len - ind - 1) * svec->elemSize);
    memset(SVEC_IND_START(svec, --svec->len), 0, svec->elemSize);

    return 0;
}

int svecpop(SVector *svec) {
    if (!svec || !svec->arr || svec->len == 0) return 1;

    if (svecfreeind(svec, svec->len - 1) != 0) return 1;
    svec->len--;

    return 0;
}

#endif