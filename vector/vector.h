#ifndef VECTOR_H
#define VECTOR_H



#define VEC_TYPE_NULL 0

#include <stddef.h>

#define MIN(n1, n2) (n1 < n2 ? n1 : n2)
#define MAX(n1, n2) (n1 > n2 ? n1 : n2)

#ifdef INCLUDE_HVEC

typedef enum HVEC_TYPES {
    HVEC_TYPE_NORMAL = 1,
    HVEC_TYPE_STRUCT,
    HVEC_TYPE_HVECTOR,
    HVEC_TYPE_SVECTOR
} HVEC_TYPES;

typedef struct HVector {
    void     **arr;
    size_t     len;
    size_t     max;
    HVEC_TYPES type;
    void     (*freestrct)(void *);
} HVector;

HVector *crthvec(size_t size, HVEC_TYPES type, void (*freefnc)(void *));

int     freehvec(HVector *hvec);
int hvecrecalloc(HVector *hvec);
int  hvecfreeind(HVector *hvec, size_t ind);

int    hvecswap(HVector *hvec, size_t ind, void *item);
int   hvecinsrt(HVector *hvec, size_t ind, void *item);
int hvecinsrtsf(HVector *hvec, size_t ind, void *item);
int    hvecpush(HVector *hvec,             void *item);

int hvecrmv(HVector *hvec, size_t ind);
int hvecpop(HVector *hvec);
 
#endif



#ifdef INCLUDE_SVEC

typedef enum SVEC_TYPES {
    SVEC_TYPE_NORMAL = 1,
    SVEC_TYPE_STRUCT
} SVEC_TYPES;

typedef struct SVector {
    void      *arr;
    size_t     len;
    size_t     max;
    SVEC_TYPES type;
    size_t     elemSize;
    void     (*freestrct)(void *);
} SVector;

SVector *crtsvec(size_t size, size_t elemSize, SVEC_TYPES type, void (*freefnc)(void *));

int     freesvec(SVector *svec);
int svecrecalloc(SVector *svec);
int  svecfreeind(SVector *svec, size_t ind);

int    svecswap(SVector *hvec, size_t ind, void *item);
int   svecinsrt(SVector *hvec, size_t ind, void *item);
int svecinsrtsf(SVector *hvec, size_t ind, void *item);
int    svecpush(SVector *hvec,             void *item);

int svecrmv(SVector *hvec, size_t ind);
int svecpop(SVector *hvec);

#endif



#endif