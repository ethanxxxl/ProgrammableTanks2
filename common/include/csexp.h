#ifndef CSEXP_H
#define CSEXP_H

#include "nonstdint.h"
#include "vector.h"

#include <stdio.h>
#include <stddef.h>

/**
 * This library implements algorithms that read/write/manipulate S-Expressions.  Currently, The library includes functions that do not use malloc.
 */


#define FOR_EACH_RESULT_TYPE(RESULT) RESULT(RESULT_OK)  \
         RESULT(RESULT_ERR)                             \
         RESULT(RESULT_BAD_NETSTRING_LENGTH)            \
         RESULT(RESULT_NETSTRING_MISSING_COLON)         \
         RESULT(RESULT_TAG_NOT_CLOSED)                  \
         RESULT(RESULT_TAG_MISSING_TAG)                 \
         RESULT(RESULT_TAG_MISSING_SYMBOL)              \
         RESULT(RESULT_LIST_NOT_CLOSED)                 \
         RESULT(RESULT_QUOTE_NOT_CLOSED)                \
         RESULT(RESULT_SYMBOL_ESCAPE_NOT_CLOSED)        \
         RESULT(RESULT_INVALID_CHARACTER)               \
         RESULT(RESULT_TRAILING_GARBAGE)                \
         RESULT(RESULT_NULL_SEXP_PARAMETER)             \

#define GENERATE_ENUM(ENUM) ENUM,
#define GENERATE_STRING(STRING) #STRING,

enum reader_result_type {
    FOR_EACH_RESULT_TYPE(GENERATE_ENUM)
};

extern const char* G_READER_RESULT_TYPE_STR[];

/**
 If the reader encounters an error during parsing (malformed input), it will
 return an error type.  Some errors will be linked to the string being read.
 The location of the error is returned in the error_location field in these
 cases.

 If the error is not tied to the format string, `error_location` will be NULL.
*/
struct reader_result {
    enum reader_result_type status;
    union {
        const char* error_location;
        size_t length;
    };
};

enum sexp_type {
    SEXP_CONS,
    SEXP_SYMBOL,
    SEXP_STRING,
    SEXP_INTEGER,
    SEXP_TAG,
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
struct sexp {
    enum sexp_type type;
    u32 length;
    u8 data[];
};

/**
 reads the sexp string and puts it into the sexp object.

 WARNING: `sexp` must contain enough space to store the sexp.  This function
 does NOT use malloc.  If the size is not known, a dryrun may be performed to
 obtain a size first.

 If an error occurs during parsing, it will be described in the results
 structure that this function returns.
*/
struct reader_result
sexp_read(const char* sexp_str, struct sexp* sexp, bool dryrun);

s32
sexp_fprint(const struct sexp*, FILE*);

s32
sexp_print(const struct sexp*);

s32
sexp_serialize(const struct sexp* sexp, char* buffer, size_t size);


struct sexp*
sexp_append(struct sexp* dst, const struct sexp* src);

struct sexp*
sexp_append_dat(struct sexp* dst, void* dat, size_t len, enum sexp_type type);


// returns how much memory it would take to represent the data if it were a
// sexp.
size_t
sexp_size(enum sexp_type type, void* data);

/* returns the number of elements in the sexp list. */
size_t
sexp_length(const struct sexp* sexp);

const struct sexp*
sexp_nth(const struct sexp* list, size_t n);

const struct sexp*
sexp_find(const struct sexp* s, char* atom);

/*************************** Malloc Implementation ****************************/

struct sexp_dyn;
struct cons {
    struct sexp_dyn *car;
    struct sexp_dyn *cdr;
};

/**
 * @param type denotes whether what s-expression type is represented by the
 * union.
 * 
 * @param cons
 * 
 * @param integer
 *
 * @param text_len used for ATOM_STRING and ATOM_SYMBOL types.  It denotes how
 * many elements are stored in the text field at the end of the structure.
 *
 * @param text stores string data for ATOM_STRING and ATOM_SYMBOL
 */
struct sexp_dyn {
    enum sexp_type type;
    
    union {
        struct cons cons;
        s32 integer;
        size_t text_len;
    };

    char text[];
};

struct sexp_dyn *sexp_dyn_read(char *str);
struct sexp_dyn *sexp_to_dyn(const struct sexp *sexp);

struct sexp_dyn *make_cons(struct sexp_dyn *car, struct sexp_dyn *cdr);
struct sexp_dyn *make_integer(s32 num);
struct sexp_dyn *make_symbol(char *symbol);
struct sexp_dyn *make_string(char *text);

// TODO implement this!
struct sexp_dyn *make_tag();

#endif
