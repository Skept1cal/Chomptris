#ifndef VECTOR_H
#define VECTOR_H

#include <stddef.h>

#define TYPE_NULL 0



/*
Type for a Vector.

 - `arr`: a `void**` array of heap-pointers;
 - `len, max`: trackers for the length and maximum capacity of the `Vector`;
 - `mtype, stype`: main-type and struct-type;
 - - `mtype` can pick up values from the `DTYPE_T` enum. `mtype` cannot be `TYPE_NULL`.
 - - `stype` is merely a marker for Vectors of type `DTYPE_STRUCT` for explicit declarations,
     and does not prevent differently-typed structs ending up in the same Vector.
 - `freestrct`: a pointer to a custom-defined function to free structs in a Vector of type `DTYPE_STRUCT`.
*/
typedef struct Vector {
    void**        arr; // Fully heap-allocated
    size_t        len, max;
    unsigned char mtype, stype; // stype is redundant, but it may have practical usage for convenience
    void          (*freestrct)(void*);
} Vector;



/*
Type for a built-in-type enum.

Values:
 - `DTYPE_NORMAL`: Type for a Vector containing anything other than Vectors or structs;
 - `DTYPE_VECTOR`: Type for a Vector containing Vectors;
 - `DTYPE_STRUCT`: Type for a Vector containing custom-defined structs
*/
typedef enum DTYPE_T {
    DTYPE_NORMAL = 1, // Anything other than a Vector, struct, etc.
    DTYPE_VECTOR,
    DTYPE_STRUCT
} DTYPE_T;


/*
Creates a Vector with an initial capacity of `size`,
a main-type of `type`,
and a freeing-function-pointer for Vectors of type `DTYPE_STRUCT` (`freefnc`).

The function initializes `stype` to `TYPE_NULL`.

If `size` is equal to 0, the function initializes the Vector with an initial capacity of `2`.

Return value:
 - `NULL` if:
 - - `type` is not included in the `DTYPE_T` enum, or is `TYPE_NULL`,
 - - Allocating memory for the `Vector` struct failed,
 - - Allocating memory for the Vector's inner array (`[Vector]->arr`) failed;
 - A pointer to a newly created `Vector` otherwise
*/
Vector* crtvec   (size_t size, unsigned char  type,                      void (*freefnc)(void*));

/*
Same as `crtvec()`, except `stype` can be specified for explicit marking and will replace `crtvec()` 's initialization of it.

Return value:
 - Same as `crtvec()`
*/
Vector* crtvecalt(size_t size, unsigned char mtype, unsigned char stype, void (*freefnc)(void*));



/*
Frees the `vec` Vector.
 - The behaviour is undefined if the type of the Vector is `DTYPE_STRUCT`, but `vec->freestrct()` is `NULL`.
 - The function trusts that `vec->freestrct()` takes care of a full, deep-free of the struct.
*/
void  freevec    (Vector* vec);

/*
Reallocates a `Vector` and initializes the newly allocated indices to `NULL`.

The Vector is untouched in case of failure.

Return value:
 - `NULL` if reallocation fails;
 - A pointer to the newly reallocated `vec->arr` otherwise
*/
void* vecrecalloc(Vector* vec);

/*
Frees the `ind` index of the `vec` Vector.
*/
void  vecfreeind (Vector* vec, size_t ind);



/*
Removes the `ind` index of `vec`.
Everything after `vec` 's `ind` index is shifted to the left by 1.

Return value:
 - `1` if:
 - - `vec` or `vec->arr` is a `NULL` pointer,
 - - `ind` is not less than `vec->len`;
 - `0` otherwise
*/
int vecrmv(Vector* vec, size_t ind);

/*
Removes the last element of `vec`.
Equivalent to `vecrmv(vec, vec->len - 1);`.

Return value:
 - `1` if `vec` is a `NULL` pointer; 
 - `vecrmv()` 's return value otherwise
*/
int vecpop(Vector* vec);



/*
Replaces the `ind` index of `vec` with `item`.
 - The old item at the `ind` index of `vec` is freed before being set to `item`.

The function allows `item` to be `NULL`.
The function does not handle reallocation in case of `ind == vec->len` (appending).

Return value:
 - `NULL` if:
 - - `vec` is a `NULL` pointer,
 - - `ind` is not less than or equal to `vec->len`
 - newly set `ind` index otherwise
*/
void* vecswap   (Vector* vec, size_t ind, void* item);

/*
Shifts everything past `ind` in `vec` to the right,
then sets the `ind` index of `vec` to `item`.

The function will shift elements regardless of whether the affected
index was `NULL` or not.

Return value:
 - `NULL` if:
 - - `vec` or `item` is a `NULL` pointer,
 - - `ind` is not less than or equal to `vec->len`,
 - - on failure;
 - `ind` index otherwise
*/
void* vecinsrt  (Vector* vec, size_t ind, void* item);

/*
Safer alternative to `vecinsrt()`.
 - If the element is `NULL` and `ind` is less than `vec->len` (not appending),
   replaces the `ind` index of `vec` with `item` via `vecswap()`;
 - If `ind` is equal to `vec->len` (appending),
   pushes `item` to `vec` via `vecpush()`;
 - Otherwise,
   inserts `item` before the `ind` indexof `vec` via `vecinsrt()`.

Unlike `vecinsrt()`,
this function doesn't force a shift to the right regardless of whether the affected element was `NULL` or not.

Return value:
 - `NULL` if `vec` or `item` is a `NULL` pointer;
 - Return value of the called function otherwise
*/
void* vecinsrtsf(Vector* vec, size_t ind, void* item);

/*
Appends `item` to `vec`.
Values past `vec->len` are not shifted right to do this.

Return value:
 - `NULL` if:
 - - `vec`, `vec->arr` or `item` is a `NULL` pointer,
 - - on failure;
 - the pushed element otherwise
*/
void* vecpush   (Vector* vec,             void* item);

#endif