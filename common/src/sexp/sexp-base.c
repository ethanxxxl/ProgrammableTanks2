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

/** Helper function to initialize a linear sexp element.

    does not copy any data over.  `data` is only required when `type` is
    `SEXP_STRING` or `SEXP_SYMBOL`.

    @return total length of sexp.  When in a list, the next sexp can be found by
    `dst + initialize_linear_sexp(type, dst, data)`
*/
s32 initialize_linear_sexp(enum sexp_type type,
                            struct sexp *dst,
                            void *data) {
    size_t data_length = 0;

    switch (type) {
    case SEXP_SYMBOL:
    case SEXP_STRING:
        if (data != NULL)
            data_length = strlen(data);
        else
            data_length = 0;
        break;
    case SEXP_INTEGER:
        data_length = sizeof(s32);
        break;
    case SEXP_CONS:
    case SEXP_TAG:
    case SEXP_LIST_TERMINATOR:
        data_length = 0;
        break;
    }

    // FIXME add check for maximum length? is this necessary?

    *dst = (struct sexp) {
        .is_root = true,
        .is_linear = true,
        .sexp_type = type,
        .data_length = data_length,
    };

    return sizeof(struct sexp) + data_length;
}

/** Sets the length of the terminator given a pointer to the parent (sexp with
    type `SEXP_CONS`) and a pointer to the terminator.

    @return length that was written to the terminator, or an error.
*/
struct result_s32 set_terminator_size(const struct sexp *cons,
                                      struct sexp *term) {
    if (cons == NULL)
        return RESULT_MSG_ERROR(s32, "List start sexp is NULL");

    if (term == NULL)
        return RESULT_MSG_ERROR(s32, "List terminator sexp is NULL");

    size_t data_len = ((u8 *)cons - (u8 *)term);

    if (data_len > SEXP_MAX_LENGTH)
        return RESULT_MSG_ERROR(s32, "List terminator size > 0x7FFFFFF!");

    term->data_length = data_len;
    return result_s32_ok(data_len);
}

struct result_sexp make_sexp(enum sexp_type type,
                             enum sexp_memory_method method) {
    struct sexp *sexp;
    
    if (method == SEXP_MEMORY_LINEAR) {
        /* The Linear S-Expression has hidden cons header. The CAR of this
           header contains the capacity of the memory allocated.  The CDR
           contains the actual object specified by the `type` parameter.  Thus,
           the initial size for this buffer must be at least 3 sexp structures
           large. */
        u32 capacity =
            sexp_linear_size(SEXP_CONS) +
            sexp_linear_size(SEXP_INTEGER) +
            sexp_linear_size(type) +
            sexp_linear_size(SEXP_LIST_TERMINATOR);
        
        sexp = malloc(capacity);

        if (sexp == NULL) {
            return RESULT_MSG_ERROR(sexp, "Malloc returned NULL");
        }

        struct sexp *next_sexp = sexp;

        // root sexp
        next_sexp += initialize_linear_sexp(SEXP_CONS, next_sexp, NULL);
        sexp->is_linear = true;
        
        // capacity sexp
        next_sexp += initialize_linear_sexp(SEXP_INTEGER, next_sexp, NULL);

        // user defined sexp
        next_sexp += initialize_linear_sexp(type, next_sexp, NULL);

        // list terminator
        struct sexp *terminator = next_sexp;
        next_sexp += initialize_linear_sexp(SEXP_LIST_TERMINATOR, next_sexp, NULL);

        // set list terminator length
        struct result_s32 r = set_terminator_size(sexp, terminator);
        if (r.status == RESULT_ERROR)
            return result_sexp_error(r.error);

        // set the length in bytes of the root sexp data field.
        sexp->data_length = (u8 *)sexp->data - (u8 *)next_sexp;

        return result_sexp_ok(sexp);
    }

    // TODO copy tree memory implementation method here.
}
