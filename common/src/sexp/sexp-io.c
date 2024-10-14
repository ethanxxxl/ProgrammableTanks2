#include "sexp/sexp-io.h"
#include "sexp/sexp-base.h"
#include "sexp/sexp-utils.h"

#include <ctype.h>
#include <string.h>
#include <stdlib.h>

/*************************** SEXP READER FUNCITONS ****************************/
/** reads an attom from the string and returns a pointer to the sexp. */
struct result_sexp
sexp_read_atom(const char **caller_cursor,
               sexp *parent,
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
    
    // Determine atom type and extract data.    
    if (digit_end != cursor && *digit_end == ':') {
        // NETSTRING
        atom_type = SEXP_SYMBOL;
        atom_data.str = digit_end+1;
        atom_length = atom_number_value;
        
        *caller_cursor = digit_end + atom_number_value + 1;
        
    }  else if (digit_end != cursor && (isspace(*digit_end) || *digit_end == ')')) {
        // NUMBER
        atom_type = SEXP_INTEGER;
        atom_data.integer = atom_number_value;
        atom_length = sizeof(s32);
        
        *caller_cursor = digit_end;

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
        atom_data.str = digit_end+1;
        atom_length = 0; // filled in next subsequent block
        
        delims = "\0 ()[]\"";
        num_delims = 7;
    }

    // Get lengths for stringy sexp types.
    if (atom_type == SEXP_SYMBOL || atom_type == SEXP_STRING) {
        for (atom_length = 0;
             memchr(delims, *cursor, num_delims) == NULL && *cursor != '\0';
             cursor++, atom_length++);

        // the string ended without finding a terminating delimeter.
        if (memchr(delims, *cursor, num_delims) == NULL) {
            return reader_err(error_code, *caller_cursor, cursor);
        }

        // skip past the terminator, unless this is a regular symbol.
        if (should_skip_terminator == true) {
            *caller_cursor = atom_type == SEXP_SYMBOL ? cursor : cursor + 1;        
        }
    }

    // BUG: This currently doesn't work for netstrings that have null characters
    // embedded in them!

    // copy atom data into temporary buffer (so that it is null terminated.)
    char tmp_data[atom_length + 1];
    memcpy(tmp_data, atom_data.str, atom_length);
    tmp_data[atom_length] = '\0';

    // make non-escaped symbols upper-case
    if (atom_type == SEXP_SYMBOL && symbol_is_escaped == true) {
        for (char *c = tmp_data; c < tmp_data + atom_length; c++) {
            *c = toupper(*c);
        }
    }

    // Create sexp and copy data
    if (parent == NULL) {
        return make_sexp(atom_type, method, tmp_data);
    }

    switch (atom_type) {
    case SEXP_INTEGER:
        return sexp_push_integer(parent, atom_data.integer);
    case SEXP_SYMBOL:
        return sexp_push_symbol(parent, tmp_data);
    case SEXP_STRING:
        return sexp_push_string(parent, tmp_data);
    default:
        return RESULT_MSG_ERROR(sexp, "Reader Feature Not Implmented");
    }
}

/** Reads a tag from the string and returns a pointer to that sexp.*/
struct result_sexp
sexp_read_tagged_atom(const char **caller_cursor,
                      sexp *parent,
                      enum sexp_memory_method method) {
    const char* cursor = *caller_cursor;

    sexp *toplevel_tag;

    struct result_sexp r;
    if (parent == NULL) {
        r = make_sexp(SEXP_TAG, method, NULL);

        if (r.status == RESULT_ERROR)
            return r;
        
        toplevel_tag = r.ok;
    } else {
        r = sexp_push_tag(parent);

        if (r.status == RESULT_ERROR)
            return r;
        
        toplevel_tag = r.ok;
    }
        
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
    r = sexp_read_atom(&cursor, toplevel_tag, method);
    if (r.status == RESULT_ERROR)
        return r;

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
    r = sexp_read_atom(&cursor, toplevel_tag, method);
    if (r.status == RESULT_ERROR)
        return r;

    // udate caller cursor, and return length in of sexp (in memory)
    *caller_cursor = cursor;
    return result_sexp_ok(toplevel_tag);
}

struct result_sexp
sexp_reader(const char **sexp_str,
            sexp *parent,
            enum sexp_memory_method method);

// list points to the first item in the list
struct result_sexp
sexp_read_list(const char **caller_cursor,
               sexp *parent,
               enum sexp_memory_method method) {
    const char* cursor = *caller_cursor;

    sexp *list;
    struct result_sexp r;
    if (parent == NULL) {
        r = make_sexp(SEXP_CONS, method, NULL);

        if (r.status == RESULT_ERROR)
            return r;

        list = r.ok;
    }

    while (true) {
        // skip leading whitespace
        while (isspace(*cursor) && *cursor != '\0') cursor++; 

        // see the the character under the cursor indicates the end of the list
        if (*cursor == ')') {
            cursor++;
            *caller_cursor = cursor;
            return result_sexp_ok(list);
        }
        if (*cursor == '\0')
            return reader_err(SEXP_RESULT_LIST_NOT_CLOSED, *caller_cursor, cursor);

        // read the next element and append it to list.
        r = sexp_reader(&cursor,
                        list,
                        method);
        
        if (r.status == RESULT_ERROR)
            return r;
    }
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
            sexp *parent,
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
        r = sexp_read_list(&cursor, parent, method);
        break;
    case TAGGED_ATOM:
        r = sexp_read_tagged_atom(&cursor, parent, method);
        break;
    case ATOM:
        r = sexp_read_atom(&cursor, parent, method);
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

    r = sexp_reader(&sexp_str, NULL, method);

    if (r.status == RESULT_ERROR)
        return r;
    
    // fail if their is trailing garbage.
    while (isspace(*cursor) && *cursor != '\0') cursor++;
    if (*cursor != '\0')
        return reader_err(SEXP_RESULT_TRAILING_GARBAGE, sexp_str, cursor);

    return r;
}

////////////////////////////////////////////////////////////////////////////////
// SEXP PRINTER FUNCTIONS //////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

const sexp*
sexp_tag_get_tag(const sexp *sexp) {
    if (sexp == NULL || sexp->type != SEXP_TAG)
        return NULL;

    return (struct sexp*)sexp->data;
}

const sexp*
sexp_tag_get_atom(const sexp *sexp) {
    if (sexp == NULL || sexp->type != SEXP_TAG)
        return NULL;

    // ⬇~~~~~~~~~~~~~~
    // TYPE-TAG LENGTH
    //         TYPE-ATOM LENGTH CONTENTS
    //      -> TYPE-ATOM LENGTH CONTENTS
    const struct sexp* tag = sexp_tag_get_tag(sexp);
    return (struct sexp*)(tag->data + tag->length);
}

s32
sexp_string_fprint(const sexp *sexp, FILE *f) {
    if (sexp == NULL || sexp->type != SEXP_STRING)
        return -1;

    fprintf(f, "\"%.*s\"", sexp->length, sexp->data);
    return 0;
}

s32
sexp_integer_fprint(const sexp *sexp, FILE *f) {
    if (sexp == NULL || sexp->type != SEXP_INTEGER)
        return -1;

    fprintf(f, "%d", *(u32*)sexp->data);
    return 0;
}

s32
sexp_symbol_fprint(const sexp *sexp, FILE *f) {
    if (sexp == NULL || sexp->type != SEXP_SYMBOL)
        return -1;

    enum print_type {
        PRINT_NORMAL,
        PRINT_SPECIAL,
        PRINT_NETSTRING,
    } print_type = PRINT_NORMAL;

    for (const u8* c = sexp->data;  c < sexp->data + sexp->length; c++) {
        if (isspace(*c) || islower(*c)) {
            print_type = PRINT_SPECIAL;
            break;
        }

        if (*c == '|') {
            print_type = PRINT_NETSTRING;
            break;
        }
    }

    switch (print_type) {
    case PRINT_NORMAL:
        fprintf(f, "%.*s", sexp->length, sexp->data);
        break;
    case PRINT_SPECIAL:
        fprintf(f, "|%.*s|", sexp->length, sexp->data);
        break;
    case PRINT_NETSTRING:
        fprintf(f, "%d:%.*s", sexp->length, sexp->length, sexp->data);
        break;
    }

    return 0;
}

s32
sexp_tagged_atom_fprint(const sexp *sexp, FILE *f) {
    if (sexp == NULL || sexp->type != SEXP_TAG)
        return -1;

    s32 ret;

    fputc('[', f);
    const struct sexp* tag = sexp_tag_get_tag(sexp);
    ret = sexp_symbol_fprint(tag, f);
    if (ret == -1)
        return ret;
    
    fputc(']', f);

    const struct sexp* atom = sexp_tag_get_atom(sexp);
    ret = sexp_symbol_fprint(atom, f);
    if (ret == -1)
        return ret;

    return 0;
}

s32
sexp_fprinter(const sexp*, FILE*);

s32
sexp_list_fprint(const sexp *sexp, FILE *f) {
    if (sexp == NULL || sexp->type != SEXP_CONS)
        return -1;
    if (sexp->length == 0) {
        fprintf(f, "()");
        return 0;
    }
        
    fprintf(f, "(");
    
    size_t n = 0;
    const struct sexp* element = sexp_nth(sexp, 0);
    while (true) {
        if (sexp_fprinter(element, f) == -1)
            return -1;

        element = sexp_nth(sexp, ++n);
        if (element == NULL)
            break;

        fputc(' ', f);
    }

    fputc(')', f);
    return 0;
}

s32
sexp_fprinter(const sexp *sexp, FILE *f) {
    if (sexp == NULL)
        return -1;
    
    s32 ret;
    switch (sexp->type) {
    case SEXP_SYMBOL:
        ret = sexp_symbol_fprint(sexp, f);
        break;
    case SEXP_INTEGER:
        ret = sexp_integer_fprint(sexp, f);
        break;
    case SEXP_STRING:
        ret = sexp_string_fprint(sexp, f);
        break;
    case SEXP_TAG: 
        ret = sexp_tagged_atom_fprint(sexp, f);
        break;
    case SEXP_CONS:
        ret = sexp_list_fprint(sexp, f);
        break;
    }

    return ret;
}

s32
sexp_fprint(const sexp *sexp, FILE *f) {
    s32 ret = sexp_fprinter(sexp, f);
    fputc('\n', f);
    return ret;
}


s32
sexp_print(const sexp *sexp) {
    return sexp_fprint(sexp, stdout);
}


s32
sexp_serialize_symbol(const sexp *sexp, char *buffer, size_t size) {
    if (sexp == NULL || sexp->type != SEXP_SYMBOL)
        return -1;

    return snprintf(buffer, size, "%.*s", sexp->length, sexp->data);
}

s32
sexp_serialize_tag(const sexp *sexp, char *buffer, size_t size) {
    if (sexp == NULL || sexp->type != SEXP_TAG)
        return -1;

    const struct sexp* tag = sexp_tag_get_tag(sexp);
    const struct sexp* atom = sexp_tag_get_atom(sexp);
    
    return snprintf(buffer, size, "[%.*s]%.*s",
                    tag->length, tag->data,
                    atom->length, atom->data);
}
    
s32
sexp_serialize_string(const sexp *sexp, char *buffer, size_t size) {
    if (sexp == NULL || sexp->type != SEXP_STRING)
        return -1;

    return snprintf(buffer, size, "\"%s\"", (char*)sexp->data);
}

s32
sexp_serialize_integer(const sexp *sexp, char *buffer, size_t size) {
    if (sexp == NULL || sexp->type != SEXP_INTEGER)
        return -1;

    return snprintf(buffer, size, "%d", *(u32*)sexp->data);
}

struct result_str
sexp_serialize_list(const sexp *sexp, char *buffer, size_t size) {
    if (buffer == NULL && size != 0)
        return make_generic_error("this is an error");
    
    if (sexp == NULL || sexp->type != SEXP_CONS)
        return make_generic_error("another error");
    
    if (sexp->length < sizeof(struct sexp))
        return snprintf(buffer, size, "()");

    size_t list_len = 1;
    snprintf(buffer, size, "(");
    
    struct sexp* element = (struct sexp*)sexp->data;
    while (element < (struct sexp*)(sexp->data + sexp->length)) {
        size_t tmp = 0;

        // case that nothing is written to buffer.
        if (size == 0  && buffer == NULL)
            tmp = sexp_serialize(element, NULL, 0);
        else if (list_len < size) 
            // don't write anything if there isn't enough space left in the buffer.
            tmp = sexp_serialize(element, buffer + list_len, size-list_len);
        else
            return list_len;

        if (tmp < 0)
            return -1;

        list_len += tmp;

        element = (struct sexp*)(element->data + element->length);

        if (element < (struct sexp*)(sexp->data + sexp->length)) {
            if (size != 0  && buffer != NULL && list_len < size)
                snprintf(buffer+list_len, size-list_len, " ");
            
            list_len++;
        }
    }

    if  (list_len < size)
        snprintf(buffer+list_len, size-list_len, ")");
    
    list_len += 1;
        
    return list_len;
}

char *_sexp_serialize(const sexp *sexp, char *buffer, size_t len, size_t cap) {
    
}


struct result_str sexp_serialize(const sexp *sexp) {
    // this function should return the number of bytes written to the buffer.
    // If buffer is NULL, then no bytes are written.  If buffer is NULL and size
    // is 0, no bytes are written, but the function will return the number of
    // bytes that would have been written if the buffer existed.
    
    if (sexp == NULL)
        return -1;

    switch (sexp->type) {
    case SEXP_CONS:
        return sexp_serialize_list(sexp, buffer, size);
    case SEXP_SYMBOL:
        return sexp_serialize_symbol(sexp, buffer, size);
    case SEXP_INTEGER:
        return sexp_serialize_integer(sexp, buffer, size);
    case SEXP_TAG:
        return sexp_serialize_tag(sexp, buffer, size);
    case SEXP_STRING:
        return sexp_serialize_string(sexp, buffer, size);
    }
}
