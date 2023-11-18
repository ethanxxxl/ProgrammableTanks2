#ifndef DYN_STRING_H
#define DYN_STRING_H

struct dyn_string {
    char *c_str;
    int len;
    int _capacity;
};

// returns a dynamic string initialized to *c_str.  if c_str is NULL
// then, the resultant dyn_string isn't used.
struct dyn_string make_dyn_string(char *c_str);


#endif
