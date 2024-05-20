#ifndef CSEXP_H
#define CSEXP_H

#include "nonstdint.h"
#include "vector.h"

#include <stdio.h>
#include <stddef.h>

enum sexp_type {
    SEXP_ATOM,
    SEXP_TAGGED_ATOM,
    SEXP_LIST,
};

struct sexp_prefix {
    enum sexp_type type;
    u32 length;
};

/** 
 * There are three types of items that can be represented by a sexp_item:
 * - <simple-string> :: <raw>
 * - <string>        :: <display>? <simple-string>
 * - <list>          :: "(" <sexp>* ")"
 *
 * These will be represented in memory as follows:
 * TYPE LENGTH DATA
 *
 * Where TYPE is a u8, LENGTH is a u32, and DATA is as many bytes as indicated
 * by LENGTH.
 *
 * This structure is intended to allow for more convenient access to the
 * elements in a data stream.
 *
 * @param type Either SEXP_SIMPLE_STRING, SEXP_STRING, or SEXP_LIST.
 * @param length The length of data.
 * @param data Actual data encoded by the sexp_item.
 */
struct sexp_item {
    enum sexp_type type;
    u32 length;
    const u8* data;
};

int sexp_append(struct sexp_item dest, const struct sexp_item src);

/**
 * takes a sexp string and returns how many bytes it will take to represesnt it
 * in memory
 */
vector* sexp_read(const u8 *sexp);
void sexp_fprint(FILE *f, vector *sexp);
void sexp_print(vector* sexp);

#endif
