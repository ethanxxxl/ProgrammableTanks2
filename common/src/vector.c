#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector.h>
#include <stdint.h>

int make_vector(struct vector *vec, size_t elem_len, size_t size_hint) {
    if (size_hint > 0) {
        vec->data = malloc(elem_len * size_hint);
        vec->capacity = size_hint;
    } else {
        vec->data = malloc(elem_len * 10);
        vec->capacity = 10;
    }

    if (vec->data == NULL) {
        return -1;
    }

    vec->element_len = elem_len;
    vec->len = 0;

    // initialize newly allocated memory
    memset(vec->data, 0, vec->element_len * vec->capacity);

    return 0;
}

int free_vector(struct vector *vec) {
    free(vec->data);
    vec->data = NULL;
    return 0;
}

int vec_reserve(struct vector *vec, size_t n) {
    if (vec->capacity > n)
        return 0;

    // reserve twice as much as requested, to reduce reallocs.
    void *tmp = realloc(vec->data, vec->element_len * n*2);
    if (tmp == NULL) {
        return -1;
    }

    vec->data = tmp;
    vec->capacity = n*2;

    // inialize newly allocated memory
    void *data_start = vec_ref(vec, vec->len);
    memset(data_start, 0, vec->element_len * (vec->capacity - vec->len));

    return 0;
}

int vec_push(struct vector *vec, const void *src) {
    int status = vec_reserve(vec, vec->len+1);
    if (status != 0)
        return status;
    
    memcpy(vec_ref(vec, vec->len),
           src, vec->element_len);

    vec->len++;

    return 0;
}

int vec_pushn(struct vector *vec, const void *src, size_t n) {
    int status = vec_reserve(vec, vec->len+n);
    if (status != 0)
        return status;
   
    memcpy(vec_ref(vec, vec->len), src, n * vec->element_len);
    vec->len += n;
    return 0;
}

int vec_resize(struct vector *vec, size_t n) {
    int status = vec_reserve(vec, n);
    if (status != 0)
        return status;
    
    vec->len = n;
    return 0;
}

int vec_pop(struct vector *vec, void *dst) {
    if (vec->len == 0)
        return -1;
    
    if (dst != NULL)
        memcpy(dst, vec_ref(vec, vec->len), vec->element_len);

    vec->len--;

    return 0;
}

int vec_rem(struct vector *vec, size_t n) {
    if (n > vec->len)
        return -1;

    if (n == vec->len - 1) {
        vec_pop(vec, NULL);
        return 0;
    }
    
    memmove(vec_ref(vec, n),
            vec_ref(vec, n+1),
            (vec->len - n - 1) * vec->element_len);

    vec->len--;
    
    return 0;
}

int vec_at(const struct vector *vec, size_t n, void *dst) {
    if (n >= vec->len || dst == NULL)
        return -1;
        
    memcpy(dst,
           vec_ref(vec, n),
           vec->element_len);
    
    return 0;
}

void* vec_ref(const struct vector *vec, size_t n) {
    if (n >= vec->capacity) {
        printf("WARNING! VEC_REF RETURNING NULL DUE TO OUT OF BOUNDS N\n");
        return NULL;
    }
    
    return (uint8_t*)(vec->data) + (vec->element_len * n);
}

int vec_set(struct vector *vec, size_t n, const void *src) {
    if (n <= vec->len || src == NULL)
        return -1;
    
    memcpy(vec_ref(vec, n),
           src,
           vec->element_len);
    
    return 0;
}
