#ifndef SEXP_UTILS_H
#define SEXP_UTILS_H

#include "sexp/sexp-base.h"

#include <stdbool.h>


///////////////////////////////// Manipulators /////////////////////////////////


struct sexp *list(struct sexp *sexp[]);

void setcar(struct sexp *dst, struct sexp *car);
void setcdr(struct sexp *dst, struct sexp *cdr);
struct sexp *car(const struct sexp *sexp);
struct sexp *cdr(const struct sexp *sexp);

/** Add item to the end of the list. */
void append(struct sexp *list, struct sexp *item);

/** Return the nth element (car) of the list. */
struct sexp *nth(const struct sexp *sexp, u32 n);

bool is_nil(const struct sexp *sexp);
u32 length(const struct sexp *sexp);

/* returns the number of elements in the sexp list. */
size_t
sexp_length(const struct sexp *sexp);

struct sexp *sexp_nth(const struct sexp *list, size_t n);

/** Create a new integer S-Expression and put it at the end of list

    @return pointer to newly created sexp at end of list or error.
 */
struct result_sexp sexp_push_integer(sexp *list, s32);

/** Create a new string S-Expression and put it at the end of list

    @return pointer to newly created sexp at end of list or error.
*/
struct result_sexp sexp_push_string(sexp *list, const char *str);

/** Create a new symbol S-Expression and put it at the end of list

    @return pointer to newly created sexp at end of list or error.
*/
struct result_sexp sexp_push_symbol(sexp *list, const char *str);

/** Create an empty tag container and put it at the end of list

    @return pointer to newly created sexp at end of list or error.
 */
struct result_sexp sexp_push_tag(sexp *list);

/** Create a nil sexp (empty list) and put it at the end of the list

    @return pointer to newly created sexp at end of list or error.
 */
struct result_sexp sexp_push_nil(sexp *list);

const struct sexp *sexp_find(const struct sexp *s, char *atom);

void sexp_set_integer(struct sexp *s, s32 i);
void sexp_set_string(struct sexp *s, const char *str);
void sexp_set_symbol(struct sexp *s, const char *sym);

#endif
