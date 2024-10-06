#ifndef SEXP_IO_H
#define SEXP_IO_H

#include "sexp/sexp-base.h"

#include <stdbool.h>
#include <stdio.h>

// FIXME sexp_static needs removed
struct sexp_static;

/**
 reads the sexp string and puts it into the sexp object.

 WARNING: `sexp` must contain enough space to store the sexp.  This function
 does NOT use malloc.  If the size is not known, a dryrun may be performed to
 obtain a size first.

 If an error occurs during parsing, it will be described in the results
 structure that this function returns.
*/
struct reader_result
sexp_read(const char* sexp_str, struct sexp_static* sexp, bool dryrun);

s32
sexp_fprint(const struct sexp_static*, FILE*);

s32
sexp_print(const struct sexp_static*);

s32
sexp_serialize(const struct sexp_static* sexp, char* buffer, size_t size);


struct sexp_static*
sexp_append(struct sexp_static* dst, const struct sexp_static* src);

struct sexp_static*
sexp_append_dat(struct sexp_static* dst, void* dat, size_t len, enum sexp_type type);


struct sexp_dyn *sexp_dyn_read(char *str);
struct sexp_dyn *sexp_to_dyn(const struct sexp_static *sexp);
struct sexp_static *sexp_from_dyn(const struct sexp_dyn *sexp);


#endif
