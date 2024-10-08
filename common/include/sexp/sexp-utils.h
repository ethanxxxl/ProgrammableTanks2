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

const struct sexp *
sexp_nth(const struct sexp* list, size_t n);

const struct sexp*
sexp_find(const struct sexp* s, char* atom);

#endif
