#ifndef ERROR_H
#define ERROR_H

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
const char *describe_generic_error(void *self);
void free_generic_error(void *self);

const struct error_ops GENERIC_ERROR_OPS = {
    .describe = describe_generic_error,
    .free = free_generic_error,
};

struct error generic_error(const char *error_message);

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

#define DEFINE_RESULT_TYPE(type)                                               \
  struct result_##type {                                                       \
    enum result_status status;                                                 \
    union {                                                                    \
      struct error error;                                                      \
      type ok;                                                                 \
    };                                                                         \
  };                                                                           \
                                                                               \
  type result_unwrap_##type(struct result_##type r) { return r.ok; }           \
  struct result_##type result_##type##_ok(type t) {                            \
    return (struct result_##type){.status = RESULT_OK, .ok = t};               \
  }                                                                            \
  struct result_##type resutl_##type##_error(struct error e) {                 \
    return (struct result_##type){.status = RESULT_ERROR, .error = e};         \
  }

#define DEFINE_RESULT_TYPE_CUSTOM(type, name)                                  \
  struct result_##name {                                                       \
    enum result_status status;                                                 \
    union {                                                                    \
      struct error error;                                                      \
      type ok;                                                                 \
    };                                                                         \
  };                                                                           \
                                                                               \
  type result_unwrap_##name(struct result_##name r) { return r.ok; }           \
                                                                               \
  struct result_##name result_##name##_ok(type t) {                            \
    return (struct result_##name){.status = RESULT_OK, .ok = t};               \
  }                                                                            \
  struct result_##name result_##name##_error(struct error e) {                 \
    return (struct result_##name){.status = RESULT_ERROR, .error = e};         \
  }

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

DEFINE_RESULT_TYPE_CUSTOM(char *, str)

#endif
