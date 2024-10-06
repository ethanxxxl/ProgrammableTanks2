#include "sexp/sexp-utils.h"

#include <ctype.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

const struct sexp*
sexp_nth(const struct sexp* sexp, size_t n) {
    if (sexp == NULL || sexp->type != SEXP_CONS || sexp->length <= 0)
        return NULL;

    struct sexp* element = (struct sexp*)sexp->data;
    for (size_t i = 0; i < n; i++) {
        element = (struct sexp*)(element->data + element->length);
        if ((void*)element >= (void*)(sexp->data + sexp->length))
            return NULL;
    }

    return element;
}

struct sexp*
sexp_append(struct sexp* dst, const struct sexp* src) {
    if (dst == NULL || src == NULL || dst->type != SEXP_CONS)
        return NULL;

    struct sexp* element = (struct sexp*)(dst->data + dst->length);
    memcpy(element, src, sizeof(struct sexp) + src->length);

    dst->length += sizeof(struct sexp) + src->length;

    return element;
}

struct sexp*
sexp_append_dat(struct sexp* dst, void* dat, size_t len, enum sexp_type type) {
    if (dst == NULL || dst->type != SEXP_CONS)
        return NULL;

    if (type != SEXP_CONS && dat == NULL)
        return NULL;

    if (type == SEXP_INTEGER && len != sizeof(u32)) {
        printf("ERROR: SEXP unsupported integer size: %zu", len);
        return NULL;
    }

    struct sexp* element = (struct sexp*)(dst->data + dst->length);

    if (type != SEXP_CONS)
        memcpy(element->data, dat, len);

    element->length = len;
    element->type = type;

    dst->length += sizeof(struct sexp) + len;

    return element;
}

size_t
sexp_length(const struct sexp* sexp) {
    if (sexp == NULL || sexp->type != SEXP_CONS)
        return 0;

    
    struct sexp* element = (struct sexp*)(sexp->data);

    size_t n = 0;
    while (element < (struct sexp*)(sexp->data + sexp->length)) {
        n++;
        element = (struct sexp*)(element->data + element->length);
    }

    return n;
}


/*************************** Malloc Implementation ****************************/

struct sexp_dyn *sexp_to_dyn(const struct sexp *sexp) {
    switch (sexp->type) {
    case SEXP_CONS: {
        struct sexp_dyn *parent_cons = make_cons(NULL, NULL);
        struct cons *cons = &parent_cons->cons;

        // fill in list
        for (u32 n = 0; n < sexp_length(sexp); n++) {
            cons->car = sexp_to_dyn(sexp_nth(sexp, n));
            cons->cdr = make_cons(NULL, NULL);

            // go to next element in the list.
            cons = &cons->cdr->cons;
        }

        // terminate list with NIL
        struct sexp_dyn *nil = make_cons(NULL, NULL);
        nil->cons.car = nil;
        nil->cons.cdr = nil;
        
        cons->cdr = nil;

        return parent_cons;
    }
    case SEXP_INTEGER:
        return make_integer(*(u32*)sexp->data);
    case SEXP_STRING: 
        return make_string((char *)sexp->data);
    case SEXP_SYMBOL:
        return make_symbol((char *)sexp->data);
    case SEXP_TAG:
        return make_tag();
    }
}

size_t get_static_space(struct sexp_dyn *sexp);

// TODO fill this out.
struct sexp_static *sexp_from_dyn(struct sexp_dyn *sexp, u8 *buf, size_t len) {
    switch (sexp->type) {
    case SEXP_CONS: {
        // iterate through each cons cell in the list, 
    }
    case SEXP_INTEGER:
    case SEXP_STRING:
    case SEXP_TAG:
    }
}


/* TODO add proper error handling */
struct sexp_dyn *sexp_dyn_read(char *str) {
    // READ IN THE SEXP
    struct reader_result result = sexp_read(str, NULL, true);
    if (result.status != RESULT_OK) {
        printf("ERROR: received following sexp_error: %s",
               G_READER_RESULT_TYPE_STR[result.status]);
        return NULL;
    }

    u8 sexp_buffer[result.length];
    struct sexp *sexp = (struct sexp *)sexp_buffer;
    sexp_read(str, sexp, false);

    // COPY OVER AND MAKE DYNAMIC
    return sexp_to_dyn(sexp);
}

struct sexp_dyn *make_cons(struct sexp_dyn *car, struct sexp_dyn *cdr) {
    struct sexp_dyn *sexp = malloc(sizeof(struct sexp_dyn));
    if (sexp == NULL) {
        return NULL;
    }
    
    sexp->type = SEXP_CONS;
    sexp->cons.car = car;
    sexp->cons.cdr = cdr;
    return sexp;
}

struct sexp_dyn *make_integer(s32 num) {
    struct sexp_dyn *sexp = malloc(sizeof(struct sexp_dyn));
    if (sexp == NULL) {
        return NULL;
    }

    sexp->type = SEXP_INTEGER;
    sexp->integer = num;
    return sexp;
}

struct sexp_dyn *make_symbol(char *symbol) {
    struct sexp_dyn *sym = make_string(symbol);
    sym->type = SEXP_SYMBOL;
    return sym;
}

struct sexp_dyn *make_string(char *str) {
    size_t length = strlen(str);
    struct sexp_dyn *sexp = malloc(sizeof(struct sexp_dyn) + length);
    if (sexp == NULL) {
        return NULL;
    }

    sexp->type = SEXP_STRING;
    memcpy(sexp->text, str, length);
    sexp->text_len = length;

    return sexp;
}

// TODO implement this
struct sexp_dyn *make_tag() {
    return NULL;
}

void free_sexp_dyn(struct sexp_dyn *sexp) {
    if (sexp->type == SEXP_CONS) {
        struct cons cons = sexp->cons;

        free(sexp);

        if (cons.car != sexp)
            free_sexp_dyn(cons.car);

        if (cons.cdr != sexp)
            free_sexp_dyn(cons.cdr);
        return;
    }

    free(sexp);
    return;
}
