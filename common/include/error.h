#ifndef RESULT_H
#define RESULT_H

#include "nonstdint.h"

#include <stddef.h>

// TODO create a global/thread-local error list that holds references to errors
// until they are handled.  This will allow errors to be caught and logged when
// they are generated and not handled.

/***************************** Error Object Type ******************************/
struct error_ops {
    const char * (*describe)(void *);
    void (*free)(void *);
};

struct error {
    const struct error_ops *operations;

    // object pointer
    void *self;
};

// Helper Functions
const char *describe_error(const struct error e);

void free_error(const struct error e);

/******************************* Generic Error ********************************/
const char *describe_error_generic(void *self);
void free_error_generic(void *self);

const struct error_ops ERROR_GENERIC_OPS = {
    .describe = describe_error_generic,
    .free = free_error_generic,
};

struct error make_error_generic(const char* error_message);

/***************************** Result Object Type *****************************/

/*

  You want an error type system that can be used across your program, and
  provide a unified way of returning/handling errors.

  REQUIREMENTS:
  - Provide a result/option type that can contain error information
  - Allow for returns to be passed by value and not exclusively by reference.
  - Allow for type-safety to be maintained with return values.
  - Should not be prohibitively cumbersome to create, check for, or handle errors.
*/

enum result_status {
    RESULT_OK,
    RESULT_ERROR,
};

#define DEFINE_RESULT_TYPE(type)                        \
    struct result_##type {                              \
        enum result_status status;                      \
        union {                                         \
            struct error error;                         \
            type result;                                \
        };                                              \
    };                                                  \
                                                        \
    type result_unwrap_##type(struct result_##type r) { \
        return r.result;                                \
    }                                                   \

#define DEFINE_RESULT_TYPE_PTR(type)                            \
    struct result_##type##_ptr {                                \
        enum result_status status;                              \
        union {                                                 \
            struct error error;                                 \
            type *result;                                       \
        };                                                      \
    };                                                          \
                                                                \
    type *result_unwrap_##type(struct result_##type##_ptr r) {  \
        return r.result;                                        \
    }                                                           \
  
DEFINE_RESULT_TYPE(s8)
DEFINE_RESULT_TYPE(s16)
DEFINE_RESULT_TYPE(s32)
DEFINE_RESULT_TYPE(s64)
DEFINE_RESULT_TYPE(u8)
DEFINE_RESULT_TYPE(u16)
DEFINE_RESULT_TYPE(u32)
DEFINE_RESULT_TYPE(u64)
DEFINE_RESULT_TYPE(f32)
DEFINE_RESULT_TYPE(f64)

DEFINE_RESULT_TYPE_PTR(char)
     
#endif
