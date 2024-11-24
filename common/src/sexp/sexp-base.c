#include "sexp/sexp-base.h"
#include "sexp/sexp-utils.h"
#include "error.h"

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>

IMPL_RESULT_TYPE_CUSTOM(struct sexp *, sexp)
REFLECT_ENUM(sexp_reader_error_code, READER_ERROR_CODE_ENUM_VALUES)
REFLECT_ENUM(sexp_type, ENUM_SEXP_TYPE_ITEMS)
     
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

void free_sexp_reader_error(void *self) {
    struct sexp_reader_error *err = self;
    free(err->input);
}

struct error sexp_reader_error(enum sexp_reader_error_code code,
                               const char *input, const char *location) {
    struct sexp_reader_error *err = malloc(sizeof(struct sexp_reader_error));
    *err = (struct sexp_reader_error) {
        .code = code,
        .input = 0,
        .location = 0,
    };

    if (input != NULL) {
        size_t len = strlen(input);
        err->input = malloc(len);
        if (err->input == NULL)
            goto return_error;
        
        memcpy(err->input, input, len);
        err->location = err->input + (location - input);
    }

 return_error:
    return (struct error) {
        .self = err,
        .operations = &SEXP_READER_ERROR_OPS
    };
}

char *describe_sexp_reader_error(void *self) {
    struct sexp_reader_error *err = self;

    char *description;
    if (err->input == NULL)
        asprintf(&description, "%s", g_reflected_sexp_reader_error_code[err->code]);
    else
        asprintf(&description, "%s\n%s\n%*c^",
                 g_reflected_sexp_reader_error_code[err->code],
                 err->input,
                 (s32)(err->location - err->input), '~');

    return description;
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

struct result_sexp make_string_sexp(const char *str) {
    return make_sexp(SEXP_STRING, SEXP_MEMORY_TREE, (void *)str);
}
struct result_sexp make_symbol_sexp(const char *sym) {
    return make_sexp(SEXP_SYMBOL, SEXP_MEMORY_TREE, (void *)sym);
}
struct result_sexp make_integer_sexp(s32 num) {
    return make_sexp(SEXP_INTEGER, SEXP_MEMORY_TREE, &num);
}

struct result_sexp make_cons_sexp() {
    sexp *ret;
    RESULT_UNWRAP(sexp, ret, make_sexp(SEXP_CONS, SEXP_MEMORY_TREE, NULL));

    // setcar and setcdr return the sexp that was modified, so setting ret to
    // their values will do nothing.
    RESULT_UNWRAP(sexp, ret, sexp_setcar(ret, NULL));
    RESULT_UNWRAP(sexp, ret, sexp_setcdr(ret, NULL));

    return result_sexp_ok(ret);
}

// TODO implement this
struct sexp *make_tag(void) {
    return NULL;
}

void free_sexp(struct sexp *sexp) {
    if (sexp == NULL)
        return;
    
    // FIXME what if this is a linear sexp?
    if (sexp_type(sexp) == SEXP_CONS) {
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
    case SEXP_NIL:
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
            if (data != NULL)
                data_len = strlen(data) + 1; // account for null terminator
            else
                data_len = sizeof(union sexp_data);
            break;
        default:
            data_len = sizeof(union sexp_data);
        }

        root = malloc(sizeof(struct sexp) + data_len);
        if (root == NULL)
            return RESULT_MSG_ERROR(sexp, "malloc returned NULL");

        root->is_root = false;
        root->is_linear = false;
        root->data_length = data_len;
        root->sexp_type = type;

        // HACK this is mostly for when the data type is a string.  This way, if
        // the string is smaller than sizeof(union sexp_data), there will not be
        // any garbage in the string. This could also probably be accomplished
        // by setting the first byte to zero as well.
        memset(root->data, 0, data_len);
        
        if (data != NULL)
            memcpy(root->data, data, data_len);

        return result_sexp_ok(root);
    }
}
