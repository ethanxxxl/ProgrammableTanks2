#ifndef SEXP_H
#define SEXP_H

#include "sexp/sexp-base.h"
#include "sexp/sexp-io.h"
#include "sexp/sexp-utils.h"

void do_not_use_me_I_am_suppressing_warnings(void) {
    struct result_sexp test = sexp_read("(hello there)", SEXP_MEMORY_TREE);
    sexp_rcar(test);
    (void)test;
    return;
}

#endif
