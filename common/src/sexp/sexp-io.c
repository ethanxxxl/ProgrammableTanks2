#include "sexp/sexp-io.h"
#include "error.h"
#include "sexp/sexp-base.h"
#include "sexp/sexp-utils.h"
#include "vector.h"

#include <ctype.h>
#include <string.h>
#include <stdlib.h>


/*************************** SEXP READER FUNCITONS ****************************/
/** reads an attom from the string and returns a pointer to the sexp. */
struct result_sexp
sexp_read_atom(const char **caller_cursor,
               enum sexp_memory_method method) {
    const char* cursor = *caller_cursor;
    while (isspace(*cursor) && *cursor != '\0') cursor++;

    if (memchr("\0])", *cursor, 3) != 0)
        // TODO get the right error here
        return reader_err(0, *caller_cursor, cursor);

    // test for netstring
    const char* digit_end = cursor;
    u32 atom_number_value = strtoul((char*)cursor, (char**)&digit_end, 10);

    enum sexp_reader_error_code error_code;

    char* delims;
    u32 num_delims;

    enum sexp_type atom_type;
    union {
        const char *str;
        s32 integer;
    } atom_data;
    size_t atom_length;

    bool should_skip_terminator = false;
    bool symbol_is_escaped = false;
    bool is_netstring = false;
    
    // Determine atom type and extract data.    
    if (digit_end != cursor && *digit_end == ':') {
        // NETSTRING
        atom_type = SEXP_SYMBOL;
        atom_data.str = digit_end+1;
        atom_length = atom_number_value;
        is_netstring = true;
        symbol_is_escaped = true;
        
        cursor = digit_end + atom_number_value + 1;
        
    }  else if (digit_end != cursor && (isspace(*digit_end) || *digit_end == ')')) {
        // NUMBER
        atom_type = SEXP_INTEGER;
        atom_data.integer = atom_number_value;
        atom_length = sizeof(s32);
        
        cursor = digit_end;

    } else if (digit_end != cursor && *digit_end != ':') {
        // ERROR CASE
        return reader_err(SEXP_RESULT_NETSTRING_MISSING_COLON, *caller_cursor, digit_end);
        
    } else if (*digit_end == '|') {
        // ESCAPED SYMBOL
        atom_type = SEXP_SYMBOL;
        atom_data.str = digit_end+1;
        atom_length = 0; // filled in next subsequent block
        
        delims = "|";
        num_delims = 1;
            
        error_code = SEXP_RESULT_SYMBOL_ESCAPE_NOT_CLOSED;
        cursor = digit_end+1;
        should_skip_terminator = true;
        symbol_is_escaped = true;
        
    } else if (*digit_end == '"') {
        // STRING
        atom_type = SEXP_STRING;
        atom_data.str = digit_end+1;
        atom_length = 0; // filled in next subsequent block

        delims = "\"";
        num_delims = 1;

        error_code = SEXP_RESULT_QUOTE_NOT_CLOSED;
        cursor = digit_end+1;
        should_skip_terminator = true;
        
    } else {
        // SYMBOL
        atom_type = SEXP_SYMBOL;
        atom_data.str = digit_end;
        atom_length = 0; // filled in next subsequent block
        
        delims = "\0 ()[]\"";
        num_delims = 7;
    }

    // Get lengths for stringy sexp types.
    if ((atom_type == SEXP_SYMBOL || atom_type == SEXP_STRING) &&
        is_netstring == false) {
        for (atom_length = 0;
             memchr(delims, *cursor, num_delims) == NULL && *cursor != '\0';
             cursor++, atom_length++);

        // the string ended without finding a terminating delimeter.
        if (memchr(delims, *cursor, num_delims) == NULL) {
            return reader_err(error_code, *caller_cursor, cursor);
        }

        // skip past the terminator, unless this is a regular symbol.
        if (should_skip_terminator == true) {
            cursor = cursor + 1;
        }
    }

    char null_terminated_str[atom_length + 1];
    if (atom_type == SEXP_SYMBOL || atom_type == SEXP_STRING) {
        // BUG: This currently doesn't work for netstrings that have null
        // characters embedded in them!

        // copy atom data into temporary buffer (so that it is null terminated.)
        memcpy(null_terminated_str, atom_data.str, atom_length);
        null_terminated_str[atom_length] = '\0';
    }

    // make non-escaped symbols upper-case
    if (atom_type == SEXP_SYMBOL && symbol_is_escaped == false) {
        for (char *c = null_terminated_str;
             c < null_terminated_str + atom_length;
             c++) {
            *c = toupper(*c);
        }
    }

    *caller_cursor = cursor;

    if (atom_type == SEXP_INTEGER)
        return make_sexp(atom_type, method, &atom_data.integer);
    else
        return make_sexp(atom_type, method, null_terminated_str);
}

/** Reads a tag from the string and returns a pointer to that sexp.*/
struct result_sexp
sexp_read_tagged_atom(const char **caller_cursor,
                      enum sexp_memory_method method) {
    const char* cursor = *caller_cursor;

    // INFO: the details in this comment only pertain to the linear method, and
    // are slightly out of date.
    // 
    // read the atom inside the tag and move the cursor past it.  This also puts
    // the atom in the sexp object.
    //
    // 1:SEXP_TAGGED_ATOM 4:LENGTH
    //         1:SEXP_ATOM 4:TAG_LENGTH TAG_LENGTH:TAG

    // ([ 3:foo ]3:bar)  ->  ([ 3:foo ]3:bar)
    // ~~⬆~~~~~~~~~~~~~  ->  ~~~~~~~~⬆~~~~~~~
    struct result_sexp tag_type;
    tag_type = sexp_read_atom(&cursor, method);
    if (tag_type.status == RESULT_ERROR) {
        free_error(tag_type.error);
        return reader_err(SEXP_RESULT_TAG_MISSING_TAG, *caller_cursor, cursor);
    }

    while(isspace(*cursor)) cursor++;

    // make sure the tag is closed.  Error if not.
    if (*cursor != ']')
        return reader_err(SEXP_RESULT_TAG_NOT_CLOSED, *caller_cursor, cursor);


    // INFO: the details in this comment only pertain to the linear method, and
    // are slightly out of date.
    //
    // move past the tag to the atom.
    // ([ 3:foo ]3:bar)  ->  ([ 3:foo ]3:bar)
    // ~~~~~~~~~⬆~~~~~~  ->  ~~~~~~~~~~⬆~~~~~
    cursor++;

    // INFO: the details in this comment only pertain to the linear method, and
    // are slightly out of date.
    //
    // read the atom associated with the tag.  This will move the cursor and
    // past the atom and put the atom on the sexp object.
    //
    // 1:SEXP_TAGGED_ATOM 4:LENGTH
    //         1:SEXP_ATOM 4:TAG_LENGTH TAG_LENGTH:TAG
    //         1:SEXP_ATOM 4:ATOM_LENGTH ATOM_LENGTH:ATOM
    //
    // ([ 3:foo ]3:bar)  ->  ([ 3:foo ]3:bar)
    // ~~~~~~~~~~⬆~~~~~  ->  ~~~~~~~~~~~~~~~⬆
    struct result_sexp tag_value;
    tag_value = sexp_read_atom(&cursor, method);
    if (tag_value.status == RESULT_ERROR) {
        free_error(tag_value.error);
        return reader_err(SEXP_RESULT_TAG_MISSING_SYMBOL, *caller_cursor, cursor);
    }

    struct result_sexp toplevel_tag;
    toplevel_tag = make_sexp(SEXP_TAG, method, NULL);

    toplevel_tag = sexp_rsetcar(toplevel_tag, tag_type);
    toplevel_tag = sexp_rsetcdr(toplevel_tag, tag_value);

    // update caller cursor, and return length in of sexp (in memory)
    *caller_cursor = cursor;
    return toplevel_tag;
}

struct result_sexp
sexp_reader(const char **sexp_str,
            enum sexp_memory_method method);

// list points to the first item in the list
struct result_sexp
sexp_read_list(const char **caller_cursor,
               enum sexp_memory_method method) {
    const char* cursor = *caller_cursor;

    struct result_sexp list = sexp_nil();
    struct result_sexp end = list;

    bool first_element = true;
    while (true) {
        // skip leading whitespace
        while (isspace(*cursor) && *cursor != '\0') cursor++; 

        // see the the character under the cursor indicates the end of the list
        if (*cursor == ')') {
            cursor++;
            *caller_cursor = cursor;
            return list;
        }

        if (*cursor == '\0')
            return reader_err(SEXP_RESULT_LIST_NOT_CLOSED, *caller_cursor, cursor);

        end = sexp_rpush(end, sexp_reader(&cursor, method));

        // list is initialized to nil, the first element will return a new list.
        if (first_element == true) {
            list = end;
            first_element = false;
        }

        if (end.status == RESULT_ERROR)
            break;
    }

    return list;
}

/** Reads the next token in an S-Expression.

    works on a string containing an S-Expression.  On the first call, `root`
    should be `NULL`, and `end_or_method` should be a pointer to an `enum
    sexp_memory_method`, which will determine the method used to allocate the
    root.

    NOTE: this function doesn't do anything with the `root` or `end_or_method`
    parameters; it simply passes them on to either `sexp_read_list()`,
    `sexp_read_tagged_atom()`, or `sexp_read_atom()`.
*/
struct result_sexp
sexp_reader(const char **caller_cursor,
            enum sexp_memory_method method) {
    const char* cursor = *caller_cursor;
    while (isspace(*cursor) && *cursor != '\0') cursor++;

    // determine what the next token type is.
    enum token_type {
        LIST_OR_CONS,
        TAGGED_ATOM,
        ATOM
    } token_type;

    switch (*cursor) {
    case '(':
        token_type = LIST_OR_CONS;
        cursor++;
        break;
    case '[':
        token_type = TAGGED_ATOM;
        cursor++;
        break;

    case ']':
        return reader_err(SEXP_RESULT_INVALID_CHARACTER, *caller_cursor, cursor);

    default:
        token_type = ATOM;
    }

    // create a sexp for the next token and return it.
    struct result_sexp r;

    switch (token_type) {
    case LIST_OR_CONS:
        r = sexp_read_list(&cursor, method);
        break;
    case TAGGED_ATOM:
        r = sexp_read_tagged_atom(&cursor, method);
        break;
    case ATOM:
        r = sexp_read_atom(&cursor, method);
        break;
    }

    if (r.status == RESULT_ERROR)
        return r;

    *caller_cursor = cursor;
    return r;
}

struct result_sexp
sexp_read(const char *sexp_str, enum sexp_memory_method method) {
    const char *cursor = sexp_str;
    struct result_sexp r;
    
    while (isspace(*sexp_str) && *sexp_str != '\0') sexp_str++;
    if (*cursor == ')')
        return reader_err(SEXP_RESULT_INVALID_CHARACTER, sexp_str, cursor);

    r = sexp_reader(&cursor, method);

    if (r.status == RESULT_ERROR)
        return r;
    
    // fail if their is trailing garbage.
    while (isspace(*cursor) && *cursor != '\0') cursor++;
    if (*cursor != '\0')
        return reader_err(SEXP_RESULT_TRAILING_GARBAGE, sexp_str, cursor);

    return r;
}

/************************** SEXP SERIALIZE FUNCTIONS **************************/

struct result_s32 sexp_serialize_list(const sexp *, vector *);
struct result_s32 sexp_serialize_symbol(const sexp *, vector *);
struct result_s32 sexp_serialize_integer(const sexp *, vector *);
struct result_s32 sexp_serialize_tag(const sexp *, vector *);
struct result_s32 sexp_serialize_string(const sexp *, vector *);

/** takes a sexp and dynamic buffer. */
struct result_s32
sexp_serialize_any(const sexp *s, vector *buffer) {
    if (buffer == NULL)
        return RESULT_MSG_ERROR(s32, "unexpected NULL buffer");

    struct result_s32 r;
    switch (sexp_type(s)) {
    case SEXP_NIL:
    case SEXP_CONS:
        r = sexp_serialize_list(s, buffer);
        break;
    case SEXP_SYMBOL:
        r = sexp_serialize_symbol(s, buffer);
        break;
    case SEXP_INTEGER:
        r = sexp_serialize_integer(s, buffer);
        break;
    case SEXP_TAG:
        r = sexp_serialize_tag(s, buffer);
        break;
    case SEXP_STRING:
        r = sexp_serialize_string(s, buffer);
        break;
    default:
        r = result_s32_ok(0);
        break;
    }

    return r;
}

struct result_s32
sexp_serialize_symbol(const sexp *sexp, vector *buffer) {
    if (buffer == NULL)
        return RESULT_MSG_ERROR(s32, "unexpected NULL buffer");

    if (sexp_type(sexp) != SEXP_SYMBOL)
        return RESULT_MSG_ERROR(s32, "sexp type is %s, not SEXP_SYMBOL",
                                g_reflected_sexp_type[sexp_type(sexp)]);
                               

    enum {NORMAL, ESCAPED, NETSTRING} representation = NORMAL; 
    
    for (s32 c = 0; c < sexp->data_length && sexp->data[c] != '\0'; c++) {
        if (islower(sexp->data[c]) && representation == NORMAL)
            representation = ESCAPED;

        if (sexp->data[c] == '|') {
            representation = NETSTRING;
            break;
        }
    }

    s32 size_start = vec_len(buffer);
    if (representation == ESCAPED)
        vec_push(buffer, "|");

    // could use sexp->data-len here, but if the string is improperly formatted,
    // it could screw up the formatting of the output.
    s32 symbol_len = strlen((char*)sexp->data);

    if (representation == NETSTRING) {
        // I am using malloc here because I am lazy.
        char *netstring_header;
        s32 header_len = asprintf(&netstring_header, "%d:", symbol_len);
        vec_pushn(buffer, netstring_header, header_len);
        free(netstring_header);
    }

    // copy all string data to the buffer.  Return allocation errors.
    s32 r = vec_pushn(buffer, sexp->data, symbol_len);
    if (r < 0)
        return RESULT_MSG_ERROR(s32, "vector resize failed");
    
    if (representation == ESCAPED)
        vec_push(buffer, "|");

    return result_s32_ok(vec_len(buffer) - size_start);
}

struct result_s32
sexp_serialize_tag(const sexp *sexp, vector *buffer) {
    if (buffer == NULL)
        return RESULT_MSG_ERROR(s32, "unexpected NULL buffer");

    if (sexp_type(sexp) != SEXP_TAG)
        return RESULT_MSG_ERROR(s32, "sexp type is %s not SEXP_TAG",
                                g_reflected_sexp_type[sexp_type(sexp)]);

    const struct sexp* tag;
    const struct sexp* atom;

    // get tag
    RESULT_UNWRAP(s32, tag, sexp_tag_get_tag(sexp));

    // get atom
    RESULT_UNWRAP(s32, atom, sexp_tag_get_atom(sexp));

    s32 size_start = vec_len(buffer);

    s32 e;
    e = vec_push(buffer, "[");
    if (e < 0) goto allocation_error;
    
    RESULT_CALL(s32, sexp_serialize_symbol(tag, buffer));
    
    e = vec_push(buffer, "]");
    if (e < 0) goto allocation_error;
    
    RESULT_CALL(s32, sexp_serialize_symbol(atom, buffer));

    return result_s32_ok(vec_len(buffer) - size_start);
    
 allocation_error:
    return RESULT_MSG_ERROR(s32, "failed to allocate space in buffer!\ncontents: %s",
                            vec_dat(buffer));
}
    
struct result_s32
sexp_serialize_string(const sexp *sexp, vector *buffer) {
    if (buffer == NULL)
        return RESULT_MSG_ERROR(s32, "unexpected NULL buffer");

    if (sexp_type(sexp) != SEXP_STRING)
        return RESULT_MSG_ERROR(s32, "sexp type is %s, not SEXP_STRING",
                                g_reflected_sexp_type[sexp_type(sexp)]);

    s32 size_start = vec_len(buffer);
    s32 len = strlen((char *)sexp->data);

    s32 e;
    // BUG characters in the string may need to be escaped
    e = vec_push(buffer, "\"");
    if (e < 0) goto allocation_error;

    e = vec_pushn(buffer, sexp->data, len);
    if (e < 0) goto allocation_error;

    e = vec_push(buffer, "\"");
    if (e < 0) goto allocation_error;

    return result_s32_ok(vec_len(buffer) - size_start);

 allocation_error:
    return RESULT_MSG_ERROR(s32, "failed to allocate space in buffer!\ncontents: %s",
                            vec_dat(buffer));
}

struct result_s32
sexp_serialize_integer(const sexp *sexp, vector *buffer) {
    if (buffer == NULL)
        return RESULT_MSG_ERROR(s32, "unexpected NULL buffer");

    if (sexp_type(sexp) != SEXP_INTEGER)
        return RESULT_MSG_ERROR(s32, "sexp type is %s, not SEXP_INTEGER",
                                g_reflected_sexp_type[sexp_type(sexp)]);

    s32 buffer_space = vec_cap(buffer) - vec_len(buffer);
    s32 bytes_written = 0;

    do {
        bytes_written = snprintf((char *)vec_last(buffer) + 1,
                                 buffer_space,
                                 "%d", *(u32*)sexp->data);

        if (bytes_written > buffer_space) {
            s32 r = vec_reserve(buffer, vec_len(buffer) + bytes_written);
            if (r == -1)
                return RESULT_MSG_ERROR(s32, "vec reserve failed");

            continue;
        }

        vec_resize(buffer, vec_len(buffer) + bytes_written);
        break;
    } while (true);

    return result_s32_ok(bytes_written);
}

struct result_s32
sexp_serialize_list(const sexp *list, vector *buffer) {
    if (buffer == NULL)
        return RESULT_MSG_ERROR(s32, "unexpected NULL buffer");

    if (sexp_type(list) != SEXP_CONS && sexp_type(list) != SEXP_NIL)
        return RESULT_MSG_ERROR(s32, "sexp type is %s, not SEXP_CONS or SEXP_NIL",
                                g_reflected_sexp_type[sexp_type(list)]);
    
    size_t start_length = vec_len(buffer);

    sexp *element = (sexp *)list;

    vec_push(buffer, "(");
    
    struct result_s32 r;
    bool is_nil = true;
    while (sexp_is_nil(element) == false) {
        // put the serialized element on the end of the buffer
        sexp *item;
        RESULT_UNWRAP(s32, item, sexp_car(element));
        
        r = sexp_serialize_any(item, buffer);
        if (r.status == RESULT_ERROR)
            return r;

        // Add space between sexp elements
        vec_push(buffer, " ");

        RESULT_UNWRAP(s32, element, sexp_cdr(element));
        is_nil = false;
    }

    // remove the last space appended to the buffer.  No space is added when
    // there are no elements.
    if (is_nil == false)
        vec_pop(buffer, NULL);

    vec_push(buffer, ")");
    
    return result_s32_ok(vec_len(buffer) - start_length);
}

struct result_vec sexp_serialize_vec(const sexp *sexp) {
    // this is currently just a guess at how much capacity would be needed.
    struct vector *buffer = make_vector(sizeof(char), 30);
    
    if (buffer == NULL)
        return RESULT_MSG_ERROR(vec, "make_vector returned zero");
    
    struct result_s32 r;
    r = sexp_serialize_any(sexp, buffer);

    if (r.status == RESULT_ERROR)
        return result_vec_error(r.error);

    vec_push(buffer, "\0");
    vec_push(buffer, "\0");
    return result_vec_ok(buffer);
}

struct result_str sexp_serialize(const sexp *sexp) {
    struct result_vec r = sexp_serialize_vec(sexp);

    if (r.status == RESULT_ERROR)
        return result_str_error(r.error);

    // since the implementation vectors is hidden, we don't have a handle to all
    // the data allocated by vec.  We must copy the data over to a new region so
    // that the pointer we return can be free'ed directly.

    vector *v = r.ok;

    char *s = malloc(vec_len(v) * sizeof(char));
    if (s == NULL)
        return RESULT_MSG_ERROR(str, "malloc returned null");

    memcpy(s, vec_dat(v), vec_len(v));
    free_vector(v);

    return result_str_ok(s);
}

/************************ AUXILLIARY PRINTER FUNCTIONS ************************/
struct result_s32 sexp_fprint(const struct sexp *s, FILE *file){
    struct result_vec r = sexp_serialize_vec(s);
    if (r.status == RESULT_ERROR)
        return result_s32_error(r.error);

    vector *v = r.ok;
    const char *str = vec_dat(v);

    fwrite(str, vec_element_len(v), vec_len(v), file);
    free_vector(r.ok);

    return result_s32_ok(0);
}

s32 sexp_print(const struct sexp *s) {
    struct result_s32 r = 
        sexp_fprint(s, stdout);

    if (r.status == RESULT_ERROR) {
        char *msg = describe_error(r.error);
        puts(msg);

        free(msg);
        free_error(r.error);
        return -1;
    }

    return r.ok;
}

s32 sexp_println(const struct sexp *s) {
    s32 r = sexp_print(s);
    r += puts("\n");
    return r;
}
