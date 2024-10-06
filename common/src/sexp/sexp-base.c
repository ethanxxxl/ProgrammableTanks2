#include "sexp/sexp-base.h"

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
