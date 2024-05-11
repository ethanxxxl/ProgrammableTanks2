#include "unit-test.h"
#include "scenario.h"

#include "vector.h"
#include <limits.h>
#include <stdint.h>
#include <string.h>

const char INIT_FAIL[] = "falled to create vector.";

// HACK these are externs referenced in scenario.c which are defined in the
// server translation unit. In order for the tests to compile, they need to
// provide a defnition for this global variable.
extern struct scenario g_scenario;
struct scenario g_scenario;

const char* tst_vec_push(void) {
    int ret;
    struct vector* vec = make_vector(sizeof(int), 3);

    if (vec == NULL)
        return INIT_FAIL;

    char *return_error = NULL;
    static char error[63] = {0};
    for (int i = 0; i < 5000; i++) {
        for (int j = 0; j < i; j++) {
            // guarantee no previous elements are being written over.
            if (((int*)vec_dat(vec))[j] != j) {
                snprintf(error, 63,
                         "%dth push operation overwrote %dth element.", i, j);
                
                return_error = error;
                goto cleanup_return;
            }   
        }
       
        ret = vec_push(vec, &i);
        if (ret != 0) {
            return_error = "vec_push self reported failure";
            goto cleanup_return;
        }
            
        
        if (((int*)vec_dat(vec))[i] != i) {
            snprintf(error, 63, "pushing failed on element %d.", i);
            return_error = error;
            goto cleanup_return;
        }
    }

 cleanup_return:
    free_vector(vec);
    return return_error;
}

const char* tst_vec_push_null(void) {
    int ret;
    struct vector* vec = make_vector(sizeof(int), 0);
    
    if (vec == NULL)
        return INIT_FAIL;
    
    ret = vec_push(vec, NULL);
    if (ret != -1 || vec_len(vec) != 0)
        return "pushed a fictitious element onto stack.";

    free_vector(vec);
    return NULL;
}

const char* tst_vec_pushn(void) {
    int ret;

    struct vector* vec = make_vector(sizeof(int), 1);
    if (vec == NULL)
        return INIT_FAIL;

    // setup an array to copy.
    const int values_len = 50;
    int values[values_len];
    for (int i = 0; i < values_len; i++) {
        values[i] = i;
    }

    char *return_error = NULL;
    static char error[100];
    for (int i = 0; i < 200; i++) {
        for (int j = 0; j < i; j++) {
            // verify integrity of previously pushed elements
            ret = memcmp(values, ((int *)vec_dat(vec)) + (values_len * j),
                         values_len);

            if (ret != 0) {
                snprintf(error, 100,
                         "%dth pushn overwrote data between %d and %d.",
                         i, j * values_len, (j+1)*values_len - 1);
                return_error = error;
                goto cleanup_return;
            }
        }

        ret = vec_pushn(vec, values, values_len);
        if (ret != 0) {
            snprintf(error, 100, "%dth pushn returned -1.", i);
            return_error = error;
            goto cleanup_return;
        }

        ret = memcmp(values, ((int *)vec_dat(vec)) + (values_len * i), values_len);
        if (ret != 0) {
            snprintf(error, 100, "%dth pushn wrote bad data.", i);
            return_error = error;
            goto cleanup_return;
        }
    }

    ret = vec_pushn(vec, NULL, 50);

 cleanup_return:
    free_vector(vec);
    return return_error;
}

const char* tst_vec_pushn_null(void) {
    int ret;
    struct vector* vec = make_vector(sizeof(int), 0);
    if (vec == NULL)
        return INIT_FAIL;
    
    ret = vec_pushn(vec, NULL, 50);
    if (ret != -1 || vec_len(vec) != 0)
        return "pushed fictitious elements onto stack";

    free_vector(vec);
    return NULL;
}

const char* tst_vec_ref(void) {
    struct vector* vec = make_vector(sizeof(int), 1);
    if (vec == NULL)
        return INIT_FAIL;

    for (int x = 0; x < 1000; x++)
        vec_push(vec, &x);


    char *error_message = NULL;
    static char error[50];
    for (int x = 0; x < 1000; x++) {
        int *ref = vec_ref(vec, x);

        if (*ref != x) {
            snprintf(error, 50, "incorrect reference on %dth element.", x);
            error_message = error;
            goto cleanup_return;
        }

        // out of bounds detection
        if (ref >= (int *)vec_dat(vec) + vec_cap(vec)) {
            snprintf(error, 50, "reference to unallocated memory at %d.", x);
            error_message = error;
            goto cleanup_return;
        }
    }

 cleanup_return:
    free_vector(vec);
    return error_message;
}

const char* tst_vec_reserve(void) {
    int ret;

    struct vector* vec = make_vector(sizeof(int), 1);
    if (vec == NULL)
        return INIT_FAIL;

    char *error_message = NULL;
    for (size_t i = 10; i < 550; i += 10) {
        ret = vec_reserve(vec, i);
        if (ret == 0 && vec_dat(vec) == NULL) {
            error_message = "invalid pointer in vector.";
            goto cleanup_return;
        }

        if (vec_cap(vec) < i) {
            error_message = "failed to allocate enough space";
            goto cleanup_return;
        }

        for (size_t j = 0; j < i-10; j++) {
            // oof, ugly casting.
            // void* -> int* -> dereferenced -> size_t
            if ((size_t)*(int *)vec_ref(vec, j) != j) {
                static char msg_buff[63];
                snprintf(msg_buff, 63, "memory corrupted at j=%zu; i=%zu: %d",
                         j,i, *(int *)vec_ref(vec, j));
                
                error_message = msg_buff;
                goto cleanup_return;
            }
        }
        
        // this will segfault if vec_reserve is lying to us.
        for (size_t j = i - 10; j < i; j++) {
            ret = vec_push(vec, &j);
            if (ret != 0) {
                error_message = "failed to push item into vector.";
                goto cleanup_return;
            }
        }
    }

    vec_reserve(vec, SIZE_MAX);
    if (ret == 0 && vec_dat(vec) == NULL) {
        error_message = "invalid pointer in vector. (after SIXE_MAX)";
        goto cleanup_return;
    }

 cleanup_return:
    free_vector(vec);
    return error_message;
}

const char* tst_vec_reserve_zero(void) {
    int ret;
    char *error_message = NULL;

    struct vector* vec = make_vector(sizeof(int), 1);
    if (vec == NULL)
        return INIT_FAIL;

    // this should just be a do nothing call really.
    ret = vec_reserve(vec, 0);
    if (vec_dat(vec) == NULL)
        error_message = "destroyed the vector pointer.";
    else if (ret != 0)
        error_message = "failed on a \"successful\" allocation.";

    free_vector(vec);
    return error_message;
}

const char* tst_vec_resize(void) {
    int ret;
    char *error_message = NULL;

    struct vector* vec = make_vector(sizeof(int), 1);
    if (vec == NULL)
        return INIT_FAIL;

    // initialize vector
    for (int i = 0; i < 1000; i++) {
        ret = vec_push(vec, &i);
        if (ret != 0) {
            error_message = "vec_push failed.";
            goto cleanup_return;
        }
    }

    // down size vector
    for (size_t n = 999; n > 0; n--) {
        ret = vec_resize(vec, n);
        if (ret != 0) {
            static char message_buff[50];
            snprintf(message_buff, 50,
                     "resize failed to decrease length to %zu.", n);
            error_message = message_buff;
            goto cleanup_return;
        }

        if (vec_len(vec) != n) {
            error_message = "the vector was not resized.";
            goto cleanup_return;
        }
        
        for (size_t j = 0; j < n; j++) {
            // oof ugly cast.
            // void * -> int * -> dereference -> size_t
            if ((size_t)*(int *)vec_ref(vec, j) != j) {
                error_message = "data corrupted after resize.";
                goto cleanup_return;
            }
        }
    }

    ret = vec_resize(vec, 3000);
    if (ret != 0 || vec_len(vec) != 3000) {
        error_message = "resize failed on a small allocation.";
        goto cleanup_return;
    }

    // its ok if the vector fails to realloc here. that is a lot of memory.
    ret = vec_resize(vec, SIZE_MAX);
    if ((ret == 0 && vec_len(vec) != SIZE_MAX) ||
        (ret == 0 && vec_dat(vec) == NULL)) {
        error_message = "failed to resize properly (SIZE_MAX)";
        goto cleanup_return;
    }

    if (ret == 0 && vec_cap(vec) < SIZE_MAX) {
        error_message = "failed to allocate enough space.";
        goto cleanup_return;
    }
    
 cleanup_return:
    free_vector(vec);
    return error_message;
}

const char* tst_vec_pop(void) {
    int ret;
    char *error_message = NULL;

    struct vector* vec = make_vector(sizeof(int), 1);
    if (vec == NULL)
        return INIT_FAIL;

    // initialize vector.
    for (int i = 0; i < 1000; i++) {
        ret = vec_push(vec, &i);
        if (ret != 0) {
            error_message = "failed to push into vector.";
            goto cleanup_return;
        }
    }

    for (int i = 999; i >= 0; i--) {
        int popped;
        ret = vec_pop(vec, &popped);
        if (ret != 0) {
            error_message = "pop returned -1 when it shouldn't have.";
            goto cleanup_return;
        }

        if (popped != i) {
            static char message_buff[50];
            snprintf(message_buff, 50, "corrupted data wrote to src: %d != %d",
                     i, popped);
            error_message = message_buff;
            goto cleanup_return;
        }
    }
        
 cleanup_return:
    free_vector(vec);
    return error_message;
}

// tests pop with NULL src
const char* tst_vec_pop_null(void) {
    int ret;
    char *error_message = NULL;

    struct vector* vec = make_vector(sizeof(int), 1);
    if (vec == NULL)
        return INIT_FAIL;

    int val = 1234;
    vec_push(vec, &val);

    ret = vec_pop(vec, NULL);
    if (ret != 0) 
        error_message = "return != 0";

    free_vector(vec);
    return error_message;
}

// tests pop on an empty vector
const char* tst_vec_pop_empty(void) {
    int ret;
    char *error_message = NULL;

    struct vector* vec = make_vector(sizeof(int), 1);
    if (vec == NULL)
        return INIT_FAIL;

    ret = vec_pop(vec, NULL);
    if (ret == 0)
        error_message = "popped on an empty list.";

    free_vector(vec);
    return error_message;
}

const char* tst_vec_rem_front(void) {
    int ret;
    char *error_message = NULL;

    struct vector* vec = make_vector(sizeof(int), 1);
    if (vec == NULL)
        return INIT_FAIL;

    // initialize vector
    for (int i = 0; i < 100; i++)
        vec_push(vec, &i);

    for (int num_removed = 1; num_removed < 100; num_removed++) {
        ret = vec_rem(vec, 0);
        if (ret != 0) {
            error_message = "failed to remove in full list";
            goto cleanup_return;
        }

        static char message_buf[50];
        for (int j = 0; j < 100 - num_removed; j++) {
            int n;
            vec_at(vec, j, &n);
            
            if (n != j + num_removed) {
                snprintf(message_buf, 50, "corrupted data: vec[%d] = %d != %d",
                         j, n, j + num_removed);
                error_message = message_buf;
                goto cleanup_return;
            }
        }
    }
        
 cleanup_return:
    free_vector(vec);
    return error_message;
}

const char* tst_vec_rem_end(void) {
    int ret;
    char *error_message = NULL;

    struct vector* vec = make_vector(sizeof(int), 1);
    if (vec == NULL)
        return INIT_FAIL;
    
    // initialize vector
    for (int i = 0; i < 100; i++)
        vec_push(vec, &i);

    for (int num_removed = 1; num_removed < 100; num_removed++) {
        ret = vec_rem(vec, vec_len(vec) - 1);
        if (ret != 0) {
            error_message = "failed to remove in full list";
            goto cleanup_return;
        }

        static char message_buf[50];
        for (int j = 0; j < 100 - num_removed; j++) {
            int n;
            vec_at(vec, j, &n);
            
            if (n != j) {
                snprintf(message_buf, 50, "corrupted data: vec[%d] = %d != %d",
                         j, n, j);
                error_message = message_buf;
                goto cleanup_return;
            }
        }
    }
    
 cleanup_return:
    free_vector(vec);
    return error_message;
}

const char* tst_vec_rem_out_of_bounds(void) {
    int ret;
    char *error_message = NULL;

    struct vector* vec = make_vector(sizeof(int), 1);
    if (vec == NULL)
        return INIT_FAIL;

    int val = 333;
    vec_push(vec, &val);
    ret = vec_rem(vec, 1);
    if (ret == 0 ||
        vec_len(vec) != 1 ||
        *(int *)vec_dat(vec) != 333)
        error_message = "messed with the vector, should have returned -1.";

    free_vector(vec);
    return error_message;
}


const char* tst_vec_at(void) {
    int ret;
    char *error_message = NULL;

    struct vector* vec = make_vector(sizeof(int), 1);
    if (vec == NULL)
        return INIT_FAIL;

    int val = 454;
    vec_push(vec, &val);

    int dst;
    ret = vec_at(vec, 0, &dst);
    if (ret != 0)
        error_message = "failed to index an existing element";
    else if (dst != val) {
        error_message = "corrupted data";
    }

    free_vector(vec);
    return error_message;
}

const char* tst_vec_at_out_of_bounds(void) {
    int ret;
    char *error_message = NULL;

    struct vector* vec = make_vector(sizeof(int), 1);
    if (vec == NULL)
        return INIT_FAIL;

    int dst = 23;
    ret = vec_at(vec, 0, &dst);
    if (ret == 0)
        error_message = "succeeded on zero length vector";

    if (dst != 23)
        error_message = "wrote garbage to dst";

    free_vector(vec);
    return error_message;
}

const char* tst_vec_at_null_dst(void) {
    int ret;
    char *error_message = NULL;

    struct vector* vec = make_vector(sizeof(int), 1);
    if (vec == NULL)
        return INIT_FAIL;

    int val = 12;
    vec_push(vec, &val);
    ret = vec_at(vec, 0, NULL);

    if (ret == 0)
        error_message = "claims to have written to NULL";

    free_vector(vec);
    return error_message;
}

const char* tst_vec_set(void) {
    int ret;
    char *error_message = NULL;

    struct vector* vec = make_vector(sizeof(int), 1);
    if (vec == NULL)
        return INIT_FAIL;

    for (int i = 0; i < 1000; i++)
        vec_push(vec, &i);

    int clr = 0;

    // clear every other element.
    static char message_buf[50];
    for (int i = 0; i < 1000; i+=2) {
        ret = vec_set(vec, i, &clr);
        if (ret != 0) {
            snprintf(message_buf, 50, "didn't set an element: %d", i);
            error_message = message_buf;
            goto cleanup_return;
        }

        for (int j = 0; j < 1000; j++) {
            int n;
            vec_at(vec, j, &n);

            int correct_n = (j % 2) == 0 ? ((j <= i) ? clr : j) : j;
            if (n != correct_n) {
                snprintf(message_buf, 50, "corrupted data: vec[%d] = %d != %d",
                         j, n, correct_n);
                error_message = message_buf;
                goto cleanup_return;
            }
        }
    }

 cleanup_return:
    free_vector(vec);
    return error_message;
}

const char* tst_vec_set_null(void) {
    int ret;
    char *error_message = NULL;

    struct vector* vec = make_vector(sizeof(int), 1);
    if (vec == NULL)
        return INIT_FAIL;

    int val = 234;
    vec_push(vec, &val);
    ret = vec_set(vec, 0, NULL);

    if (ret == 0)
        error_message = "claims to have coppied from NULL";

    free_vector(vec);
    return error_message;
}

const char* tst_vec_set_out_of_bounds(void) {
    int ret;
    char *error_message = NULL;

    struct vector* vec = make_vector(sizeof(int), 1);
    if (vec == NULL)
        return INIT_FAIL;

    // clear out this memory for testing.
    char clear[50] = {0};
    vec_pushn(vec, clear, 50);
    vec_resize(vec, 0);
    
    int value = 234;
    ret = vec_set(vec, 0, &value);
    if (ret == 0 ||
        *(int *)vec_dat(vec) == 234)
        error_message = "wrote out fo bounds";
    
    free_vector(vec);
    return error_message;
}

struct test g_all_tests[] = {
    {"vec_push", &tst_vec_push},
    {"vec_push with NULL src", &tst_vec_push_null},
    {"vec_pushn", &tst_vec_pushn},
    {"vec_pushn with NULL src", &tst_vec_pushn_null},
    {"vec_ref", &tst_vec_ref},
    {"vec_reserve", &tst_vec_reserve},
    {"vec_reserve with 0 bytes", &tst_vec_reserve_zero},
    {"vec_resize", &tst_vec_resize},
    {"vec_pop", &tst_vec_pop},
    {"vec_pop with NULL dst", &tst_vec_pop_null},
    {"vec_pop on empy vector", &tst_vec_pop_empty},
    {"vec_rem from begining", &tst_vec_rem_front},
    {"vec_rem from end", &tst_vec_rem_end},
    {"vec_rem out of bounds", &tst_vec_rem_out_of_bounds},
    {"vec_at", &tst_vec_at},
    {"vec_at out of bounds", &tst_vec_at_out_of_bounds},
    {"vec_at with NULL dst", &tst_vec_at_null_dst},
    {"vec_set", &tst_vec_set},
    {"vec_set with NULL src", &tst_vec_set_null},
    {"vec_set out of bounds", &tst_vec_set_out_of_bounds},
};

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    size_t num_tests = sizeof(g_all_tests)/sizeof(struct test);
    run_test_suite(g_all_tests, num_tests, "vector tests");
    
    return 0;
}
