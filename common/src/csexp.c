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

struct reader_result
sexp_read_atom(const char** caller_cursor, struct sexp* sexp, bool dryrun) {
    const char* cursor = *caller_cursor;
    
    while (isspace(*cursor)) cursor++;
    if (*cursor == '\0')
        return result_ok(0);

    // ([3:gif] 250:xxxxxx 3:foo)
    // ~~~~~~~~~^~~~~~~~~~~~~~~~~
    const char* digit_end = cursor;
    u32 atom_len = strtoul((char*)cursor, (char**)&digit_end, 10);

    // strtoul will set digit_end to cursor if no conversion was performed.
    // that is, an invalid number is present.
    if (digit_end == cursor)
        return result_err(RESULT_BAD_NETSTRING_LENGTH, cursor);

    // ensure that the colon is present, then skip past it.
    // ([3:gif] 250:xxxxxx 3:foo)
    // ~~~~~~~~~~~~^~~~~~~~~~~~~~
    cursor = digit_end;
    if (*cursor != ':')
        return result_err(RESULT_NETSTRING_MISSING_COLON, cursor);
    cursor++;

    // if this isn't a dryrun...
    if (!dryrun && sexp != NULL) {
        sexp->length = atom_len;
        sexp->type = SEXP_ATOM;
        memcpy(sexp->data, cursor, atom_len);
    } else if (!dryrun && sexp == NULL) {
        return result_err(RESULT_NULL_SEXP_PARAMETER, NULL);
    }

    // return length of atom in memory, and update the caller cursor.
    // ([3:gif] 250:xxxxxx 3:foo)
    // ~~~~~~~~~~~~~~~~~~~^~~~~~~
    cursor += atom_len;
    *caller_cursor = cursor;
    return result_ok(sizeof(struct sexp) + atom_len);
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
    
    if (result.status != RESULT_OK)
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

    if (result.status != RESULT_OK)
        return result;

    size_t atom_length = result.length;

    if (!dryrun && sexp != NULL) {
        sexp->length = tag_length + atom_length;
        sexp->type = SEXP_TAGGED_ATOM;        
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
        while (isspace(*cursor)) cursor++; // skip leading whitespace

        // see the the character under the cursor indicates the end of the list
        if (*cursor == ')') {
            if (!dryrun && sexp != NULL) {
                sexp->length = list_length;
                sexp->type = SEXP_LIST;
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
    while (isspace(*cursor)) cursor++;

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
    while (isspace(*sexp_str)) sexp_str++;
    if (*sexp_str == ')')
        return result_err(RESULT_INVALID_CHARACTER, sexp_str);

    struct reader_result result = sexp_reader(&sexp_str, sexp, dryrun);

    if (result.status != RESULT_OK)
        return result;
    
    // fail if their is trailing garbage.
    while (isspace(*sexp_str)) sexp_str++;
    if (*sexp_str != '\0')
        return result_err(RESULT_TRAILING_GARBAGE, sexp_str);

    return result;
}

const struct sexp* sexp_nth(const struct sexp* sexp, size_t n) {
    if (sexp == NULL || sexp->type != SEXP_LIST || sexp->length <= 0)
        return NULL;

    struct sexp* element = (struct sexp*)sexp->data;
    for (size_t i = 0; i < n; i++) {
        element = (struct sexp*)(element->data + element->length);
        if ((void*)element >= (void*)(sexp->data + sexp->length))
            return NULL;
    }

    return element;
}

const struct sexp* sexp_tag_get_tag(const struct sexp* sexp) {
    if (sexp == NULL || sexp->type != SEXP_TAGGED_ATOM)
        return NULL;

    return (struct sexp*)sexp->data;
}

const struct sexp* sexp_tag_get_atom(const struct sexp* sexp) {
    if (sexp == NULL || sexp->type != SEXP_TAGGED_ATOM)
        return NULL;

    // ⬇~~~~~~~~~~~~~~
    // TYPE-TAG LENGTH
    //         TYPE-ATOM LENGTH CONTENTS
    //      -> TYPE-ATOM LENGTH CONTENTS
    const struct sexp* tag = sexp_tag_get_tag(sexp);
    return (struct sexp*)(tag->data + tag->length);
}

s32 sexp_atom_fprint(const struct sexp* sexp, FILE* f) {
    if (sexp == NULL || sexp->type != SEXP_ATOM)
        return -1;

    bool special_print = false;
    for (const u8* c = sexp->data;  c < sexp->data + sexp->length; c++) {
        if (isspace(*c) || islower(*c)) {
            special_print = true;
            break; 
        }
    }

    if (special_print)
        fprintf(f, "|%.*s|", sexp->length, sexp->data);
    else
        fprintf(f, "%.*s", sexp->length, sexp->data);

    return 0;
}

s32 sexp_tagged_atom_fprint(const struct sexp* sexp, FILE* f) {
    if (sexp == NULL || sexp->type != SEXP_TAGGED_ATOM)
        return -1;

    s32 ret;

    fputc('[', f);
    const struct sexp* tag = sexp_tag_get_tag(sexp);
    ret = sexp_atom_fprint(tag, f);
    if (ret == -1)
        return ret;
    
    fputc(']', f);

    const struct sexp* atom = sexp_tag_get_atom(sexp);
    ret = sexp_atom_fprint(atom, f);
    if (ret == -1)
        return ret;

    return 0;
}

s32 sexp_fprinter(const struct sexp*, FILE*);

s32 sexp_list_fprint(const struct sexp* sexp, FILE* f) {
    if (sexp == NULL || sexp->type != SEXP_LIST)
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

s32 sexp_fprinter(const struct sexp* sexp, FILE* f) {
    if (sexp == NULL)
        return -1;
    
    s32 ret;
    switch (sexp->type) {
    case SEXP_ATOM:
        ret = sexp_atom_fprint(sexp, f);
        break;
    case SEXP_TAGGED_ATOM: 
        ret = sexp_tagged_atom_fprint(sexp, f);
        break;
    case SEXP_LIST:
        ret = sexp_list_fprint(sexp, f);
        break;
    }

    return ret;
}

s32 sexp_fprint(const struct sexp* sexp, FILE* f) {
    s32 ret = sexp_fprinter(sexp, f);
    fputc('\n', f);
    return ret;
}


s32 sexp_print(const struct sexp* sexp) {
    return sexp_fprint(sexp, stdout);
}
