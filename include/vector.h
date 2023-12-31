#ifndef VECTOR_H
#define VECTOR_H

#include <stddef.h>

struct vector {
    void *data;
    size_t element_len;
    size_t capacity;
    size_t len;
};

int make_vector(struct vector *vec, size_t elem_len, size_t size_hint);
int free_vector(struct vector *vec);

/// requests that the vector be large enough to fit at least n
/// elements. Will not reallocate unless necessary.
int vec_reserve(struct vector *vec, size_t n);

int vec_push(struct vector *vec, const void *src);

/// push n contiguous elements of src into the vector.
int vec_pushn(struct vector *vec, const void *src, size_t n);

/// ensures that there is room for n elements, and resizes the vector
/// to match this size.
int vec_resize(struct vector *vec, size_t n);

int vec_pop(struct vector *vec, void *dst);

/// removes the element at index n, and shifts following elelments.
int vec_rem(struct vector *vec, size_t n);

int vec_at(const struct vector *vec, size_t n, void *dst);
void* vec_ref(const struct vector *vec, size_t n);

#endif
