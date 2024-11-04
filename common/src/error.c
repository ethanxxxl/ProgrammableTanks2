#include "error.h"

#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

/***************************** Error Object Type ******************************/
const char *describe_error(const struct error e) {
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

const char *describe_msg_error(void *self) {
    return (char*)self;
}

void free_msg_error(void *self) {
    // if malloc failed while creating this error, self will not be heap memory,
    // which will likely cause a
    free((char *)self);
}

struct error make_msg_error(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    return vmake_msg_error(fmt, args);
}

struct error vmake_msg_error(const char *fmt, va_list args) {
    char *message;
    vasprintf(&message, fmt, args); // calls malloc

    if (message == NULL) {
        message =
            "ERROR: malloc returned NULL while trying to return an error!";
    }

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
