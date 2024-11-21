#ifndef SEXP_BASE_H
#define SEXP_BASE_H

#include "error.h"
#include "nonstdint.h"
#include "error.h"
#include "enum_reflect.h"

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
#define ENUM_SEXP_TYPE_ITEMS                \
    SEXP_CONS,                              \
    SEXP_SYMBOL,                            \
    SEXP_STRING,                            \
    SEXP_INTEGER,                           \
    SEXP_TAG,                               \
    SEXP_LIST_TERMINATOR,                   \
    SEXP_LINEAR_ROOT

enum sexp_type { ENUM_SEXP_TYPE_ITEMS };
extern const char *g_reflected_sexp_type[];

/** Fundamental structure for both S-Expression implementations. */
struct cons {
    struct sexp *car;
    struct sexp *cdr;
};

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
    When initialized through `make_sexp()`, the caller will receive a handle to
    an sexp.  If a function such as `sexp_append()` is called on a child sexp,
    it could require that the sexp be realloc'ed.  The root sexp isn't passed
    into any utility functions to maintain transparency between memory layouts.
    This means that if a realloc occurs, the caller will have a bad handle to
    the root sexp, causing a memory leak.

    To solve this, a linear sexp performs two memory allocations.  The result of
    the first is passed to the caller, and is never realloc'ed.  It contains the
    result of the second allocation, which is the actual sexp handle.

    The first node is special in linear S-Expressions.  Regardless of the
    desired sexp type, the root node is always of type `SEXP_LINEAR_ROOT`.  the
    corresponding union type contains the capacity of the allocated memory,
    along with a pointer to the sexp that represents the data the user wants to
    encode.

    The sexp pointed to in root node is also special.  It is a list with the
    form `(ROOT_HANDLE CAPACITY VALUE)`.  `ROOT_HANDLE` points back to the
    root node, and is of type `SEXP_LINEAR_ROOT`.  `CAPACITY` contains the
    capacity of the sexp, and is a `SEXP_INTEGER`.  Finally, `VALUE` is whatever
    the desired sexp is.

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
struct sexp{
    u32 is_linear: 1;
    u32 is_root: 1;
    u32 sexp_type: 3;
    u32 data_length: 27;
    
    u8 data[];
};

typedef struct sexp sexp;

#define SEXP_MAX_LENGTH (0x7ffffff)

union sexp_data {
    struct cons cons;
    s32 integer;
    
    struct sexp *linear_root;
};

/************************** ERRORS AND RETURN TYPES ***************************/

#define READER_ERROR_CODE_ENUM_VALUES           \
        SEXP_RESULT_ERR,                        \
        SEXP_RESULT_BAD_NETSTRING_LENGTH,       \
        SEXP_RESULT_NETSTRING_MISSING_COLON,    \
        SEXP_RESULT_TAG_NOT_CLOSED,             \
        SEXP_RESULT_TAG_MISSING_TAG,            \
        SEXP_RESULT_TAG_MISSING_SYMBOL,         \
        SEXP_RESULT_LIST_NOT_CLOSED,            \
        SEXP_RESULT_QUOTE_NOT_CLOSED,           \
        SEXP_RESULT_SYMBOL_ESCAPE_NOT_CLOSED,   \
        SEXP_RESULT_INVALID_CHARACTER,          \
        SEXP_RESULT_TRAILING_GARBAGE,           \
        SEXP_RESULT_NULL_SEXP_PARAMETER
enum sexp_reader_error_code { READER_ERROR_CODE_ENUM_VALUES };
extern const char *g_reflected_sexp_reader_error_code[];

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
char *describe_sexp_reader_error(void *self);
void free_sexp_reader_error(void *self);

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
DECLARE_RESULT_TYPE_CUSTOM(struct sexp *, sexp)

    
/******************************** INITIALIZERS ********************************/
/** Initialize a new S-Expression (sexp) object.

    creates a new sexp type, using the indicated method.
*/
struct result_sexp make_sexp(enum sexp_type type,
                             enum sexp_memory_method method, void *data);

struct result_sexp make_integer_sexp(s32 num);
struct result_sexp make_string_sexp(const char *str);
struct result_sexp make_symbol_sexp(const char *sym);
struct result_sexp make_cons_sexp();
void free_sexp(sexp *s);

#endif
