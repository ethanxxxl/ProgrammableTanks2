#ifndef SEXP_UTILS_H
#define SEXP_UTILS_H

#include "sexp/sexp-base.h"

///////////////////////////////// Initializers /////////////////////////////////

struct sexp_dyn *make_cons(struct sexp_dyn *car, struct sexp_dyn *cdr);
struct sexp_dyn *make_integer(s32 num);
struct sexp_dyn *make_symbol(const char *symbol);
struct sexp_dyn *make_string(const char *text);

// TODO implement this!
struct sexp_dyn *make_tag(void);

///////////////////////////////// Manipulators /////////////////////////////////


struct sexp_dyn *list(struct sexp_dyn *sexp[]);

void setcar(struct sexp_dyn *dst, struct sexp_dyn *car);
void setcdr(struct sexp_dyn *dst, struct sexp_dyn *cdr);
struct sexp_dyn *car(const struct sexp_dyn *sexp);
struct sexp_dyn *cdr(const struct sexp_dyn *sexp);

/** Add item to the end of the list. */
void append(struct sexp_dyn *list, struct sexp_dyn *item);

/** Return the nth element (car) of the list. */
struct sexp_dyn *nth(const struct sexp_dyn *sexp, u32 n);

bool is_nil(const struct sexp_dyn *sexp);
u32 length(const struct sexp_dyn *sexp);

// returns how much memory it would take to represent the data if it were a
// sexp.
size_t
sexp_size(enum sexp_type type, void* data);

/* returns the number of elements in the sexp list. */
size_t
sexp_length(const struct sexp_static* sexp);

const struct sexp_static*
sexp_nth(const struct sexp_static* list, size_t n);

const struct sexp_static*
sexp_find(const struct sexp_static* s, char* atom);

#endif
