#ifndef SEXP_BASE_H
#define SEXP_BASE_H

#include "error.h"
#include "nonstdint.h"
#include "error.h"

/**************************** SEXP IMPLEMENTATION *****************************/

enum sexp_type {
    SEXP_CONS = 0x0,
    SEXP_SYMBOL = 0x1,
    SEXP_STRING = 0x2,
    SEXP_INTEGER = 0x3,
    SEXP_TAG = 0x4,
};

struct cons {
    struct sexp *car;
    struct sexp *cdr;
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

/** Universial S-Expression Structure

 This structure represents an S-Expression (sexp) in memory.  There are two
 implementations available for this representation: Dynamic and Static.  The
 dynamic implementation allocates a new sexp structure every time one is
 created.  The static implementation conatinas all sexps in a contiguous block
 of memory, and an allocation is static or on the stack.

 Static and dynamic sexpression will not be mixed.
*/
// FIXME there is a problem here: for the linear style you don't know the
// capacity of the sexp.

// XXX maybe you can create a custom type just for the start of the linear sexp,
// that contains capacity information?  or perhaps, implicitly, every linear
// root sexp is a CONS, and the CAR contains the capacity, and the CDR contains
// the SEXP that is actually encoded.
struct sexp {
    u32 is_linear: 1;
    u32 is_root: 1;
    u32 sexp_type: 3;
    u32 data_length: 27;
    
    enum sexp_type type;
    union {
        struct cons cons;
        s32 integer; // TODO can this be an s64 and not take up any more space?
    };

    u8 data[];
};

/************************** ERRORS AND RETURN TYPES ***************************/

#define FOR_EACH_RESULT_TYPE(RESULT)                         \
         RESULT(SEXP_RESULT_ERR)                             \
         RESULT(SEXP_RESULT_BAD_NETSTRING_LENGTH)            \
         RESULT(SEXP_RESULT_NETSTRING_MISSING_COLON)         \
         RESULT(SEXP_RESULT_TAG_NOT_CLOSED)                  \
         RESULT(SEXP_RESULT_TAG_MISSING_TAG)                 \
         RESULT(SEXP_RESULT_TAG_MISSING_SYMBOL)              \
         RESULT(SEXP_RESULT_LIST_NOT_CLOSED)                 \
         RESULT(SEXP_RESULT_QUOTE_NOT_CLOSED)                \
         RESULT(SEXP_RESULT_SYMBOL_ESCAPE_NOT_CLOSED)        \
         RESULT(SEXP_RESULT_INVALID_CHARACTER)               \
         RESULT(SEXP_RESULT_TRAILING_GARBAGE)                \
         RESULT(SEXP_RESULT_NULL_SEXP_PARAMETER)             \
 
#define GENERATE_ENUM(ENUM) ENUM,
#define GENERATE_STRING(STRING) #STRING,

enum sexp_reader_error_code {
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
struct sexp_reader_error {
    enum sexp_reader_error_code code;
    char *input;
    char *location;
};

struct error sexp_reader_error(enum sexp_reader_error_code, const char *input,
                               const char *locaton);
const char *describe_sexp_reader_error(void *self);
void free_sexp_reader_error(void *self);

const struct error_ops SEXP_READER_ERROR_OPS = {
    .describe = describe_sexp_reader_error,
    .free = free_sexp_reader_error,
};

/** Single function to return an error wrapped in a `struct sexp_result type`.
    calls `sexp_reader_error()` to generate the error.  NOTE: the RESULT_OK
    equivalent of this function is `result_sexp_ok()` that is defined with the
    DEFINE_RESULT_TYPE_CUSTOM() macro.

    @param code The error that was encountered during parsing
    @param input The string that was being parsed.
    @param location The location at which the error occured.

    @return Tagged union result_sexp containing the error.
*/
struct result_sexp reader_err(enum sexp_reader_error_code code,
                              const char *input,
                              const char *location);

/** Tagged Union for the reader, utilities, etc.*/
DEFINE_RESULT_TYPE_CUSTOM(struct sexp *, sexp)

#endif
