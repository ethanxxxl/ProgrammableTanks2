#ifndef RESULT_H
#define RESULT_H

#include "nonstdint.h"

enum status {
    ok,
    fail
};

struct result {
    enum status status;
    union {
        const char *error_message;
        void *result;
    };
};

#endif
