#ifndef SEXP_BASE_H
#define SEXP_BASE_H

#include "error.h"
#include "nonstdint.h"
#include "error.h"

/**************************** SEXP IMPLEMENTATION *****************************/
/** The S-Expression (sexp) memory layout/allocation method.

    1. The `SEXP_MEMORY_LINEAR` method uses a single contiguous block of memory
    to store all data related to the sexp

    2. The `SEXP_MEMORY_TREE` method allocates memory for every cons/atom in a
    sexp.
*/
enum sexp_memory_method {
    SEXP_MEMORY_LINEAR,
    SEXP_MEMORY_TREE
};

/** Type of S-Expression (sexp) element.

    This implementation of sexp contains the types listed in this enumeration.
    Custom types that extend the implmentation are created using the `SEXP_TAG`
    type.

    The `SEXP_LIST_TERMINATOR` is used as the last element in linear sexp lists.
    
 */
enum sexp_type {
    SEXP_CONS = 0x0,
    SEXP_SYMBOL = 0x1,
    SEXP_STRING = 0x2,
    SEXP_INTEGER = 0x3,
    SEXP_TAG = 0x4,

    SEXP_LIST_TERMINATOR = 0x5,
};

/** Fundamental structure for both S-Expression implementations. */
struct cons {
    struct sexp *car;
    struct sexp *cdr;
};

// there is a problem here.  If you pass a linear sexp to a function, that
// function can know if it is the root or not.  There is know way to find the
// root if the function is operating on a child node.

// 

/** Universial S-Expression Structure

    This structure represents an S-Expression (sexp) in memory.  There are two
    implementations available for this representation: linear and tree.  The
    Tree implementation allocates a new sexp structure every time one is
    created.  The linear implementation conatinas all sexps in a contiguous
    block of memory.

    Static and dynamic sexp's are not mixed with each other.  It is not
    advisable to create your own sexp object or modify any of the fields, as
    this may invalidate the structure, or worse, cause a memory leak or
    segmentation fault.

    NOTE: currently this implementation does not support dotted forms.
        
    Linear S-Expressions
    ============================================================================
    When initialized, regardless of the desired sexp type, root node is always
    of type `SEXP_CONS`.  At a high level, this means every linear sexp takes
    the form `(CAPACITY SEXP)`. `CAPACITY` is a `SEXP_INTEGER` and indicates how
    much space was allocated via `malloc()`.  `SEXP` may be any sexp type.

    Lists in a linear sexp are terminated with a sexp with type
    `SEXP_LIST_TERMINATOR`.  This isn't a "real" sexp type, it is only used as
    the last element in a list.  The `data_length` field of a
    `SEXP_LIST_TERMINATOR` is used to indicate the size of the list in bytes.
    Thus the "parent" of any sexp can be found be jumping back from the list
    terminator.  This can be done recursively until the sexp with `is_root =
    true` is found.

    Tree S-Expressions
    ============================================================================

    @param is_linear boolean value that specifies the memory layout of the
    structure.

    @param is_root boolean flag to indicate if the current sexp is the topmost
    object.  Linear sexps must know the capacity of the entire block of data
    allocated in addition to the length of used space.  This is accomplished
    through an implicit/hidden CONS cell as the root sexp.  The CAR of this CONS
    cell contains an integer SEXP that tracks buffer capaicity.  The CDR
    contains the actual first element of the sexp.

    @param sexp_type corresponds to the `sexp_type` enumeration.  This field
    specifies the format of the `data` field.

    @param data_length the remaining bits in the 4 byte bit field are used to
    indicate the length (in bytes) of the data stored in the `data` flexible
    array member at the end of this structure.

    @param data flexible array member that corresponds to an ASCII c-string when
    `sexp_type` is SEXP_SYMBOL or SEXP_STRING. `data` maps to the `sexp_data`
    union for all other values of `sexp_type`, unless the linear layout is used.
    In a linear layout, `data` maps to `sexp_data` when `sexp_type` is not
    `SEXP_CONS`.  When `sexp_type` is `SEXP_CONS`, `data` contains more sexp
    structures, one after another, each containing their own length.
*/
struct sexp {
    u32 is_linear: 1;
    u32 is_root: 1;
    u32 sexp_type: 3;
    u32 data_length: 27;
    
    u8 data[];
};

#define SEXP_MAX_LENGTH (0x7ffffff)

union sexp_data {
    struct cons cons;
    s32 integer;
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

    
/******************************** INITIALIZERS ********************************/
/** Initialize a new S-Expression (sexp) object.

    creates a new sexp type, using the indicated method.
*/
struct result_sexp make_sexp(enum sexp_type type,
                             enum sexp_memory_method method);

#endif
