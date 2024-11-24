#include "sexp/sexp-utils.h"
#include "error.h"
#include "sexp/sexp-base.h"

#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

        
// FIXME create compound errors. If there are multiple errors, return
// them all, not just the first one that is encountered.
struct result_sexp sexp_list(struct result_sexp first, ...) {
    va_list args;
    va_start(args, first);

    // an error either in first or list initialization will be handled in the
    // first iteration of the loop.
    struct result_sexp root = sexp_rsetcar(make_cons_sexp(), first);
    struct result_sexp current_element = root;

    struct result_sexp item;
    while (item = va_arg(args, struct result_sexp),
           sexp_is_nil(item.ok) == false) {

        current_element = sexp_rpush(current_element, item);
        if (current_element.status == RESULT_ERROR) 
            goto error_occured;
    }

    va_end(args);
    return root;

 error_occured:
    va_end(args);

    if (root.status == RESULT_OK)
        free_sexp(root.ok);
    return current_element;
}

struct result_sexp sexp_setcar(sexp *dst, sexp *car) {
    if (sexp_is_nil(dst))
        return RESULT_MSG_ERROR(sexp, "NIL is not a valid destination");

    if (dst->is_linear == true)
        return RESULT_MSG_ERROR(sexp, "Not Implemented for linear sexp");
    
    if (dst->sexp_type != SEXP_CONS)
        return RESULT_MSG_ERROR(sexp, "dst is not a cons");

    ((union sexp_data *)dst->data)->cons.car = car;

    return result_sexp_ok(dst);
}
struct result_sexp sexp_rsetcar(struct result_sexp dst, struct result_sexp car) {
    // FIXME what if they are both errors? this could be a memory leak.
    if (dst.status == RESULT_ERROR && car.status == RESULT_ERROR)
        // combine errors somehow?
        return RESULT_MSG_ERROR(sexp, "error in both dst and car! potential memory leak!");

    if (dst.status == RESULT_ERROR)
        return dst;
    if (car.status == RESULT_ERROR)
        return car;

    return sexp_setcar(dst.ok, car.ok);
}

struct result_sexp sexp_setcdr(sexp *dst, sexp *cdr) {
    if (sexp_is_nil(dst) || dst->sexp_type != SEXP_CONS)
        return RESULT_MSG_ERROR(sexp, "dst is %s, not a %s");

    if (dst->is_linear == true)
        return RESULT_MSG_ERROR(sexp, "Not Implmented for linear sexp");
        
    ((union sexp_data *)dst->data)->cons.cdr = cdr;

    return result_sexp_ok(dst);
}
struct result_sexp sexp_rsetcdr(struct result_sexp dst, struct result_sexp cdr) {
    // FIXME what if they are both errors? this could be a memory leak.
    if (dst.status == RESULT_ERROR && cdr.status == RESULT_ERROR)
        // combine errors somehow?
        return RESULT_MSG_ERROR(sexp, "error in both dst and cdr! potential memory leak!");

    if (dst.status == RESULT_ERROR)
        return dst;
    if (cdr.status == RESULT_ERROR)
        return cdr;

    return sexp_setcar(dst.ok, cdr.ok);
}

struct result_sexp sexp_car(const sexp *sexp) {
    if (sexp_is_nil(sexp))
        return sexp_nil();
    
    if (sexp->is_linear == true)
        return RESULT_MSG_ERROR(sexp, "Not Implemented for linear sexp");

    if (sexp->sexp_type != SEXP_CONS)
        return RESULT_MSG_ERROR(sexp, "sexp is not a cons");

    return result_sexp_ok(((union sexp_data *)sexp->data)->cons.car);
}

struct result_sexp sexp_rcar(struct result_sexp sexp) {
    if (sexp.status == RESULT_ERROR)
        return sexp;

    return sexp_car(sexp.ok);
}

struct result_sexp sexp_cdr(const sexp *sexp) {
    if (sexp_is_nil(sexp))
        return sexp_nil();

    if (sexp->is_linear == true)
        return RESULT_MSG_ERROR(sexp, "Not Implemented for linear sexp");

    if (sexp->sexp_type != SEXP_CONS)
        return RESULT_MSG_ERROR(sexp, "sexp is not a cons");

    return result_sexp_ok(((union sexp_data *)sexp->data)->cons.cdr);
}

struct result_sexp sexp_rcdr(struct result_sexp sexp) {
    if (sexp.status == RESULT_ERROR)
        return sexp;

    return sexp_cdr(sexp.ok);
}

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
    if (sexp_is_nil(s) || s->sexp_type != SEXP_CONS)
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
    sexp *end;
    RESULT_UNWRAP(sexp, end, sexp_last(list));

    enum sexp_memory_method mem_method = list->is_linear ?
        SEXP_MEMORY_LINEAR : SEXP_MEMORY_TREE;

    sexp *new_end;
    RESULT_UNWRAP(sexp, new_end, make_sexp(SEXP_CONS, mem_method, NULL));

    RESULT_CALL(sexp, sexp_setcdr(end, new_end));

    sexp *new_item;
    RESULT_UNWRAP(sexp, new_item, make_sexp(type, mem_method, data));

    RESULT_CALL(sexp, sexp_setcar(end, new_item));
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

struct result_sexp sexp_push_tag(sexp *list) {
    (void)list;
    return RESULT_MSG_ERROR(sexp, "Not Implemented");
}


struct result_sexp sexp_tag_get_tag(const sexp *s) {
    (void)s;
    return RESULT_MSG_ERROR(sexp, "not implemented");
}

struct result_sexp sexp_tag_get_atom(const sexp *s) {
    (void)s;
    return RESULT_MSG_ERROR(sexp, "not implemented");
}


struct result_sexp sexp_push(sexp *list, sexp *item) {
    if (list->is_linear || item->is_linear)
        return RESULT_MSG_ERROR(sexp, "Not Implemented for linear sexps");

    sexp *old_end;
    // add a new element to end of the list.  The original end will be returned.
    RESULT_UNWRAP(sexp, old_end,
                  sexp_rsetcdr(sexp_last(list), make_cons_sexp()));

    return sexp_rsetcar(sexp_last(old_end), result_sexp_ok(item));

}

struct result_sexp sexp_rpush(struct result_sexp list, struct result_sexp item) {
    if (list.status == RESULT_ERROR && item.status == RESULT_ERROR)
        return RESULT_MSG_ERROR(sexp, "both list and item are errors, potential memory leak!");

    if (list.status == RESULT_ERROR)
        return list;

    if (item.status == RESULT_ERROR)
        return item;

    return sexp_push(list.ok, item.ok);
}

struct result_sexp sexp_nil() {
    return result_sexp_ok(NULL);
}
