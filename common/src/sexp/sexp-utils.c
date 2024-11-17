#include "sexp/sexp-utils.h"
#include "error.h"
#include "sexp/sexp-base.h"

#include <ctype.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

struct result_s32 sexp_int_val(const sexp *s) {
    if (sexp_is_nil(s) || s->sexp_type != SEXP_INTEGER)
        return RESULT_MSG_ERROR(s32, "sexp is not SEXP_INTEGER");

    return result_s32_ok(((const union sexp_data *)(s->data))->integer);
}
struct result_str sexp_str_val(const sexp *s) {
    if (sexp_is_nil(s) || s->sexp_type != SEXP_STRING)
        return RESULT_MSG_ERROR(str, "sexp is not SEXP_STRING");

    return result_str_ok((char *)s->data);
}
struct result_str sexp_sym_val(const sexp *s) {
    if (sexp_is_nil(s) || s->sexp_type != SEXP_SYMBOL)
        return RESULT_MSG_ERROR(str, "sexp is not SEXP_SYMBOL");

    return result_str_ok((char *)s->data);
}

bool sexp_is_nil(const sexp *s) {
    if (s == NULL)
        return true;

    if (s->sexp_type != SEXP_CONS)
        return false;

    struct result_sexp r = sexp_cdr(s);
    if (r.status != RESULT_OK) {
        // the check has already been done, this branch should not be entered.
        printf("WARNING! recieved error in sexp_is_nil: %s\n",
               describe_error(r.error));
        free_error(r.error);
        return false;
    }
        
    sexp *cdr = r.ok;

    if (cdr == s)
        return true;

    return false;
}

struct result_sexp
sexp_nth(const sexp *s, size_t n) {
    if (s->sexp_type != SEXP_CONS)
        return RESULT_MSG_ERROR(sexp, "Type is not SEXP_CONS");
    
    if (sexp_is_nil(s))
        return result_sexp_ok(NULL);

    struct result_sexp r;
    const sexp *ret = s;
    for (size_t i = 0; i < n; i++) {
        r = sexp_cdr(ret);
        if (r.status == RESULT_ERROR) return r;
        
        ret = r.ok;
    }

    return sexp_car(ret);
}

struct result_sexp
sexp_last(sexp *s) {
    if (s->sexp_type != SEXP_CONS)
        return RESULT_MSG_ERROR(sexp, "Type is not SEXP_CONS");
    
    if (sexp_is_nil(s))
        return result_sexp_ok(NULL);

    sexp *last = s;
    while (sexp_is_nil(last) == false) {
        struct result_sexp r = sexp_cdr(last);
        if (r.status == RESULT_ERROR) return r;
        last = r.ok;
    }

    return result_sexp_ok(last);
}

struct result_sexp
sexp_append(sexp *list1, sexp *list2) {
    if (list1->is_linear != list2->is_linear)
        return RESULT_MSG_ERROR(sexp, "list1 and list2 have different memory layouts");

    enum sexp_type t1 = list1->sexp_type;
    enum sexp_type t2 = list2->sexp_type;
    char *msg_part = NULL;
    if (t1 != SEXP_CONS && t2 != SEXP_CONS)
        msg_part = "list1 and list2 are";
    else if (t1 != SEXP_CONS)
        msg_part = "list1 is";
    else if (t2 != SEXP_CONS)
        msg_part = "list2 is";

    if (msg_part != NULL) {
        return RESULT_MSG_ERROR(sexp, "%s not SEXP_CONS", msg_part);
    }

    sexp *ret;
    struct result_sexp r;

    r = make_sexp(SEXP_CONS,
                  list1->is_linear ? SEXP_MEMORY_LINEAR : SEXP_MEMORY_TREE,
                  NULL);

    if (r.status == RESULT_ERROR)
        return r;

    ret = result_unwrap_sexp(r);

    if (ret->is_linear == false) {
        // generic method to do the append operation
        
        sexp *end = ret;
        for (s32 x = 0; x <= 1; x++) {
            sexp *lists[] = {list1, list2};

            sexp *i = lists[x];
            while (sexp_is_nil(i) == false) {
                r = sexp_car(i);
                if (r.status == RESULT_ERROR) return r;
                sexp *car = r.ok;

                sexp *new_car;
                r = make_sexp(car->sexp_type,
                              SEXP_MEMORY_TREE,
                              car->data);
                if (r.status == RESULT_ERROR)
                    return r;

                new_car = r.ok;
                sexp_setcar(end, new_car);

                sexp *new_cdr;
                r = make_sexp(SEXP_CONS, SEXP_MEMORY_TREE, NULL);
                if (r.status == RESULT_ERROR)
                    return r;

                new_cdr = r.ok;
                sexp_setcdr(end, new_cdr);
                
                end = new_cdr;
                r = sexp_cdr(i);
                if (r.status == RESULT_ERROR) return r;
                
                i = r.ok;
            }
        }

        return result_sexp_ok(ret);
    } else {
        // optimized method for linear memory layout.
        return RESULT_MSG_ERROR(sexp, "not implemented");
    }
}

struct result_sexp sexp_nconc(sexp *list1, sexp *list2) {
    (void)list1;
    (void)list2;

    return RESULT_MSG_ERROR(sexp, "not implemented");
}

struct result_u32
sexp_length(const sexp *s) {
    if (s->sexp_type != SEXP_CONS)
        return RESULT_MSG_ERROR(u32, "incorrect type");

    struct result_sexp r;
    u32 n = 0;
    for (const sexp *elem = s; !sexp_is_nil(elem); n++) {
        r = sexp_cdr(elem);
        if (r.status == RESULT_ERROR) return result_u32_error(r.error);

        elem = r.ok;
    }

    return result_u32_ok(n);
}

struct result_sexp _sexp_push_data(sexp *list, enum sexp_type type, void* data) {
    struct result_sexp r = sexp_last(list);
    if (r.status == RESULT_ERROR)
        return r;
    
    sexp *end = r.ok;

    enum sexp_memory_method mem_method = list->is_linear ?
        SEXP_MEMORY_LINEAR : SEXP_MEMORY_TREE;
    
    r = make_sexp(SEXP_CONS, mem_method, NULL);
    if (r.status == RESULT_ERROR)
        return r;

    sexp_setcdr(end, r.ok);
    end = r.ok;

    r = make_sexp(type, mem_method, data);
    if (r.status == RESULT_ERROR)
        return r;

    sexp_setcar(end, r.ok);
    return result_sexp_ok(end);
}

struct result_sexp sexp_push_integer(sexp *list, s32 num) {
    return _sexp_push_data(list, SEXP_INTEGER, &num);
}

struct result_sexp sexp_push_string(sexp *list, const char *str) {
    return _sexp_push_data(list, SEXP_STRING, (void*)str);
}

struct result_sexp sexp_push_symbol(sexp *list, const char *sym) {
    return _sexp_push_data(list, SEXP_SYMBOL, (void*)sym);
}

struct result_sexp sexp_tag_get_tag(const sexp *s) {
    (void)s;
    return RESULT_MSG_ERROR(sexp, "not implemented");
}

struct result_sexp sexp_tag_get_atom(const sexp *s) {
    (void)s;
    return RESULT_MSG_ERROR(sexp, "not implemented");
}

