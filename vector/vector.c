#include <stdlib.h>
#include <string.h>

#include "vector.h"

//#include <stdio.h>



/*static void logvec(Vector* vec) {
    printf("\nVector:");
    for (int i = 0; i < vec->max; i++) {
        printf("\nindex %i: 0x%x", i, vec->arr[i]);
        if (vec->arr[i]) printf("; %lf", *(double*)vec->arr[i]);
        else printf("; NULL");
    }
    printf("\n");
}*/



#define DTYPES_BEGIN (DTYPE_NORMAL)
#define DTYPES_END   (DTYPE_STRUCT + 1)
static const DTYPE_T DTYPES[] = {[DTYPES_BEGIN]=DTYPE_NORMAL, DTYPE_VECTOR, DTYPE_STRUCT};

Vector* crtvec(size_t size, unsigned char type, void (*freefnc)(void*)) {
    int validmtype = 0;
    if (type) {
        for (int i = DTYPES_BEGIN; i < DTYPES_END; i++) {
            if (type == DTYPES[i]) {
                validmtype = 1;
                break;
            }
        }
    }
    if (!validmtype) return NULL;

    Vector* vec = malloc(sizeof(Vector));
    if (!vec) return NULL;

    size_t allocated = (size > 0) ? size : 2;

    vec->arr = calloc(allocated, sizeof(void*));
    if (!vec->arr) {
        free(vec);
        return NULL;
    }

    vec->max = allocated;
    vec->len = 0;

    vec->mtype = type;
    vec->stype = TYPE_NULL;

    vec->freestrct = freefnc;

    return vec;
}

Vector* crtvecalt(size_t size, unsigned char mtype, unsigned char stype, void (*freefnc)(void*)) {
    Vector* vec = crtvec(size, mtype, freefnc);
    if (!vec) return NULL;
    vec->stype = stype;
    return vec;
}



void freevec(Vector* vec) {
    if (!vec || !vec->arr) return;

    switch (vec->mtype) {
        case DTYPE_STRUCT: for (int i = 0; i < vec->len; i++) vec->freestrct(vec->arr[i]); break;
        case DTYPE_VECTOR: for (int i = 0; i < vec->len; i++)        freevec(vec->arr[i]); break;
                  default: for (int i = 0; i < vec->len; i++)           free(vec->arr[i]);
    }

    free(vec->arr);
    free(vec);
}

void* vecrecalloc(Vector* vec) {
    void** temp = realloc(vec->arr, vec->max * 2 * sizeof(void*));
    if (!temp) return NULL;

    vec->max *= 2;
    memset(temp + vec->len, 0, (vec->max - vec->len) * sizeof(void*));
    vec->arr = temp;

    return vec->arr;
}

void vecfreeind(Vector* vec, size_t ind) {
    if (!vec || !vec->arr || ind >= vec->len || !vec->arr[ind]) return;

    switch (vec->mtype) {
        case DTYPE_STRUCT: vec->freestrct(vec->arr[ind]); break;
        case DTYPE_VECTOR:        freevec(vec->arr[ind]); break;
                  default:           free(vec->arr[ind]);
    }

    vec->arr[ind] = NULL;
}



int vecrmv(Vector* vec, size_t ind) {
    if (!vec || !vec->arr || ind >= vec->len) return 1;

    vecfreeind(vec, ind);

    // Must subtract 1 for size as `vec->len - ind` would include the index itself, which we don't want to move
    if (ind < vec->len - 1) memmove(vec->arr + ind, vec->arr + ind+1, (vec->len - ind - 1) * sizeof(void*));

    // We must set the last element to NULL as memmove() duplicated it
    // We don't free it as it'd potentially free both the last and second to last elements
    // 'vec->len' is yet to be decremented, so we decrement and use that value to get rid of the duplicated last element
    vec->arr[--vec->len] = NULL;

    return 0;
}

int vecpop(Vector* vec) {
    if (!vec) return 1;
    return vecrmv(vec, vec->len - 1);
}



void* vecswap(Vector* vec, size_t ind, void* item) {
    if (!vec || ind > vec->len) return NULL;

    vecfreeind(vec, ind);
    vec->arr[ind] = item;
    
    return vec->arr[ind];
}

void* vecinsrt(Vector* vec, size_t ind, void* item) {
    if (!vec || ind > vec->len || !item) return NULL;

    if (vec->len == vec->max) {
        if (!vecrecalloc(vec)) return NULL;
    }

    // Shift everything to the right of 'ind' right by 1
    memmove(vec->arr + ind+1, vec->arr + ind, (vec->len - ind) * sizeof(void*));

    // Must do this here as vecswap() would free both the 'ind'-th and 'ind + 1'-th elements and cause a double-free later
    vec->arr[ind] = NULL;

    if (!vecswap(vec, ind, item)) return NULL;
    vec->len++;

    return vec->arr[ind];
}

void* vecinsrtsf(Vector* vec, size_t ind, void* item) {
    if (!vec || ind > vec->len || !item)  return NULL;

    if (!vec->arr[ind] && ind < vec->len) return  vecswap(vec, ind, item);
    if (ind == vec->len)                  return  vecpush(vec, item);
    else                                  return vecinsrt(vec, ind, item);
}

void* vecpush(Vector* vec, void* item) {
    if (!vec || !vec->arr || !item) return NULL;

    if (vec->len == vec->max) {
        if (!vecrecalloc(vec)) return NULL;
    }

    if (!vecswap(vec, vec->len, item)) return NULL;

    return vec->arr[vec->len++];
}

// Testing
/*
int main() {
    size_t vecSize = 10;

    Vector* vec = crtvec(vecSize, DTYPE_VECTOR, NULL, NULL);

    for (size_t i = 0; i < vecSize; i++) {
        Vector* subvec = crtvec(vecSize, DTYPE_VECTOR, NULL, NULL);
        
        for (size_t j = 0; j < vecSize; j++) {
            Vector *subsubvec = crtvec(vecSize, DTYPE_NORMAL, NULL, NULL);

            for (size_t k = 0; k < vecSize; k++) {
                int* num = malloc(sizeof(int));
                *num = k;
                vecpush(subsubvec, num);
            }

            vecpush(subvec, subsubvec);
        }

        vecpush(vec, subvec);
    }

    printf("\nVector:");
    for (size_t i = 0; i < vecSize; i++) {
        printf("\n\tSubvector %i:", (int)i);
        for (size_t j = 0; j < vecSize; j++) {
            printf("\n\t\tSubsubvector %i:\n\t\t\t", (int)j);
            for (size_t k = 0; k < vecSize; k++) {
                printf("%i%s",
                        *(int*)( (Vector*)( (Vector*)vec->arr[i] )->arr[j] )->arr[k],
                        (k < vecSize - 1) ? ", " : "");
            }
        }
    }
    printf("\n");

    freevec(vec);

    return 0;
}*/
/*
Vector* crtndimvec(unsigned int dims, unsigned int dimSize) {
    Vector* vec = crtvec(dimSize, (dims > 1) ? DTYPE_VECTOR : DTYPE_NORMAL, NULL, NULL);

    for (int i = 0; i < dimSize; i++) {
        if (dims > 1) vecpush(vec, crtndimvec(dims - 1, dimSize));
        else {
            int* num = malloc(sizeof(int));
            *num = i;
            vecpush(vec, num);
        }
    }

    return vec;
}

void printndimvec(Vector* vec, unsigned int currdepth) {
    if (vec->mtype == DTYPE_VECTOR) {
        for (int i = 0; i < vec->len; i++) {
            printf("\n");
            for (int j = 0; j <= currdepth; j++) printf("\t");

            printf("Subvector %i:", i);
            printndimvec((Vector*)vec->arr[i], currdepth + 1);
        }
    } else {
        printf("\n");
        for (int j = 0; j <= currdepth; j++) printf("\t");

        for (int i = 0; i < vec->len; i++) {
            printf(
                "%i%s",
                *(int*)vec->arr[i],
                (i < vec->len - 1) ? ", " : ""
            );
        }
    }
}

int main() {
    Vector* ndimvec = crtndimvec(10, 7);

    printf("\nVector:");
    printndimvec(ndimvec, 0);
    printf("\n");

    freevec(ndimvec);

    return 0;
}
*/
/*
int main() {
    Vector* vec = crtvec(0, DTYPE_NORMAL, NULL);

    for (int i = 0; i < 5; i++) {
        int* num = malloc(sizeof(int));
        *num = i;
        vecpush(vec, num);
    }

    for (int i = 0; i < vec->len; i++) {
        printf("\n%i", *(int*)vec->arr[i]);
    }
    printf("\n");

    vecrmv(vec, 1);
    vecpop(vec);



    int* inserted = malloc(sizeof(int));
    *inserted = 100;
    vecpush(vec, inserted);

    printf("\n\n\n");

    int* insertedtwo = malloc(sizeof(int));
    *insertedtwo = *inserted;
    vecinsrtsf(vec, 1, insertedtwo);

    int* insertedthree = malloc(sizeof(int));
    *insertedthree = 99;
    vecinsrtsf(vec, vec->len, insertedthree);

    int* insertedfour = malloc(sizeof(int));
    *insertedfour = 9999;
    vecinsrtsf(vec, 2, insertedfour);

    vecswap(vec, 2, NULL);

    int* insertedfive = malloc(sizeof(int));
    *insertedfive = 12345;
    vecinsrtsf(vec, 2, insertedfive);



    printf("\nMax: %zu", vec->max);
    printf("\nLength: %zu\n", vec->len);

    for (int i = 0; i < vec->len; i++) {
        if (vec->arr[i]) printf("\n%i", *(int*)vec->arr[i]);
        else             printf("\nNULL");
    }
    printf("\n");

    freevec(vec);

    return 0;
}*/

/*int main() {
    Vector* vec = crtvec(0, DTYPE_VECTOR, NULL);

    for (int i = 0; i < 128; i++) {
        Vector* subvec = crtvec(0, DTYPE_NORMAL, NULL);
        for (int j = 1; j <= 128; j++) {
            double* num = malloc(sizeof(double));
            *num = j * j;
            vecpush(subvec, num);
        }
        vecpush(vec, subvec);
    }

    printf("\nVector:");
    for (int i = 0; i < vec->len; i++) {
        printf("\n\tSubvector %i:", i);
        printf("\n\t\t");
        for (int j = 0; j < ((Vector*)vec->arr[i])->len; j++) {
            printf(
                "%lf%s",
                *(double*)( (Vector*)vec->arr[i] )->arr[j],
                (j < ((Vector*)vec->arr[i])->len - 1) ? ", " : ""
            );
        }
    }
    printf("\n");
    printf("\nLength: %zu; Max: %zu\n", vec->len, vec->max);

    freevec(vec);
    return 0;
}*/