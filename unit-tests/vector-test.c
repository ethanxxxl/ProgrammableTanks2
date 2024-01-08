#include "unit-test.h"
#include "scenario.h"

#include "vector.h"

// HACK these are externs referenced in scenario.c which are defined in the
// server translation unit. In order for the tests to compile, they need to
// provide a defnition for this global variable.
extern struct scenario g_scenario;
struct scenario g_scenario;

const char* tst_vec_init_free(void) {
    struct vector vec;
    int status = make_vector(&vec, sizeof(int), 0);

    // bad news if we make a vector with an invalid pointer.
    if (status != -1 && vec.data == NULL)
        return "made an invalid vector";

    status = free_vector(&vec);
    if (status != 0 || vec.data != NULL)
        return "failed to NULL out data field";

    return NULL;
}
const char* tst_vec_push(void) {
    int ret;
    struct vector vec;
    ret = make_vector(&vec, sizeof(int), 3);

    if (ret != 0)
        return "failed to create vector";

    char *return_error = NULL;
    static char error[50] = {0};
    for (int i = 0; i < 5000; i++) {
        for (int j = 0; j < i; j++) {
            // guarantee no previous elements are being written over.
            if (((int *)vec.data)[j] != j) {
                snprintf(error, 50,
                         "%dth push operation overwrote %dth element.", i, j);
                
                return_error = error;
                goto cleanup_return;
            }   
        }
       
        ret = vec_push(&vec, &i);
        if (ret != 0) {
            return_error = "vec_push self reported failure";
            goto cleanup_return;
        }
            
        
        if (((int *)vec.data)[i] != i) {
            snprintf(error, 50, "pushing failed on element %d.", i);
            return_error = error;
            goto cleanup_return;
        }
    }

 cleanup_return:
    free_vector(&vec);
    return return_error;
}
const char* tst_vec_pushn(void) {
    int ret;

    struct vector vec;
    ret = make_vector(&vec, sizeof(int), 1);
    if (ret != 0)
        return "failed to create vector";

    // setup an array to copy.
    const int values_len = 50;
    int values[values_len];
    for (size_t i = 0; i < values_len; i++) {
        values[i] = i;
    }

    char *return_error = NULL;
    static char error[100];
    for (int i = 0; i < 200; i++) {
        for (int j = 0; j < i; j++) {
            // verify integrity of previously pushed elements
            ret = memcmp(values, ((int *)vec.data) + (values_len * j),
                         values_len);

            if (ret != 0) {
                snprintf(error, 100,
                         "%dth pushn overwrote data between %d and %d.",
                         i, j * values_len, (j+1)*values_len - 1);
                return_error = error;
                goto cleanup_return;
            }
        }

        ret = vec_pushn(&vec, values, values_len);
        if (ret != 0) {
            snprintf(error, 100, "%dth pushn returned -1.", i);
            return_error = error;
            goto cleanup_return;
        }

        ret = memcmp(values, ((int *)vec.data) + (values_len * i), values_len);
        if (ret != 0) {
            snprintf(error, 100, "%dth pushn wrote bad data", i);
            return_error = error;
            goto cleanup_return;
        }
    }

 cleanup_return:
    free_vector(&vec);
    return return_error;
}
const char* tst_vec_ref(void) {
    int ret;

    struct vector vec;
    ret = make_vector(&vec, sizeof(int), 1);
    if (ret != 0)
        return "failed to create vector.";

    for (int x = 0; x < 1000; x++)
        vec_push(&vec, &x);


    char *error_message = NULL;
    static char error[50];
    for (int x = 0; x < 1000; x++) {
        int *ref = vec_ref(&vec, x);

        if (*ref != x) {
            snprintf(error, 50, "incorrect reference on %dth element.", x);
            error_message = error;
            goto cleanup_return;
        }

        // out of bounds detection
        if (ref >= (int *)vec.data + vec.capacity) {
            snprintf(error, 50, "reference to unallocated memory at %d.", x);
            error_message = error;
            goto cleanup_return;
        }
    }
        
 cleanup_return:
    free_vector(&vec);
    return error_message;
}
const char* tst_vec_reserve(void) {
    return TEST_NOT_IMPLEMENTED_ERROR;
}
const char* tst_vec_resize(void) {
    return TEST_NOT_IMPLEMENTED_ERROR;
}
const char* tst_vec_pop(void) {
    return TEST_NOT_IMPLEMENTED_ERROR;
}
const char* tst_vec_rem(void) {
    return TEST_NOT_IMPLEMENTED_ERROR;
}
const char* tst_vec_at(void) {
    return TEST_NOT_IMPLEMENTED_ERROR;
}
const char* tst_vec_set(void) {
    return TEST_NOT_IMPLEMENTED_ERROR;
}

struct test g_all_tests[] = {
    {"make/free", &tst_vec_init_free},
    {"vec_push", &tst_vec_push},
    {"vec_pushn", &tst_vec_pushn},
    {"vec_ref", &tst_vec_ref},
    {"vec_reserve", &tst_vec_reserve},
    {"vec_resize", &tst_vec_resize},
    {"vec_pop", &tst_vec_pop},
    {"vec_rem", &tst_vec_rem},
    {"vec_at", &tst_vec_at},
    {"vec_set", &tst_vec_set},
};

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    size_t num_tests = sizeof(g_all_tests)/sizeof(struct test);
    run_test_suite(g_all_tests, num_tests, "vector tests");
    
    return 0;
}
