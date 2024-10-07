#include "sexp/sexp-base.h"
#include "error.h"

const char* G_READER_RESULT_TYPE_STR[] = {
    FOR_EACH_RESULT_TYPE(GENERATE_STRING)
};

struct result_sexp reader_err(enum sexp_reader_error_code code,
                              const char *input,
                              const char* location) {
    return (struct result_sexp){
        .status = RESULT_ERROR,
        .error = sexp_reader_error(code, input, location),
    };
}
