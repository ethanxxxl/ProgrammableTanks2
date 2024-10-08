#include "sexp/sexp-base.h"
#include "error.h"

#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>

const char* G_READER_RESULT_TYPE_STR[] = {
    FOR_EACH_RESULT_TYPE(GENERATE_STRING)
};

struct result_sexp reader_err(enum sexp_reader_error_code code,
                              const char *input,
                              const char* location) {
    return (struct result_sexp){
        .status = RESULT_ERROR,
        .error = sexp_reader_error(code, input, location),
    };
}

/** size of a linear S-Expression structure

    If sexp is a `SEXP_CONS`, `SEXP_STRING`, or `SEXP_SYMBOL`, then the length
    of the sexp passed in are added to the result.
*/
size_t sexp_linear_size(enum sexp_type type) {
    switch (type) {
    case SEXP_CONS:
    case SEXP_SYMBOL:
    case SEXP_STRING:
    default:
        return sizeof(struct sexp);
    case SEXP_INTEGER:
        return sizeof(struct sexp) + sizeof(s32);
    case SEXP_TAG:
        return sizeof(struct sexp);
    }
}

///////////////////////////////// Initializers /////////////////////////////////

struct result_sexp make_sexp_string(const char *str);
struct result_sexp make_sexp_symbol(const char *str);
struct result_sexp make_sexp_integer(s32 num);
struct result_sexp make_sexp_cons(struct sexp *car, struct sexp *cdr);

struct result_sexp make_sexp_cons(struct sexp *car, struct sexp *cdr) {
    struct sexp *sexp = malloc(sizeof(struct sexp));
    if (sexp == NULL) {
        return NULL;
    }
    
    sexp->sexp_type = SEXP_CONS;
    sexp->cons.car = car;
    sexp->cons.cdr = cdr;
    return result_sexp_ok(sexp);
}

struct result_sexp make_sexp_integer(s32 num) {
    struct sexp *sexp = malloc(sizeof(struct sexp));
    if (sexp == NULL) {
        return NULL;
    }

    sexp->sexp_type = SEXP_INTEGER;
    sexp->integer = num;
    return result_sexp_ok(sexp);
}

struct result_sexp make_sexp_symbol(const char *symbol) {
    struct result_sexp r = make_sexp_string(symbol);
    if (r.status == RESULT_ERROR) {
        return r;
    }
    
    struct sexp *sym = r.ok;
    
    sym->sexp_type = SEXP_SYMBOL;
    return result_sexp_ok(sym);
}

struct result_sexp make_sexp_string(const char *str) {
    size_t length = strlen(str);
    struct sexp *sexp = malloc(sizeof(struct sexp) + length);
    if (sexp == NULL) {
        return NULL;
    }

    sexp->sexp_type = SEXP_STRING;
    memcpy(sexp->data, str, length);
    sexp->data_length = length;

    return result_sexp_ok(sexp);
}

// TODO implement this
struct sexp *make_tag(void) {
    return NULL;
}

void free_sexp(struct sexp *sexp) {
    if (sexp->sexp_type == SEXP_CONS) {
        struct cons cons = sexp->cons;

        free(sexp);

        if (cons.car != sexp)
            free_sexp(cons.car);

        if (cons.cdr != sexp)
            free_sexp(cons.cdr);
        return;
    }

    free(sexp);
    return;
}


// TODO consider splitting this off into two functions, one that creates a
// linear SEXP, another that creates a tree SEXP.

struct result_sexp make_sexp(enum sexp_type type,
                             enum sexp_memory_method method) {
    struct sexp *sexp;
    
    if (method == SEXP_MEMORY_LINEAR) {
        /* The Linear S-Expression has hidden cons header. The CAR of this
           header contains the capacity of the memory allocated.  The CDR
           contains the actual object specified by the `type` parameter.  Thus,
           the initial size for this buffer must be at least 3 sexp structures
           large. */
        u32 capacity = sizeof(struct sexp) * 3;
        sexp = malloc(capacity);

        if (sexp == NULL) {
            // return result_sexp_msg_error("Malloc returned NULL\n" PROGRAM_CONTEXT_STR);
        }

        // initialize CAR
        struct sexp *car = (struct sexp *)sexp->data;
        *car = (struct sexp) {
            .is_linear = true,
            .is_root = false,
            .sexp_type = SEXP_INTEGER,
            .data_length = sizeof(s32),
        };
        *(u32 *)sexp->data = capacity;
        u32 car_size = sizeof(struct sexp) + car->data_length;
        
        // initialize CDR
        struct sexp *cdr = (struct sexp *)(car->data + car->data_length);
        *cdr = (struct sexp) {
            .is_linear = true,
            .is_root = false,
            .sexp_type = type,
            .data_length = sexp_linear_size(SEXP_INTEGER),
        };

        // TODO finish me!!!
        *sexp = (struct sexp) {
            .is_linear = true,
            .is_root = true,
            .sexp_type = SEXP_CONS,
            .data_length = sizeof(struct sexp) * 2,
        };

        struct sexp *capacity_sexp = sexp + sizeof(struct sexp);
        *capacity_sexp = (struct sexp) {
            .is_linear = true,
            .is_root = false,
            .sexp_type = SEXP_INTEGER,
            .data_length = sizeof(s32),

            .data = *()
        }
    }
}
