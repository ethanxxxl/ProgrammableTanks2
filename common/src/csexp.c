#include "csexp.h"

#include <ctype.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

const char* G_READER_RESULT_TYPE_STR[] = {
    FOR_EACH_RESULT_TYPE(GENERATE_STRING)
}; 

struct reader_result
result_err(enum reader_result_type error, const char* location) {
    return (struct reader_result){
        .status = error,
        .error_location = location};
}

struct reader_result
result_ok(size_t length) {
    return (struct reader_result){
        .status = RESULT_OK,
        .length = length};
}

////////////////////////////////////////////////////////////////////////////////
// SEXP READER FUNCTIONS ///////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

struct reader_result
sexp_read_atom(const char** caller_cursor, struct sexp* sexp, bool dryrun) {
    const char* cursor = *caller_cursor;
    while (isspace(*cursor) && *cursor != '\0') cursor++;

    if (memchr("\0])", *cursor, 3) != 0)
        return result_err(RESULT_ERR, cursor);

    // test for netstring
    const char* digit_end = cursor;
    u32 atom_len = strtoul((char*)cursor, (char**)&digit_end, 10);

    struct reader_result error;

    char* delims;
    u32 num_delims;

    enum atom_type {
        STRING,
        SPECIAL_SYMBOL,
        SYMBOL,
    } atom_type;
    
    if (digit_end != cursor && *digit_end == ':') {
        // this symbol is a netstring.
        if (!dryrun) {
            sexp->type = SEXP_SYMBOL;
            sexp->length = atom_len;
            memcpy(sexp->data, digit_end+1, atom_len);
        }

        *caller_cursor = digit_end + atom_len + 1;
        return result_ok(sizeof(struct sexp) + atom_len);
    }  else if (digit_end != cursor && (isspace(*digit_end) || *digit_end == ')')) {
        // symbol is a number
        if (!dryrun) {
            sexp->type = SEXP_INTEGER;
            sexp->length = sizeof(u32);
            *(u32*)sexp->data = atom_len;
        }

        *caller_cursor = digit_end;
        return result_ok(sizeof(struct sexp) + sizeof(u32));
    } else if (digit_end != cursor && *digit_end != ':') {
        return result_err(RESULT_NETSTRING_MISSING_COLON, digit_end);
        
    } else if (*digit_end == '|') {
        // symbol is an escaped symbol
        delims = "|";
        num_delims = 1;
            
        error.status = RESULT_SYMBOL_ESCAPE_NOT_CLOSED;
        cursor = digit_end+1;
        atom_type = SPECIAL_SYMBOL;
                    
    } else if (*digit_end == '"') {
        // symbol is a string
        delims = "\"";
        num_delims = 1;
        
        error.status = RESULT_QUOTE_NOT_CLOSED;
        cursor = digit_end+1;
        atom_type = STRING;
        
    } else {
        // regular symbol.
        delims = "\0 ()[]\"";
        num_delims = 7;
        atom_type = SYMBOL;
    }

    size_t i;
    for (i = 0;
         memchr(delims, *cursor, num_delims) == 0 && *cursor != '\0';
         cursor++, i++);

    // the string ended without finding a terminating delimeter.
    if (memchr(delims, *cursor, num_delims) == 0) {
        error.error_location = cursor;
        return error;
    }

    if (!dryrun) {
        switch (atom_type) {
        case SPECIAL_SYMBOL:
        case SYMBOL:
            sexp->type = SEXP_SYMBOL;
            break;
        case STRING:
            sexp->type = SEXP_STRING;
            break;
        }

        sexp->length = i;

        memcpy(sexp->data, cursor - i, i);
        if (atom_type == SYMBOL)
            for (u8* c = sexp->data; c < sexp->data + i; c++)
                *c = toupper(*c);
    }

    // skip past the terminator, unless this is a regular symbol.
    *caller_cursor = atom_type == SYMBOL ? cursor : cursor + 1;
    return result_ok(i + sizeof(struct sexp));
}

struct reader_result
sexp_read_tagged_atom(const char** caller_cursor,
                      struct sexp* sexp,
                      bool dryrun) {
    const char* cursor = *caller_cursor;

    // read the atom inside the tag and move the cursor past it.  This also puts
    // the atom in the sexp object.
    //
    // 1:SEXP_TAGGED_ATOM 4:LENGTH
    //         1:SEXP_ATOM 4:TAG_LENGTH TAG_LENGTH:TAG

    // ([ 3:foo ]3:bar)  ->  ([ 3:foo ]3:bar)
    // ~~⬆~~~~~~~~~~~~~  ->  ~~~~~~~~⬆~~~~~~~
    struct reader_result result;
    result = sexp_read_atom(&cursor,
                            sexp == NULL ? NULL : (struct sexp*)sexp->data,
                            dryrun);

    if (result.status == RESULT_ERR)
        return result_err(RESULT_TAG_MISSING_TAG, result.error_location);
    else if (result.status != RESULT_OK)
        return result;

    size_t tag_length = result.length;

    while(isspace(*cursor)) cursor++;

    // make sure the tag is closed.  Error if not.
    if (*cursor != ']')
        return result_err(RESULT_TAG_NOT_CLOSED, cursor);

    // move past the tag to the atom.
    // ([ 3:foo ]3:bar)  ->  ([ 3:foo ]3:bar)
    // ~~~~~~~~~⬆~~~~~~  ->  ~~~~~~~~~~⬆~~~~~
    cursor++;

    // read the atom associated with the tag.  This will move the cursor and
    // past the atom and put the atom on the sexp object.
    //
    // 1:SEXP_TAGGED_ATOM 4:LENGTH
    //         1:SEXP_ATOM 4:TAG_LENGTH TAG_LENGTH:TAG
    //         1:SEXP_ATOM 4:ATOM_LENGTH ATOM_LENGTH:ATOM
    //
    // ([ 3:foo ]3:bar)  ->  ([ 3:foo ]3:bar)
    // ~~~~~~~~~~⬆~~~~~  ->  ~~~~~~~~~~~~~~~⬆
    result = sexp_read_atom(&cursor,
                            sexp == NULL ? NULL :
                                        (struct sexp*)(sexp->data + tag_length),
                            dryrun);

    if (result.status == RESULT_ERR)
        return result_err(RESULT_TAG_MISSING_SYMBOL, result.error_location);
    else if (result.status != RESULT_OK)
        return result;

    size_t atom_length = result.length;

    if (!dryrun && sexp != NULL) {
        sexp->length = tag_length + atom_length;
        sexp->type = SEXP_TAG;        
    }

    // udate caller cursor, and return length in of sexp (in memory)
    *caller_cursor = cursor;
    return result_ok(sizeof(struct sexp) + tag_length + atom_length);
}

struct reader_result
sexp_reader(const char **sexp_str, struct sexp *sexp, bool dryrun);

// list points to the first item in the list
struct reader_result
sexp_read_list(const char** caller_cursor, struct sexp* sexp, bool dryrun) {
    const char* cursor = *caller_cursor;

    // list length in memory
    size_t list_length = 0;
    while (true) {
        while (isspace(*cursor) && *cursor != '\0') cursor++; // skip leading whitespace

        // see the the character under the cursor indicates the end of the list
        if (*cursor == ')') {
            if (!dryrun && sexp != NULL) {
                sexp->length = list_length;
                sexp->type = SEXP_CONS;
            }

            cursor++;
            *caller_cursor = cursor;
            return result_ok(list_length + sizeof(struct sexp));
        }
        if (*cursor == '\0')
            return result_err(RESULT_LIST_NOT_CLOSED, cursor);

        // read the next element in the list
        struct reader_result result;
        result = sexp_reader(&cursor,
                             sexp == NULL ? NULL : (struct sexp*)(sexp->data + list_length),
                             dryrun);
        
        if (result.status != RESULT_OK)
            return result;

        // append the length of that item to the total list length
        list_length += result.length;
    }
}

struct reader_result
sexp_reader(const char** caller_cursor, struct sexp* sexp, bool dryrun) {
    const char* cursor = *caller_cursor;
    // CURRENTLY ONLY SUPPORT CANONICAL TRANSPORT MODE
    while (isspace(*cursor) && *cursor != '\0') cursor++;

    struct reader_result result;
    switch (*cursor) {
    case '(':
        // these functions don't modify the data at sexp.
        cursor++;
        result = sexp_read_list(&cursor,
                                sexp,
                                dryrun);
        break;
    case '[':
        // these functions don't modify the data at
        cursor++;
        result = sexp_read_tagged_atom(&cursor,
                                       sexp,
                                       dryrun);
        break;

    case ']':
        return result_err(RESULT_INVALID_CHARACTER, cursor);

    default:
        // these functions don't modify the data at sexp.
        result = sexp_read_atom(&cursor,
                                sexp,
                                dryrun);
    }

    if (result.status != RESULT_OK)
        return result;

    *caller_cursor = cursor;
    return result;
}

struct reader_result
sexp_read(const char* sexp_str, struct sexp* sexp, bool dryrun) {
    while (isspace(*sexp_str) && *sexp_str != '\0') sexp_str++;
    if (*sexp_str == ')')
        return result_err(RESULT_INVALID_CHARACTER, p_str);

    struct reader_result result = sexp_reader(&sexp_str, sexp, dryrun);

    if (result.status != RESULT_OK)
        return result;
    
    // fail if their is trailing garbage.
    while (isspace(*sexp_str) && *sexp_str != '\0') sexp_str++;
    if (*sexp_str != '\0')
        return result_err(RESULT_TRAILING_GARBAGE, sexp_str);

    return result;
}

////////////////////////////////////////////////////////////////////////////////
// SEXP PRINTER FUNCTIONS //////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

const struct sexp*
sexp_tag_get_tag(const struct sexp* sexp) {
    if (sexp == NULL || sexp->type != SEXP_TAG)
        return NULL;

    return (struct sexp*)sexp->data;
}

const struct sexp*
sexp_tag_get_atom(const struct sexp* sexp) {
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
sexp_string_fprint(const struct sexp* sexp, FILE* f) {
    if (sexp == NULL || sexp->type != SEXP_STRING)
        return -1;

    fprintf(f, "\"%.*s\"", sexp->length, sexp->data);
    return 0;
}

s32
sexp_integer_fprint(const struct sexp* sexp, FILE* f) {
    if (sexp == NULL || sexp->type != SEXP_INTEGER)
        return -1;

    fprintf(f, "%d", *(u32*)sexp->data);
    return 0;
}

s32
sexp_symbol_fprint(const struct sexp* sexp, FILE* f) {
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
sexp_tagged_atom_fprint(const struct sexp* sexp, FILE* f) {
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
sexp_fprinter(const struct sexp*, FILE*);

s32
sexp_list_fprint(const struct sexp* sexp, FILE* f) {
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
sexp_fprinter(const struct sexp* sexp, FILE* f) {
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
sexp_fprint(const struct sexp* sexp, FILE* f) {
    s32 ret = sexp_fprinter(sexp, f);
    fputc('\n', f);
    return ret;
}


s32
sexp_print(const struct sexp* sexp) {
    return sexp_fprint(sexp, stdout);
}


s32
sexp_serialize_symbol(const struct sexp *sexp, char *buffer, size_t size) {
    if (sexp == NULL || sexp->type != SEXP_SYMBOL)
        return -1;

    return snprintf(buffer, size, "%.*s", sexp->length, sexp->data);
}

s32
sexp_serialize_tag(const struct sexp *sexp, char *buffer, size_t size) {
    if (sexp == NULL || sexp->type != SEXP_TAG)
        return -1;

    const struct sexp* tag = sexp_tag_get_tag(sexp);
    const struct sexp* atom = sexp_tag_get_atom(sexp);
    
    return snprintf(buffer, size, "[%.*s]%.*s",
                    tag->length, tag->data,
                    atom->length, atom->data);
}
    
s32
sexp_serialize_string(const struct sexp *sexp, char *buffer, size_t size) {
    if (sexp == NULL || sexp->type != SEXP_STRING)
        return -1;

    return snprintf(buffer, size, "\"%s\"", (char*)sexp->data);
}

s32
sexp_serialize_integer(const struct sexp *sexp, char *buffer, size_t size) {
    if (sexp == NULL || sexp->type != SEXP_INTEGER)
        return -1;

    return snprintf(buffer, size, "%d", *(u32*)sexp->data);
}

s32
sexp_serialize_list(const struct sexp *sexp, char *buffer, size_t size) {
    if (buffer == NULL && size != 0)
        return -1;
    
    if (sexp == NULL || sexp->type != SEXP_CONS)
        return -1;
    
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

s32
sexp_serialize(const struct sexp* sexp, char* buffer, size_t size) {
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

////////////////////////////////////////////////////////////////////////////////
// SEXP UTILITIES //////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

const struct sexp*
sexp_nth(const struct sexp* sexp, size_t n) {
    if (sexp == NULL || sexp->type != SEXP_CONS || sexp->length <= 0)
        return NULL;

    struct sexp* element = (struct sexp*)sexp->data;
    for (size_t i = 0; i < n; i++) {
        element = (struct sexp*)(element->data + element->length);
        if ((void*)element >= (void*)(sexp->data + sexp->length))
            return NULL;
    }

    return element;
}

struct sexp*
sexp_append(struct sexp* dst, const struct sexp* src) {
    if (dst == NULL || src == NULL || dst->type != SEXP_CONS)
        return NULL;

    struct sexp* element = (struct sexp*)(dst->data + dst->length);
    memcpy(element, src, sizeof(struct sexp) + src->length);

    dst->length += sizeof(struct sexp) + src->length;

    return element;
}

struct sexp*
sexp_append_dat(struct sexp* dst, void* dat, size_t len, enum sexp_type type) {
    if (dst == NULL || dst->type != SEXP_CONS)
        return NULL;

    if (type != SEXP_CONS && dat == NULL)
        return NULL;

    if (type == SEXP_INTEGER && len != sizeof(u32)) {
        printf("ERROR: SEXP unsupported integer size: %zu", len);
        return NULL;
    }

    struct sexp* element = (struct sexp*)(dst->data + dst->length);

    if (type != SEXP_CONS)
        memcpy(element->data, dat, len);

    element->length = len;
    element->type = type;

    dst->length += sizeof(struct sexp) + len;

    return element;
}

size_t
sexp_length(const struct sexp* sexp) {
    if (sexp == NULL || sexp->type != SEXP_CONS)
        return 0;

    
    struct sexp* element = (struct sexp*)(sexp->data);

    size_t n = 0;
    while (element < (struct sexp*)(sexp->data + sexp->length)) {
        n++;
        element = (struct sexp*)(element->data + element->length);
    }

    return n;
}


/*************************** Malloc Implementation ****************************/

struct sexp_dyn *sexp_to_dyn(const struct sexp *sexp) {
    switch (sexp->type) {
    case SEXP_CONS: {
        struct sexp_dyn *parent_cons = make_cons(NULL, NULL);
        struct cons *cons = &parent_cons->cons;

        // fill in list
        for (u32 n = 0; n < sexp_length(sexp); n++) {
            cons->car = sexp_to_dyn(sexp_nth(sexp, n));
            cons->cdr = make_cons(NULL, NULL);

            // go to next element in the list.
            cons = &cons->cdr->cons;
        }

        // terminate list with NIL
        struct sexp_dyn *nil = make_cons(NULL, NULL);
        nil->cons.car = nil;
        nil->cons.cdr = nil;
        
        cons->cdr = nil;

        return parent_cons;
    }
    case SEXP_INTEGER:
        return make_integer(*(u32*)sexp->data);
    case SEXP_STRING: 
        return make_string((char *)sexp->data);
    case SEXP_SYMBOL:
        return make_symbol((char *)sexp->data);
    case SEXP_TAG:
        return make_tag();
    }
}

// TODO fill this out.
struct sexp_static *sexp_from_dyn(struct sexp_dyn *sexp) {
    switch (sexp->type) {
    case SEXP_CONS: {
        
    }
    case SEXP_INTEGER:
    case SEXP_STRING:
    case SEXP_TAG:
    }
}


/* TODO add proper error handling */
struct sexp_dyn *sexp_dyn_read(char *str) {
    // READ IN THE SEXP
    struct reader_result result = sexp_read(str, NULL, true);
    if (result.status != RESULT_OK) {
        printf("ERROR: received following sexp_error: %s",
               G_READER_RESULT_TYPE_STR[result.status]);
        return NULL;
    }

    u8 sexp_buffer[result.length];
    struct sexp *sexp = (struct sexp *)sexp_buffer;
    sexp_read(str, sexp, false);

    // COPY OVER AND MAKE DYNAMIC
    return sexp_to_dyn(sexp);
}

struct sexp_dyn *make_cons(struct sexp_dyn *car, struct sexp_dyn *cdr) {
    struct sexp_dyn *sexp = malloc(sizeof(struct sexp_dyn));
    if (sexp == NULL) {
        return NULL;
    }
    
    sexp->type = SEXP_CONS;
    sexp->cons.car = car;
    sexp->cons.cdr = cdr;
    return sexp;
}

struct sexp_dyn *make_integer(s32 num) {
    struct sexp_dyn *sexp = malloc(sizeof(struct sexp_dyn));
    if (sexp == NULL) {
        return NULL;
    }

    sexp->type = SEXP_INTEGER;
    sexp->integer = num;
    return sexp;
}

struct sexp_dyn *make_symbol(char *symbol) {
    struct sexp_dyn *sym = make_string(symbol);
    sym->type = SEXP_SYMBOL;
    return sym;
}

struct sexp_dyn *make_string(char *str) {
    size_t length = strlen(str);
    struct sexp_dyn *sexp = malloc(sizeof(struct sexp_dyn) + length);
    if (sexp == NULL) {
        return NULL;
    }

    sexp->type = SEXP_STRING;
    memcpy(sexp->text, str, length);
    sexp->text_len = length;

    return sexp;
}

// TODO implement this
struct sexp_dyn *make_tag() {
    return NULL;
}

void free_sexp_dyn(struct sexp_dyn *sexp) {
    if (sexp->type == SEXP_CONS) {
        struct cons cons = sexp->cons;

        free(sexp);

        if (cons.car != sexp)
            free_sexp_dyn(cons.car);

        if (cons.cdr != sexp)
            free_sexp_dyn(cons.cdr);
        return;
    }

    free(sexp);
    return;
}
