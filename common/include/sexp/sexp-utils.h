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

/******************************* MISC LIST OPS ********************************/
/** Returns a list created by the sexps specified as arguments.

    For convienience, the sexps are wrapped in the result types, functions that
    return result_sexp types can be used directly.  If an error is passed into
    the function, that error will be returned.

    IMPORTANT: LAST ELEMENT MUST BE NIL (NULL qualifies as nill)
*/
struct result_sexp sexp_list(struct result_sexp sexp, ...);

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

/** Concatenates list1 and list2 by copying their elements into a new list.,
    unless a parameter is an error.

    ie (append '(a b c) '(d e f)) => (a b c d e f)

    This function does not free `list1` or `list2`.  It is the responsibility of
    the programmer to free unused lists.

    both `list1` and `list2` must have the same memory layout

    @param list1 sexp with type SEXP_CONS
    @param list2 sexp with type SEXP_CONS
    @return a newly allocated list.
 */
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

/** Concatenates list1 and list2 by modifying their elements, unless a
    parameter is an error.
    
    ie (append '(a b c) '(d e f)) => (a b c d e f)

    It is not safe to use `list1` or `list2` after this function is called.
    These pointers may have been free'ed or realloc'ed.

    both `list1` and `list2` must have the same memory layout

    @param list1 sexp with type SEXP_CONS
    @param list2 sexp with type SEXP_CONS
    @return sexp_cons containing the contatenation of `list1` and `list2`
 */
struct result_sexp sexp_rnconc(struct result_sexp list1, struct result_sexp list2);

/** returns the number of elements in the sexp list. */
struct result_u32 sexp_length(const sexp *sexp);

/** returns the number of elements in the sexp list, unless a parameter is an
    error. */
struct result_u32 sexp_rlength(struct result_sexp sexp);

/** Return the nth element (car) of the list. */
struct result_sexp sexp_nth(const sexp *list, size_t n);

/** Return the last cons cell of list */
struct result_sexp sexp_last(sexp *list);

/** Return the last cons cell of list */
struct result_sexp sexp_rlast(struct result_sexp list);


/******************************** LIST PUSHERS ********************************/
/** Create a new integer S-Expression and put it at the end of list

    @return pointer to newly created sexp at end of list or error.
 */
struct result_sexp sexp_push_integer(sexp *list, s32 num);

/** Create a new integer S-Expression and put it at the end of list, unless a
    parameter is an error.

    @return pointer to newly created sexp at end of list or error.
 */
struct result_sexp sexp_rpush_integer(struct result_sexp list, struct result_s32 num);

/** Create a new string S-Expression and put it at the end of list

    @return pointer to newly created sexp at end of list or error.
*/
struct result_sexp sexp_push_string(sexp *list, const char *str);

/** Create a new string S-Expression and put it at the end of list, unless a
    parameter is an error.

    @return pointer to newly created sexp at end of list or error.
*/
struct result_sexp sexp_rpush_string(struct result_sexp list, struct result_str str);

/** Create a new symbol S-Expression and put it at the end of list

    @return pointer to newly created sexp at end of list or error.
*/
struct result_sexp sexp_push_symbol(sexp *list, const char *str);

/** Create a new symbol S-Expression and put it at the end of list, unless a
    parameter is an error.

    @return pointer to newly created sexp at end of list or error.
*/
struct result_sexp sexp_rpush_symbol(struct result_sexp list, struct result_str str);

/** Create an empty tag container and put it at the end of list

    @return pointer to newly created sexp at end of list or error.
 */
struct result_sexp sexp_push_tag(sexp *list);

/** Create an empty tag container and put it at the end of list, unless a
    parameter is an error.

    @return pointer to newly created sexp at end of list or error.
 */
struct result_sexp sexp_rpush_tag(struct result_sexp list);

/** Create a nil sexp (empty list) and put it at the end of the list

    @return pointer to newly created sexp at end of list or error.
 */
struct result_sexp sexp_push_nil(sexp *list);

/** Create a nil sexp (empty list) and put it at the end of the list, unless a
    parameter is an error.

    @return pointer to newly created sexp at end of list or error.
 */
struct result_sexp sexp_rpush_nil(struct result_sexp list);

/** Create a generic sexp (empty list) and put it at the end of the list

    @return pointer to newly created sexp at end of list or error.
 */
struct result_sexp sexp_push(sexp *list, sexp *item);

/** Create a generic sexp (empty list) and put it at the end of the list, unless
    a parameter is an error.

    @return pointer to newly created sexp at end of list or error.
 */
struct result_sexp sexp_rpush(struct result_sexp list, struct result_sexp item);


/******************************** DATA GETTERS ********************************/
/** Returns the integer value of the sexp.  sexp must be of type SEXP_INTEGER.
    
    @return integer value of sexp
*/
struct result_s32 sexp_int_val(const sexp *s);

/** Returns the integer value of the sexp.  sexp must be of type SEXP_INTEGER,
    unless a parameter is an error.
    
    @return integer value of sexp
*/
struct result_s32 sexp_rint_val(struct result_sexp s);

/** Returns the string value of the sexp.  sexp must be of type SEXP_STRING.

    @return string value of sexp
*/
struct result_str sexp_str_val(const sexp *s);

/** Returns the string value of the sexp.  sexp must be of type SEXP_STRING,
    unless a parameter is an error.

    @return string value of sexp
*/
struct result_str sexp_rstr_val(struct result_sexp s);

/** Returns the symbol value of the sexp.  sexp must be of type SEXP_SYMBOL,

    @return symbol value of sexp
*/
struct result_str sexp_sym_val(const sexp *s);

/** Returns the symbol value of the sexp.  sexp must be of type SEXP_SYMBOL,
    unless a parameter is an error.

    @return symbol value of sexp
*/
struct result_str sexp_rsym_val(struct result_sexp s);

struct result_sexp sexp_tag_get_tag(const sexp *s);
struct result_sexp sexp_tag_get_atom(const sexp *s);

/** sets the CAR of dst to car.

    @return dst, or error. */
struct result_sexp sexp_setcar(sexp *dst, sexp *car);

/** sets the CAR of dst to car, if neither dst or car are an error.

    @return dst, or error. */
struct result_sexp sexp_rsetcar(struct result_sexp dst, struct result_sexp car);

/** sets the CDR of dst to cdr.

    @return dst, or error. */
struct result_sexp  sexp_setcdr(sexp *dst, sexp *cdr);

/** sets the CDR of dst to cdr, if neither dst or cdr are an error.

    @return dst, or error. */
struct result_sexp sexp_rsetcdr(struct result_sexp dst, struct result_sexp cdr);

/******************************** DATA SETTERS ********************************/
void sexp_set_integer(sexp *s, s32 i);
void sexp_set_string(sexp *s, const char *str);
void sexp_set_symbol(sexp *s, const char *sym);

/** gets the CAR of sexp.

    @return CAR of sexp. */
struct result_sexp sexp_car(const sexp *sexp);

/** gets the CAR of sexp, unless sexp is an error.

    @return CAR of sexp. */
struct result_sexp sexp_rcar(struct result_sexp sexp);

/** gets the CDR of sexp

    @return CDR of sexp. */
struct result_sexp sexp_cdr(const sexp *sexp);

/** gets the CDR of sexp, unless sexp is an error.

    @return CDR of sexp. */
struct result_sexp sexp_rcdr(struct result_sexp sexp);

/*********************************** OTHER ************************************/
/** Returns the symbol value of the sexp.  sexp must be of type SEXP_SYMBOL.,
    unless a parameter is an error.
    
    @return symbol value of sexp
*/
const sexp *sexp_find(const sexp *s, char *atom);

/** tests if sexp is nil

    A sexp is nil if it meets onse of the following conditions:
    - NULL ptr
    - sexp_type == SEXP_NIL
    - CDR points to itself
    - it is a symbol, and its value is "NIL" or "nil"

    @return true if sexp is nil, false otherwise.*/
bool sexp_is_nil(const sexp *sexp);

/** tests if sexp is nil, unless a parameter is an error.

    @return true if sexp
    is nil, false otherwise.*/
bool sexp_ris_nil(struct result_sexp sexp);

struct result_sexp sexp_nil();

enum sexp_type sexp_type(const struct sexp *s);

#endif
