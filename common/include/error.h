#ifndef ERROR_H
#define ERROR_H

#include "nonstdint.h"

#include <stddef.h>
#include <stdarg.h>

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
const char *describe_msg_error(void *self);
void free_msg_error(void *self);

const struct error_ops MSG_ERROR_OPS = {
    .describe = describe_msg_error,
    .free = free_msg_error,
};

struct error make_msg_error(const char *fmt, ...);
struct error vmake_msg_error(const char *fmt, va_list args);
struct error make_msg_error_with_location(const char *file,
                                          s32 line_num,
                                          const char *fn_name,
                                          const char *fmt, ...);

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
  }                                                                            \
  struct result_##name result_##name##_msg_error(const char *str, ...) {       \
    va_list args;                                                              \
    va_start(args, str);                                                       \
    return (struct result_##name){.status = RESULT_ERROR,                      \
                                  .error = vmake_msg_error(str, args)};        \
  }

#define DEFINE_RESULT_TYPE(type) DEFINE_RESULT_TYPE_CUSTOM(type, type)

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

/****************************** Convience Macros ******************************/

/** Wrapper that automatically inserts location information at call site. */
#define MAKE_MSG_ERROR_WITH_LOCATION(...) \
    make_msg_error_with_location(__FILE__, __LINE__, __func__, __VA_ARGS__)

/** Generates a result type containing a message error that includes file/line
    number context.

    @return result type containing an error message with context that must be
    freed
*/
#define RESULT_MSG_ERROR(name_or_type, ...) \
    result_##name_or_type##_error(MAKE_MSG_ERROR_WITH_LOCATION(__VA_ARGS__))


/** Reduces boilerplate and temp variables when a function will not handle any
    error conditions. */
#define RETURN_ERROR(name_or_type, result, ...)         \
    { struct result_##name_or_type r = __VA_ARGS__;     \
        if (r.status == RESULT_ERROR) return r;         \
        result = r.ok;                                  \
    }
#endif
