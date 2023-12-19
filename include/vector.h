#ifndef VECTOR_H
#define VECTOR_H

#include <stddef.h>

struct vector {
    void *data;
    size_t element_len;
    int capacity;
    int len;
};

int make_vector(struct vector *vec, size_t elem_len, int size_hint);
int free_vector(struct vector *vec);

int vec_push(struct vector *vec, const void *src);
int vec_pop(struct vector *vec, void *dst);
int vec_at(const struct vector *vec, int n, void *dst);
void* vec_ref(const struct vector *vec, int n);

#endif
