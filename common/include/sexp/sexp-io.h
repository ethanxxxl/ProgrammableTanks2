#ifndef SEXP_IO_H
#define SEXP_IO_H

#include "sexp/sexp-base.h"

#include <stdbool.h>
#include <stdio.h>

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
                             enum sexp_memory_method method);

/** Converts the S-Expression (sexp) to a string

    WARNING, this result must be free'ed!

    @return c-string contianing the sexp
 */
struct result_str sexp_serialize(const struct sexp *sexp);

/** Converts the S-Expression (sexp) to a string encapsulated in a vector.

    WARNING, this result must be free'ed!

    @return c-string contianing the sexp
 */
struct result_vec sexp_serialize_vec(const struct sexp *sexp);

/* TODO finish documenting this function*/
/** Serialize the sexp and send it to the specified file. */
struct result_s32 sexp_fprint(const struct sexp*, FILE*);

/* TODO document this function*/
s32 sexp_print(const struct sexp*);

#endif
