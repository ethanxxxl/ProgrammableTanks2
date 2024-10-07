#include "error.h"

#include <string.h>
#include <stddef.h>
#include <stdlib.h>

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

const char *describe_generic_error(void *self) {
    return (char*)self;
}

void free_generic_error(void *self) {
    // if malloc failed while creating this error, self will not be heap memory,
    // which will likely cause a
    free((char *)self);
}

struct error generic_error(const char* error_message) {
    size_t msg_len = strlen(error_message);
    char *copied_message = malloc(sizeof(char) * msg_len);

    if (copied_message == NULL) {
        // yikes, not good
        copied_message =
            "ERROR: malloc returned NULL while trying to return an error!";
    }

    return (struct error) {
        .operations = &GENERIC_ERROR_OPS,
        .self = copied_message,
    };
}
