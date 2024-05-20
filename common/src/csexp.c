#include "csexp.h"
#include "vector.h"

#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// pushes the prefix for a sexp item onto the sexp object.  Returns the index
// to the start of that item.
size_t sexp_push_prefix(vector* sexp, enum sexp_type type, u32 length) {
    size_t start = vec_len(sexp);

    struct sexp_prefix prefix = { type, length };
    vec_pushn(sexp, &prefix, sizeof(struct sexp_prefix));

    return start;
}

struct sexp_prefix* sexp_get_prefix(vector* sexp, size_t sexp_offset) {
    return (struct sexp_prefix*)vec_ref(sexp, sexp_offset);
}

s64 sexp_read_atom(u8** atom, vector* sexp) {
    u8* cursor = *atom;
    size_t sexp_start = vec_len(sexp);

    while (isspace(*cursor)) cursor++;
    if (*cursor == '\0')
        return 0;

    // ([3:gif] 250:xxxxxx 3:foo)
    // ~~~~~~~~~^~~~~~~~~~~~~~~~~
    u8* digit_end = cursor;
    u32 atom_len = strtoul((char*)cursor, (char**)&digit_end, 10);

    // strtoul will set digit_end to cursor if no conversion was performed.
    // that is, an invalid number is present.
    if (digit_end == cursor)
        return -1;

    // ensure that the colon is present, then skip past it.
    // ([3:gif] 250:xxxxxx 3:foo)
    // ~~~~~~~~~~~~^~~~~~~~~~~~~~
    cursor = digit_end;
    if (*cursor != ':')
        return -1;
    cursor++;

    // add the data to the sexp object
    sexp_push_prefix(sexp, SEXP_ATOM, atom_len);
    vec_pushn(sexp, cursor, atom_len);

    // update the cursor for subsequent calls
    // ([3:gif] 250:xxxxxx 3:foo)
    // ~~~~~~~~~~~~~~~~~~~^~~~~~~
    cursor += atom_len;
    *atom = cursor;
    
    return vec_len(sexp) - sexp_start;
}

s64 sexp_read_tagged_atom(u8** tag, vector* sexp) {
    size_t sexp_start = vec_len(sexp);
    u8* cursor = *tag;

    // push the prefix on now, before pushing the tag and atom on.
    // we will have to update it later.
    size_t prefix_idx = sexp_push_prefix(sexp, SEXP_TAGGED_ATOM, 0);
    
    // read the atom inside the tag and move the cursor past it.  This also puts
    // the atom in the sexp object.
    //
    // 1:SEXP_TAGGED_ATOM 4:LENGTH
    //         1:SEXP_ATOM 4:TAG_LENGTH TAG_LENGTH:TAG
    
    // ([3:foo]3:bar)  ->  ([3:foo]3:bar)
    // ~~⬆~~~~~~~~~~~  ->  ~~~~~~~⬆~~~~~~
    s64 tag_length = sexp_read_atom(&cursor, sexp);
    if (tag_length < 0)
        return -1;

    // make sure the tag is closed.  Error if not.
    while(isspace(*cursor)) cursor++;
    if (*cursor != ']')
        return -1;

    // move past the tag to the atom.
    // ([3:foo]3:bar)  ->  ([3:foo]3:bar)
    // ~~~~~~~⬆~~~~~~  ->  ~~~~~~~~⬆~~~~~
    cursor++;

    // read the atom associated with the tag.  This will move the cursor and
    // past the atom and put the atom on the sexp object.
    //
    // 1:SEXP_TAGGED_ATOM 4:LENGTH
    //         1:SEXP_ATOM 4:TAG_LENGTH TAG_LENGTH:TAG
    //         1:SEXP_ATOM 4:ATOM_LENGTH ATOM_LENGTH:ATOM
    //
    // ([3:foo]3:bar)  ->  ([3:foo]3:bar)
    // ~~~~~~~~⬆~~~~~  ->  ~~~~~~~~~~~~~⬆
    s64 atom_length = sexp_read_atom(&cursor, sexp);
    if (atom_length < 0)
        return -1;

    u32 total_length = tag_length + atom_length;

    // update total length in object.
    sexp_get_prefix(sexp, prefix_idx)->length = total_length;

    // update the callers cursor
    *tag = cursor;
    return vec_len(sexp) - sexp_start;
}


// list points to the first item in the list
s64 sexp_read_list(u8** list, vector* sexp) {
    u8* cursor = *list;

    size_t list_prefix = sexp_push_prefix(sexp, SEXP_LIST, 0);
    
    size_t sexp_start = vec_len(sexp);
    
    while (true) {
        while (isspace(*cursor)) cursor++; // skip leading whitespace

        s64 item_length;
        
        switch (*cursor) {
        case '(':
            // ITEM IS A LIST
            cursor++; // point to first item in sub-list
            item_length = sexp_read_list(&cursor, sexp);
            break;
        case '[':
            // ITEM IS A TAGGED ATOM
            cursor++;
            item_length = sexp_read_tagged_atom(&cursor, sexp);
            break;
            
        case ')':
            // RETURN CONDITION
            vec_push(sexp, "\0");
            *list = cursor+1; // set the callers cursor to their next item.

            // update list length tracker
            sexp_get_prefix(sexp, list_prefix)->length =
                vec_len(sexp) - sexp_start;

            return vec_len(sexp) - sexp_start;
        case '\0':
            return -1;

        default:
            // ITEM IS AN ATOM
            item_length = sexp_read_atom(&cursor, sexp);
        }
        
        if (item_length < 0 || item_length > 0xffffffff)
            return -1;
    }
}


vector* sexp_read(const u8* sexp_str) {
    vector* sexp = make_vector(sizeof(u8), 100);
    
    // CURRENTLY ONLY SUPPORT CANONICAL TRANSPORT MODE
    while (isspace(*sexp_str)) sexp_str++;

    switch (*sexp_str) {
    case '(':
        // these functions don't modify the data at sexp.
        sexp_str++;
        sexp_read_list((u8**)&sexp_str, sexp);
        break;
    case '[':
        // these functions don't modify the data at sexp.
        sexp_str++;
        sexp_read_tagged_atom((u8**)&sexp_str, sexp);
        break;

    case ')':
    case ']':
        goto exit_bad_format;

    default:
        // these functions don't modify the data at sexp.
        sexp_read_atom((u8**)&sexp_str, sexp);
    }

    // fail if their is trailing garbage.
    while (isspace(*sexp_str)) sexp_str++;
    if (*sexp_str != '\0')
        goto exit_bad_format;
    
    return sexp;

 exit_bad_format:
    free_vector(sexp);
    return NULL;
 }


// returns the index of the next sexp in the object.
// TODO add bounds checking
size_t sexp_get_next_idx(vector* sexp, size_t sexp_idx) {
    return
        sexp_idx +
        sizeof(struct sexp_prefix) +
        sexp_get_prefix(sexp, sexp_idx)->length;
}

struct sexp_prefix* sexp_get_next_prefix(vector* sexp, size_t sexp_idx) {
    size_t next_idx = sexp_get_next_idx(sexp, sexp_idx);
    
    return (struct sexp_prefix*)vec_ref(sexp, next_idx);
}

// gets the index of the next sexp, if it is a list
size_t sexp_list_first(vector* sexp, size_t sexp_idx) {
    enum sexp_type type = sexp_get_prefix(sexp, sexp_idx)->type;

    if (type != SEXP_LIST)
        return sexp_idx;

    return sexp_idx + sizeof(struct sexp_prefix);
}

size_t sexp_tag_get_tag(vector* sexp, size_t sexp_idx) {
    enum sexp_type type = sexp_get_prefix(sexp, sexp_idx)->type;

    if (type != SEXP_TAGGED_ATOM)
        return sexp_idx;

    return sexp_idx + sizeof(struct sexp_prefix);
}

size_t sexp_tag_get_atom(vector* sexp, size_t sexp_idx) {
    struct sexp_prefix* prefix = sexp_get_prefix(sexp, sexp_idx);

    if (prefix->type != SEXP_TAGGED_ATOM)
        return sexp_idx;

    struct sexp_prefix* tag =
        sexp_get_prefix(sexp, sexp_tag_get_tag(sexp, sexp_idx));

    // ⬇~~~~~~~~~~~~~~
    // TYPE-TAG LENGTH
    //         TYPE-ATOM LENGTH CONTENTS
    //       ⮕TYPE-ATOM LENGTH CONTENTS

    // skip the space from the overarching tag type prefix, the tag prefix, and
    // the tag length.

    return sexp_idx + 2*sizeof(struct sexp_prefix) + tag->length;
}

size_t sexp_atom_fprint(FILE* f, vector* sexp, size_t idx) {
    size_t len = sexp_get_prefix(sexp, idx)->length;

    u8* atom_txt = vec_ref(sexp, idx + sizeof(struct sexp_prefix));
    
    char tmp[len];
    for (size_t i = 0; i < len; i++) tmp[i] = toupper(atom_txt[i]);

    fprintf(f, "%.*s", (s32)len, tmp);

    return sexp_get_next_idx(sexp, idx);
}

size_t sexp_tagged_atom_fprint(FILE* f, vector* sexp, size_t idx) {
    fprintf(f, "[");
    size_t tag_idx = sexp_tag_get_tag(sexp, idx);
    sexp_atom_fprint(f, sexp, tag_idx);
    fprintf(f, "]");

    size_t atom_idx = sexp_tag_get_atom(sexp, idx);
    sexp_atom_fprint(f, sexp, atom_idx);

    return sexp_get_next_idx(sexp, idx);
}

size_t sexp_list_fprint(FILE* f, vector* sexp, size_t idx) {
    fprintf(f, "(");

    size_t next_item = sexp_list_first(sexp, idx);
    while (true) {
        switch (sexp_get_prefix(sexp, next_item)->type) {
        case SEXP_ATOM:
            next_item = sexp_atom_fprint(f, sexp, next_item);
            break;
        case SEXP_TAGGED_ATOM:
            next_item = sexp_tagged_atom_fprint(f, sexp, next_item);
            break;
        case SEXP_LIST:
            next_item = sexp_list_fprint(f, sexp, next_item);
        break;
    }

        if (next_item >= idx + sexp_get_prefix(sexp, idx)->length)
            break;

        fprintf(f, " ");
    }

    fprintf(f, ")");
    return sexp_get_next_idx(sexp, idx);
}

void sexp_fprint(FILE* f, vector* sexp) {
    enum sexp_type type = sexp_get_prefix(sexp, 0)->type;

    switch (type) {
    case SEXP_ATOM:
        sexp_atom_fprint(f, sexp, 0);
        break;
    case SEXP_TAGGED_ATOM:
        sexp_tagged_atom_fprint(f, sexp, 0);
        break;
    case SEXP_LIST:
        sexp_list_fprint(f, sexp, 0);
        break;
    }

    fprintf(f, "\n");
}

void sexp_print(vector* sexp) {
    sexp_fprint(stdout, sexp);
}
