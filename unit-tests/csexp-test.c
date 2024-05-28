#include "unit-test.h"

#include "csexp.h"
#include "nonstdint.h"
#include "vector.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

/**
 * reads in a test file into buffer.  every new line is cached in the newlines
 * vector.  Every line will have is newline character replaced with a null
 * terminator*/
s32 open_test_file(const char* filepath,
                    vector** buffer,
                    vector** newlines) {
    FILE* test_file = fopen(filepath, "r");
    
    if (test_file == NULL)  {
        printf("ERROR: couldn't open %s\n", filepath);
        return -1;
    }

    char* line_buffer = malloc(50);
    size_t line_buffer_len = 50;

    if (line_buffer == NULL) {
        printf("ERROR: couldn't allocate linebuffer.\n");
        fclose(test_file);
        return -1;
    }

    *buffer = make_vector(1, 1000);
    if (*buffer == NULL) {
        fclose(test_file);
        free(line_buffer);
        printf("ERROR: couldn't allocate file buffer\n");
        return -1;
    }
    
    *newlines = make_vector(sizeof(size_t), 100);
    if (*newlines == NULL) {
        fclose(test_file);
        free(line_buffer);
        free_vector(*buffer);
        printf("ERROR: couldn't allocate newline buffer\n");
        return -1;
    }

    s32 bytes_read;
    while (true) {
        // read a newline character
        bytes_read = getline(&line_buffer, &line_buffer_len, test_file);

        if (bytes_read <= 0)
            break;

        // store the newline
        size_t line_start = vec_len(*buffer);
        vec_push(*newlines, &line_start);

        // store the line in the buffer. replace the newlines character with
        // null terminator
        vec_pushn(*buffer, line_buffer, bytes_read);
        *(char*)vec_end(*buffer) = '\0';
    };

    fclose(test_file);
    return 0;
}

char* get_file_line(size_t line, vector* buffer, vector* line_cache) {
    return vec_ref(buffer, *(size_t*)vec_ref(line_cache, line));
}

struct test_case {
    const char* input;
    const char* assert;
};

const char* g_reader_test_filepath = "unit-tests/csexp-tests/reader-tests.org";
const char* g_reader_log_filepath  = "unit-tests/csexp-tests/reader-log.org";
vector *g_reader_test_file;
vector *g_reader_test_file_line_cache;

/**
 takes a line, and if it contains an org description list-item, it returns the
 description.

 For example, the following are all valid:
 
 * ITEM :: description
 - ANOTHER-ITEM :: description
   + ITEM 3     :: description

*/
const char* org_item_description(const char* line, const char* item) {
    while (isspace(*line)) line++;

    switch (*line) {
    case '-':
    case '+':
    case '*':
        line++;
        break;
    default:
        return NULL;
    }

    while (isspace(*line)) line++;


    for (const char* c = item; *c != '\0' && *line != '\0'; c++, line++) {
        if (*c != *line)
            return NULL;
    }
    
    while (isspace(*line)) line++;

    if (strncmp(line, "::", 2) != 0)
        return NULL;

    line += 2;

    while (isspace(*line)) line++;
    return line;
}

/**
 * test function.  this is highly stateful function.  It relies on the test
 * suite calling it once for every header in the test file.
 *
 * for testing cases where the reader should return NULL, the test file should
 * have the text <NULL> as the result.  While this would be a valid atom, it is
 * reserved for this purpose.
 */
const char* tst_reader(void) {
    static size_t current_line = 0;
    char* error_msg = NULL;

    FILE* log_file = fopen(g_reader_log_filepath, "a+");
    if (log_file == NULL) {
        return "COULDN'T OPEN LOG FILE";
    }

    vector* buffer = g_reader_test_file;
    vector* line_cache = g_reader_test_file_line_cache;

    // find the next heading
    for (; current_line < vec_len(line_cache); current_line++) {
        char* line = get_file_line(current_line, buffer, line_cache);

        if (strncmp(line, "* ", 2) == 0) {
            fputs(line, log_file);
            fputc('\n', log_file);
            
            current_line++;
            break;
        }
    }
    
    for (; current_line < vec_len(line_cache); current_line++) {
        char* line = get_file_line(current_line, buffer, line_cache);

        // if another heading is seen, stop.  The next call to this function
        // will pick up where it was left off.
        if (strncmp(line, "* ", 2) == 0)
            break;

        
        const char* input_str = org_item_description(line, "Input");
        if (input_str == NULL)
            continue;

        const char* assert_str;
        do {
            current_line++;
            line = get_file_line(current_line, buffer, line_cache);
            assert_str = org_item_description(line, "Assert");
        } while (assert_str == NULL);
        
        /// RUN TEST

        struct reader_result result = sexp_read(input_str, NULL, true);

        struct sexp* sexp = NULL;
        size_t sexp_length = 0;

        if (result.status == RESULT_OK)
            sexp_length = result.length;

        u8 sexp_data[sexp_length];

        if (result.status == RESULT_OK) {
            sexp = (struct sexp*)sexp_data;
            sexp_read(input_str, sexp, false);
        }
        
        // put the input into the logfile
        fprintf(log_file, "  - Input    :: %s\n", input_str);
        fprintf(log_file, "    + Assert :: %s\n", assert_str); 
        fprintf(log_file, "    + Return :: ");
        fflush(log_file);

        // find the position in the file before writing
        fseek(log_file, 0, SEEK_END);
        size_t start = ftell(log_file);

        // print the sexp (unless parsing failed)
        if (sexp != NULL) {
            sexp_fprint(sexp, log_file);
            fflush(log_file);
        } else {
            fprintf(log_file,
                    "<%s>\n", G_READER_RESULT_TYPE_STR[result.status]);
            
        }
        
        // find the end of the log file to determine how much data was written
        size_t end = ftell(log_file);
        fseek(log_file, start, SEEK_SET);

        // read in results from log file
        char printed_sexp[end-start];
        fread(printed_sexp, sizeof(char), end-start, log_file);

        // assert_str has a newline instead of a null terminator, so don't
        // compare that last character
        if (strncmp(printed_sexp, assert_str, strlen(assert_str)) != 0)
            error_msg = "bad reader value, check log file";

        // print error message on next line, after grabbing the output from the
        // log file.
        if (sexp == NULL && result.error_location != NULL) {
            fprintf(log_file, "    + Error  :: %s\n", result.error_location);
        }
        
        fputs("\n", log_file);
        fflush(log_file);

    }

    // find the next line that has a > on it.
    fclose(log_file);
    return error_msg;
}

void run_reader_test_suite() {    
    // open and load test file
    open_test_file(g_reader_test_filepath,
                   &g_reader_test_file,
                   &g_reader_test_file_line_cache);

    FILE* log_file = fopen(g_reader_log_filepath, "w");
    if (log_file == NULL) {
        printf("couldn't open log file to reset it!\n");
        return;
    }
    fclose(log_file);
    
    vector* buffer = g_reader_test_file;
    vector* line_cache = g_reader_test_file_line_cache;

    u32 headings = 0;
    // find all the headings
    for (size_t l = 0; l < vec_len(line_cache); l++) {
        char* line = get_file_line(l, buffer, line_cache);

        if (strncmp(line, "* ", 2) != 0)
            continue;

        headings++;
    }

    // populate the headings as the unit test names.
    struct test tests[headings];
    u32 heading = 0;
    for (size_t l = 0; l < vec_len(line_cache); l++) {
        char* line = get_file_line(l, buffer, line_cache);

        if (strncmp(line, "* ", 2) != 0)
            continue;

        tests[heading].name = line + 2;
        tests[heading].unit_test = &tst_reader;

        heading++;
        if (heading == headings)
            break;
    }

    run_test_suite(tests, headings, "CSEXP reader tests");

    free_vector(buffer);
    free_vector(line_cache);
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    run_reader_test_suite();

    return 0;
}
