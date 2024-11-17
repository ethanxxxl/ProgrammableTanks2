#include "sexp/sexp-base.h"
#include "sexp/sexp-utils.h"
#include "error.h"

#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>

IMPL_RESULT_TYPE_CUSTOM(struct sexp *, sexp)
REFLECT_ENUM(sexp_reader_error_code, READER_ERROR_CODE_ENUM_VALUES)

const struct error_ops SEXP_READER_ERROR_OPS = {
    .describe = describe_sexp_reader_error,
    .free = free_sexp_reader_error,
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
        return RESULT_MSG_ERROR(sexp, "malloc returned NULL");
    }

    (void)car;
    (void)cdr;

    sexp->sexp_type = SEXP_CONS;
    // sexp->cons.car = car;
    // sexp->cons.cdr = cdr;
    return result_sexp_ok(sexp);
}

struct result_sexp make_sexp_integer(s32 num) {
    struct sexp *sexp = malloc(sizeof(struct sexp));
    if (sexp == NULL) {
        return RESULT_MSG_ERROR(sexp, "malloc returned NULL");
    }

    (void)num;

    sexp->sexp_type = SEXP_INTEGER;
    // sexp->integer = num;
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
        return RESULT_MSG_ERROR(sexp, "malloc returned NULL");
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
        struct cons cons = *(struct cons *)sexp->data;

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

    Copies data into sexp if it exists.  If more space is needed, it
    automatically reallocates double the amount required, and updates the
    pointers.

    @return pointer to where the next sexp can be initialized.
*/
struct result_sexp initialize_linear_sexp(struct sexp **root,
                                          size_t *capacity,
                                          struct sexp *dst,
                                          enum sexp_type type,
                                          void *data) {

    if (root == NULL || *root == NULL || dst == NULL || capacity == NULL) {
        return RESULT_MSG_ERROR(sexp, "unexpected NULL value");
    }

    // get length of the data that may be copied to dst.
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
    case SEXP_LINEAR_ROOT:
        data_length = sizeof(struct sexp *);
        break;
    }

    if (data_length > SEXP_MAX_LENGTH) {
        return RESULT_MSG_ERROR(sexp, "sexp length too large");
    }

    // length of the root sexp in bytes
    size_t current_length = (u8 *)dst - (u8 *)*root;

    // ensure there is enough space to copy the data.
    if (current_length + data_length > *capacity) {
        size_t new_capacity = (current_length + data_length) * 2;
        sexp *tmp = realloc(*root, new_capacity);

        if (tmp == NULL)
            return RESULT_MSG_ERROR(sexp, "realloc returned NULL");

        *root = tmp;
        *capacity = new_capacity;

        // since root is updated, dst is invalid and needs updated.
        dst = (sexp *)((u8 *)tmp + current_length);
    }

    // Initialize dst and copy data.
    *dst = (struct sexp) {
        .is_root = true,
        .is_linear = true,
        .sexp_type = type,
        .data_length = data_length,
    };

    if (data != NULL) {
        memcpy(dst->data, data, data_length);
    }

    return result_sexp_ok((struct sexp *)(dst->data + data_length));
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
                             enum sexp_memory_method method,
                             void *data) {
    struct sexp *root;
    
    if (method == SEXP_MEMORY_LINEAR) {
        return RESULT_MSG_ERROR(sexp, "Not Implemented");
        // TODO finish linnear sexp implementation.
        
        // INITIALIZE HANDLE
        root = malloc(sizeof(struct sexp) + sizeof(struct sexp *));

        if (root == NULL) {
            return RESULT_MSG_ERROR(sexp, "malloc returned NULL when creating linear root");
        }

        *root = (struct sexp) {
            .is_linear = true,
            .is_root = true,
            .sexp_type = SEXP_LINEAR_ROOT,
            .data_length = sizeof(struct sexp *),
        };

        // first node a linear sexp has the following metadata:
        //     (ROOT_HANDLE CAPACITY VALUE)
        size_t capacity =
            sexp_linear_size(SEXP_CONS)
            + sexp_linear_size(SEXP_LINEAR_ROOT)
            + sexp_linear_size(SEXP_INTEGER)
            + sexp_linear_size(type)
            + sexp_linear_size(SEXP_LIST_TERMINATOR);

        sexp *value = malloc(capacity);
        if (value == NULL) {
            return RESULT_MSG_ERROR(sexp, "malloc returned NULL when creating linear root value");
        }

        // INITIALIZE ROOT SEXP
        sexp *next = value;

        struct result_sexp r;
        r = initialize_linear_sexp(&value, &capacity, next, SEXP_CONS,  NULL);
        if (r.status == RESULT_OK) next = r.ok; else return r;

        r = initialize_linear_sexp(&value, &capacity, next, SEXP_LINEAR_ROOT,  &root);
        if (r.status == RESULT_OK) next = r.ok; else return r;
        
        r = initialize_linear_sexp(&value, &capacity, next, SEXP_INTEGER,  NULL);
        if (r.status == RESULT_OK) next = r.ok; else return r;

        r = initialize_linear_sexp(&value, &capacity, next, type,  data);
        if (r.status == RESULT_OK) next = r.ok; else return r;
        
        r = initialize_linear_sexp(&value, &capacity, next, SEXP_LIST_TERMINATOR,  NULL);
        if (r.status == RESULT_OK) next = r.ok; else return r;

        *(struct sexp **)root->data = value;

        // sexp *capacity_field = sexp_nth(value, 1);
        // sexp_set_integer(capacity_field, capacity);
        
        return result_sexp_ok(root);
    } else {
        /////////////////////////// CREATE TREE SEXP ///////////////////////////
        size_t data_len;
        switch (type) {
        case SEXP_STRING:
        case SEXP_SYMBOL:
            data_len = strlen(data);
        case SEXP_INTEGER:
            data_len = sizeof(s32);
        case SEXP_CONS:
            data_len = sizeof(struct cons);
        default:
            data_len = 0;
        }

        root = malloc(sizeof(struct sexp) + sizeof(union sexp_data));
        if (root == NULL)
            return RESULT_MSG_ERROR(sexp, "malloc returned NULL");

        if (data != NULL)
            memcpy(root->data, data, data_len);

        return result_sexp_ok(root);
    }
}
