#include <stdlib.h>
#include <string.h>
#include <vector.h>

int make_vector(struct vector *vec, size_t elem_len, int size_hint) {
    if (size_hint >= 0) {
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

    return 0;
}

int free_vector(struct vector *vec) {
    free(vec->data);
    return 0;
}

int vec_push(struct vector *vec, const void *src) {
    if (vec->capacity <= vec->len) {
	void *tmp = realloc(vec->data, vec->capacity * 2);

	if (tmp == NULL) {
	    return -1;
	}

	vec->data = tmp;
	vec->capacity *= 2;
    }

    memcpy(vec_ref(vec, vec->len),
	   src, vec->element_len);

    vec->len++;

    return 0;
}

int vec_pop(struct vector *vec, void *dst) {
    memcpy(dst,
	   vec_ref(vec, vec->len--),
	   vec->element_len);
    
    return 0;
}

int vec_at(const struct vector *vec, int n, void *dst) {
    memcpy(dst,
	   vec_ref(vec, n),
	   vec->element_len);
    
    return 0;
}

void* vec_ref(const struct vector *vec, int n) {
    return vec->data + (vec->element_len * n);
}

int vec_set(const struct vector *vec, int n, void* src) {
    memcpy(vec_ref(vec, n),
	   src,
	   vec->element_len);
    
    return 0;
}
