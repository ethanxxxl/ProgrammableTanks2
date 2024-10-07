#ifndef SEXP_IO_H
#define SEXP_IO_H

#include "sexp/sexp-base.h"

#include <stdbool.h>
#include <stdio.h>

// FIXME sexp_static needs removed
struct sexp_static;

enum sexp_reader_method {
    READ_LINEAR,
    READ_TREE
};

/** Reads the S-Expression (sexp) in the provided string and returns a sexp
    object or an error.  

    The sexp object be allocated on the heap and must be freed before it goes
    out of scope.  There are two storage implementations for sexp.  Currently,
    this function returns a linear sexp rather than a tree sexp.

    If an error occurs during parsing, the result structure will have a
    `RESULT_ERROR` status, and the error field will contain an error describing
    the error that occured.

    @param sexp_str The string that contains a serialized S-Expression
    @param method Determines whether the returned S-Expressio is stored using a
                  linear or tree format.
    @return Tagged Union containing either an S-Expression or `error` Object.
*/
struct result_sexp sexp_read(const char *sexp_str,
                             enum sexp_reader_method method);

/* TODO finish documenting this function*/
/** Serialize the sexp and send it to the specified file. */
s32 sexp_fprint(const struct sexp_static*, FILE*);

/* TODO document this function*/
s32 sexp_print(const struct sexp_static*);

/** Serializes the S-Expression and returns a pointer to the resulting string.
    WARNING: this value must be free'ed!

    @param sexp The S-Expression to serialize.

    @return pointer to serialized expression on heap.*/
struct result_str sexp_serialize(const struct sexp *sexp);

/* TODO document this function*/
struct sexp_static*
sexp_append(struct sexp_static* dst, const struct sexp_static* src);

/* TODO document this function*/
struct sexp_static*
sexp_append_dat(struct sexp_static* dst, void* dat, size_t len, enum sexp_type type);

/* TODO document this function*/
struct sexp_dyn *sexp_dyn_read(char *str);

/* TODO document this function*/
struct sexp_dyn *sexp_to_dyn(const struct sexp_static *sexp);

/* TODO document this function*/
struct sexp_static *sexp_from_dyn(const struct sexp_dyn *sexp);


#endif
