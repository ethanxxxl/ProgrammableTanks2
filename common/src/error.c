#include "error.h"

#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

/************************* Generic Result Type Impls **************************/
IMPL_RESULT_TYPE(s8)
IMPL_RESULT_TYPE(s16)
IMPL_RESULT_TYPE(s32)
IMPL_RESULT_TYPE(s64)
IMPL_RESULT_TYPE(u8)
IMPL_RESULT_TYPE(u16)
IMPL_RESULT_TYPE(u32)
IMPL_RESULT_TYPE(u64)
IMPL_RESULT_TYPE(f32)
IMPL_RESULT_TYPE(f64)
IMPL_RESULT_TYPE_CUSTOM(char *, str)
IMPL_RESULT_TYPE_CUSTOM(void *, voidp)
IMPL_RESULT_TYPE_CUSTOM(u8, void)

/***************************** Error Object Type ******************************/
/** A printable representation of the error received.  This must be freed.*/
char *describe_error(const struct error e) {
    if (e.operations->describe != NULL)
        return e.operations->describe(e.self);
    else
        return NULL;
}

void free_error(const struct error e) {
    if (e.operations->free != NULL)
        e.operations->free(e.self);
}


/******************************* Generic Error ********************************/

char *describe_msg_error(void *self) {
    size_t len = strlen(self);
    char *new_str = malloc(len);
    memcpy(new_str, self, len);

    return new_str;
}

void free_msg_error(void *self) {
    // if malloc failed while creating this error, self will not be heap memory,
    // which will likely cause a
    free(self);
}

struct error make_msg_error(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    return vmake_msg_error(fmt, args);
}

const struct error_ops MSG_ERROR_OPS = {
    .describe = describe_msg_error,
    .free = free_msg_error,
};

struct error vmake_msg_error(const char *fmt, va_list args) {
    char *message;
    vasprintf(&message, fmt, args); // calls malloc

    // if vasprintf returns NULL, message cannot return a string literal,
    // because the program would try to free it.

    // the best thing to do is to warn the user about it when it occures.
    if (message == NULL)
        printf("CRITICAL ERROR: malloc returned null while creating an error!");

    return (struct error) {
        .operations = &MSG_ERROR_OPS,
        .self = message,
    };    
}

struct error make_msg_error_with_location(const char *file,
                                          s32 line_num,
                                          const char *fn_name,
                                          const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    
    char *message;
    vasprintf(&message, fmt, args); // calls malloc, will be free'ed at end.

    if (message == NULL) {
        message =
            "ERROR: malloc returned NULL while trying toreturn an error!";
    }

    return make_msg_error("%s\nfile: %s\nline: %d\nfunc: %s\n",
                          message,
                          file,
                          line_num,
                          fn_name);
}
