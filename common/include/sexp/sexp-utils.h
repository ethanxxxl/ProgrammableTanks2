#ifndef SEXP_UTILS_H
#define SEXP_UTILS_H

#include "sexp/sexp-base.h"

#include <stdbool.h>

/* Many of the utility functions in this file have a corresponding "result"
   form, which accepts result types as parameters rather than raw types.  Since
   most functions return result types anyway, it can be more convenient to use
   chainging, if a function receives an error as a parameter, it simply returns
   that error.
*/

///////////////////////////////// Manipulators /////////////////////////////////

/** Returns a list created by the sexps specified as arguments.

 For convienience, the sexps are wrapped in the result types, functions that
 return result_sexp types can be used directly.  If an error is passed into the
 function, that error will be returned.
*/
struct result_sexp sexp_list(struct result_sexp sexp, ...);

struct result_sexp sexp_setcar(sexp *dst, sexp *car);
struct result_sexp sexp_rsetcar(struct result_sexp dst, struct result_sexp car);

struct result_sexp  sexp_setcdr(sexp *dst, sexp *cdr);
struct result_sexp sexp_rsetcdr(struct result_sexp dst, struct result_sexp cdr);

struct result_sexp sexp_car(const sexp *sexp);
struct result_sexp sexp_rcar(struct result_sexp sexp);

struct result_sexp sexp_cdr(const sexp *sexp);
struct result_sexp sexp_rcdr(struct result_sexp sexp);


bool sexp_is_nil(const sexp *sexp);
bool sexp_ris_nil(struct result_sexp sexp);

/** Concatenates list1 and list2 by copying their elements into a new list.

    ie (append '(a b c) '(d e f)) => (a b c d e f)

    This function does not free `list1` or `list2`.  It is the responsibility of
    the programmer to free unused lists.

    both `list1` and `list2` must have the same memory layout

    @param list1 sexp with type SEXP_CONS
    @param list2 sexp with type SEXP_CONS
    @return a newly allocated list.
 */
struct result_sexp sexp_append(sexp *list1, sexp *list2);
struct result_sexp sexp_rappend(struct result_sexp list1, struct result_sexp list2);

/** Concatenates list1 and list2 by modifying their elements.
    
    ie (append '(a b c) '(d e f)) => (a b c d e f)

    It is not safe to use `list1` or `list2` after this function is called.
    These pointers may have been free'ed or realloc'ed.

    both `list1` and `list2` must have the same memory layout

    @param list1 sexp with type SEXP_CONS
    @param list2 sexp with type SEXP_CONS
    @return sexp_cons containing the contatenation of `list1` and `list2`
 */
struct result_sexp sexp_nconc(sexp *list1, sexp *list2);
struct result_sexp sexp_rnconc(struct result_sexp list1, struct result_sexp list2);

/* returns the number of elements in the sexp list. */
struct result_u32 sexp_length(const sexp *sexp);
struct result_u32 sexp_rlength(struct result_sexp sexp);

/** Return the nth element (car) of the list. */
struct result_sexp sexp_nth(const sexp *list, size_t n);

/** Create a new integer S-Expression and put it at the end of list

    @return pointer to newly created sexp at end of list or error.
 */
struct result_sexp sexp_push_integer(sexp *list, s32 num);
struct result_sexp sexp_rpush_integer(struct result_sexp list, struct result_s32 num);

/** Create a new string S-Expression and put it at the end of list

    @return pointer to newly created sexp at end of list or error.
*/
struct result_sexp sexp_push_string(sexp *list, const char *str);
struct result_sexp sexp_rpush_string(struct result_sexp list, struct result_str str);

/** Create a new symbol S-Expression and put it at the end of list

    @return pointer to newly created sexp at end of list or error.
*/
struct result_sexp sexp_push_symbol(sexp *list, const char *str);
struct result_sexp sexp_rpush_symbol(struct result_sexp list, struct result_str str);

/** Create an empty tag container and put it at the end of list

    @return pointer to newly created sexp at end of list or error.
 */
struct result_sexp sexp_push_tag(sexp *list);
struct result_sexp sexp_rpush_tag(struct result_sexp list);

/** Create a nil sexp (empty list) and put it at the end of the list

    @return pointer to newly created sexp at end of list or error.
 */
struct result_sexp sexp_push_nil(sexp *list);
struct result_sexp sexp_rpush_nil(struct result_sexp list);

/** Create a generic sexp (empty list) and put it at the end of the list

    @return pointer to newly created sexp at end of list or error.
 */
struct result_sexp sexp_push(sexp *list, sexp *item);
struct result_sexp sexp_rpush(struct result_sexp list, struct result_sexp item);

/** The following three return the value enclosed by the S expression, if it
    encloses that type. */
struct result_s32 sexp_int_val(const sexp *s);
struct result_s32 sexp_rint_val(struct result_sexp s);

struct result_str sexp_str_val(const sexp *s);
struct result_str sexp_rstr_val(struct result_sexp s);

struct result_str sexp_sym_val(const sexp *s);
struct result_str sexp_rsym_val(struct result_sexp s);

const sexp *sexp_find(const sexp *s, char *atom);

void sexp_set_integer(sexp *s, s32 i);
void sexp_set_string(sexp *s, const char *str);
void sexp_set_symbol(sexp *s, const char *sym);

struct result_sexp sexp_tag_get_tag(const sexp *s);
struct result_sexp sexp_tag_get_atom(const sexp *s);

#endif
