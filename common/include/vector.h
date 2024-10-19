#ifndef VECTOR_H
#define VECTOR_H

#include "error.h"

#include <stddef.h>

/**
 * A managed, dynamically allocated vector implentation.
 *
 * once initialized (using `make-vector`) a vector will contain a contiguous,
 * addressable block of memory. The number of elements stored in the vector is
 * kept track of by the `len` parameter. The functions that operate on vectors
 * will automatically reallocate data to fit new elements.
 */
struct vector;
typedef struct vector vector;

DEFINE_RESULT_TYPE_CUSTOM(vector *, vec)

/**
 * initialize a new vector.
 *
 * @param[out] vec pointer to an empty vector to initialize.
 * @param[in] elem_len the size of individual elements.
 * @param[in] size_hint how much space to initially allocate for the vector. if
 * this parameter is zero, the "default" value of 10 will be used. Otherwise,
 * this parameter can be used to minimize the amount of calls to `vec_reserve`.
 *
 * @return NULL if failed to allocate space, a pointer to the vector otherwise.
 */
struct vector* make_vector(size_t elem_len, size_t size_hint);

/**
 * frees the resources used by vector.
 *
 * this function makes the vector structure pointed to by `vec` invalid. this
 * function should be used once the vector has 'gone out of scope'.
 *
 * @param[in] vec the vector that will be freed.
 */
void free_vector(struct vector* vec);

void* vec_dat(struct vector* vec);
size_t vec_len(const struct vector* vec);
size_t vec_cap(const struct vector* vec);
size_t vec_element_len(const struct vector* vec);

/**
 * requests that the vector be large enough to fit *at least* n elements.
 *
 * This function will not reallocate unless necessary. It is also not garunteed
 * that the capacity of the vector will be exactly n after calling this
 * function. emphasis on *at least* n elements. 
 *
 * @param[in,out] vec The initialized vector for which to acquire additional
 * resources.
 * @param[in] n the number of elements the vector needs to be able to store.
 *
 * @return 0 if the allocation was successful, -1 on failure.
 */
int vec_reserve(struct vector* vec, size_t n);

/**
 * push an element to the end of a vector.
 *
 * the element must be the same type that the vector was initialized. Otherwise,
 * garbage data may be appended to the vector.
 *
 * @param[in,out] vec the initialized vector to add a new element to.
 * @param[in] src the element to append to the end of the vector.
 *
 * @return 0 if the element was pushed successfully, -1 if there was an
 * unsuccessful reserve/allocation.
 */
int vec_push(struct vector* vec, const void* src);

/**
 * push `n` contiguous elements of `src` into the vector.
 *
 * It is a common operation to want to append an existing array to a vector. as
 * long as `src` is a contiguous block of `n` elements, then this funcition can
 * be used to append those values to `vec`.
 *
 * @param[in,out] vec initialized vector to append elements to.
 * @param[in] src beginning of the block of elements to appned to `vec`.
 * @param[in] n number of elements to contained in `src`.
 *
 * @return 0 if all the elements were successfully appended, -1 if there was an
 * allocation error, and the function failed. 
 */
int vec_pushn(struct vector* vec, const void* src, size_t n);

/**
 * makes the vector exactly `n` elements in length, padding/truncating as
 * necessary.
 *
 * @param[in,out] vec the vector to resize.
 * @param[in] n the new length of the vector.
 *
 * @return 0 if the operation was successful. -1 if there is an allocation
 * failure.
 */
int vec_resize(struct vector* vec, size_t n);

/**
 * remove the last element from the vector, and copy it into `dst`.
 *
 * if `dst` is null, then this function doesn't copy anything, and simply
 * removes the last element.
 * 
 * @param[in,out] vec the vector to 'pop' from.
 * @param[out] dst pointer to location to copy last element in vector to, or
 *                  NULL
 *
 * @return 0 on success, -1 if length was zero and nothing was copied.
 */
int vec_pop(struct vector* vec, void* dst);

/**
 * removes the element at indext `n`, shifting subsequent elements left.
 *
 * @param[in,out] vec vector to remove an element from.
 * @param[in] n index to remove an element from.
 *
 * @return 0 on success, -1 if the element wasn't in bounds, and no elements
 * were removed.
 */
int vec_rem(struct vector* vec, size_t n);

/**
 * copies the element at index `n` into `dst`.
 *
 * @param[in,out] vec the vector to copy an element from.
 * @param[in] n the index to copy from in the vector.
 * @param[out] dst pointer to a location to copy the element into. Must be a
 * valid pointer.
 *
 * @return if n is out of bounds or `src` is NULL, returns -1. zero is returned
 * otherwise.
 */
int vec_at(const struct vector* vec, size_t n, void* dst);

/**
 * Indexes index `n` of the vector, returning a reference (pointer).
 *
 * since the vector API is generic across types, the return value will have to
 * be cast the to appropriate type of this vector. Great care must also be taken
 * when using this function. If the capacity of the vector is changed, this
 * pointer will be INVALID.
 *
 * this function will return a reference for any elements address that is within
 * the capacity of the vector. You can get a reference for uninitializecd
 * elements if you desire. However, this function will not grant references to
 * memory that hasn't been reserved.
 *
 * @param[in,out] vec the vector to obtain a reference too.
 * @param[in] n the index of interest.
 *
 * @return A pointer to index `n` in the vector, or NULL if n is out of bounds.
 */
void* vec_ref(const struct vector* vec, size_t n);

/**
 * Returns a reference to the last element in the vector.
 * 
 * @param[in] vec the vector to obtain a reference too.
 *
 * @return a pointer to hte last element in the vector.
 */
void* vec_end(const struct vector* vec);

/**
 * similar to `vec_ref` but with a byte offset rather than an index.*/
void* vec_byte_ref(const struct vector* vec, size_t offset);

/**
 * sets element `n` to the value at `src` by copying the value into the vector.
 *
 * @param[in,out] vec the vector to index in.
 * @param[in] n the index of the element to set.
 * @param[in] src pointer to the value to set the element to.
 *
 * @return 0 on success, -1 if `n` is out of bounds or `src` is NULL.
*/
int vec_set(struct vector* vec, size_t n, const void* src);

/**
 * pushes the contents of `src` onto `vec`
 */
int vec_concat(struct vector* vec, const struct vector* src);

/**
 * copies `size` bytes starting at the byte index `offset` to `dst`.  The actual
 * element length is not used in this function.  This may be useful for casting
 * operations.*/
int vec_bytes(const struct vector* vec, size_t offset, size_t size, void* dst);

#endif
