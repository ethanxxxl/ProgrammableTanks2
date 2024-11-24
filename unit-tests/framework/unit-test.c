#include "unit-test.h"
#include "error.h"
#include "vector.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

const char GENERAL_ERROR[] = "GENERAL ERROR";
const char TEST_NOT_IMPLEMENTED_ERROR[] = "TEST NOT IMPLEMENTED";

struct result_void fail_msg(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    return result_void_error(vmake_msg_error(fmt, args));
}

/** returns a non-error status result struct. */
struct result_void no_error() {
    return result_void_ok(0);
}

// color codes, only visible in this file.
static const char CODE_CLEAR[]      = "\033[0m";
static const char CODE_INVERT[]     = "\033[7m";
static const char CODE_BOLD[]       = "\033[1m";
static const char CODE_BOLD_CLR[]   = "\033[22m";
static const char CODE_ITALIC[]     = "\033[3m";
static const char CODE_ITALIC_CLR[] = "\033[23m";
static const char CODE_BG_GREEN[]   = "\033[30;42m";
static const char CODE_BG_RED[] = "\033[30;41m";

void run_test_suite(const struct test tests[], size_t num_tests,
                    const char *test_suite_name) {
    // get length of left column
    size_t l_column_size = 0;

    int num_tests_tmp;
    if (num_tests > 0)
        num_tests_tmp = num_tests - 1;
    else
        num_tests_tmp = 0;
    
    do {
        num_tests_tmp /= 10;
        l_column_size++;
    } while (num_tests_tmp != 0);

    // get length of center column
    size_t c_column_size = 0;
    for (size_t i = 0; i < num_tests; i++) {
        size_t name_len = strlen(tests[i].name);
        
        c_column_size = name_len > c_column_size ? name_len : c_column_size;
    }

    // get the length of the right column
    const char passed_str[] = "passed";
    const char failed_str[] = "failed";
    const char pending_str[] = ".....";

    size_t r_column_size = 0;
    if (strlen(passed_str) > r_column_size)
        r_column_size = strlen(passed_str);

    if (strlen(failed_str) > r_column_size)
        r_column_size = strlen(failed_str);

    if (strlen(pending_str) > r_column_size)
        r_column_size = strlen(pending_str);

    if (strlen(test_suite_name) > r_column_size)
        r_column_size = strlen(test_suite_name);


    // print table header. the header label takes up the left and center columns
    // | RUNNING TEST SUITE:    | whatever_suite |
    // |  n1 | example          |         passed |
    // |  n2 | another example  |         failed |
    // | ... | ...              |         ...... |
    char table_header[] = "RUNNING TEST SUITE:";

    const int left_padding = 2;
    const int right_padding = 2;
    const int cell_padding = 1;
    
    // adjust the center column if necessary
    int lc_columns_total_space = l_column_size + cell_padding + c_column_size;
    int table_header_len = strlen(table_header);
    
    if (lc_columns_total_space < table_header_len)
        c_column_size += table_header_len - lc_columns_total_space;

    printf("%s%*s"    // CODE_INVERT and left padding,
           "%-*s"     // table header,
           "%*s"      // cell spacing,
           "%s%*s%s"  // CODE_BOLD, test suite name, CODE_BOLD_CLR,
           "%*s%s\n", // right padding and CODE_CLEAR
           CODE_INVERT, left_padding, "[",
           (int)(l_column_size + cell_padding + c_column_size), table_header,
           cell_padding, "",
           CODE_BOLD, (int)r_column_size, test_suite_name, CODE_BOLD_CLR,
           -right_padding, "]", CODE_CLEAR);

    struct vector* errors = make_vector(sizeof(struct failed_test), 4);
    
    // for each unit test, print out the name in the table without formatting.
    // then run the test. redraw the table row with the necessary highlighting.
    for (size_t i = 0; i < num_tests; i++) {
        // create backgrounds for table
        const char *line_bg_pass = CODE_BG_GREEN;
        char line_bg_fail[sizeof(CODE_BG_RED) + sizeof(CODE_BOLD)] = {0};
        strcat(line_bg_fail, CODE_BG_RED);
        strcat(line_bg_fail, CODE_BOLD);
        char line_bg_pending[] = "";

        char const* line_bg = line_bg_pending;
        char const* result_str = pending_str;
        
        printf("%s%*s%s"   // line_bg, left_padding, CODE_ITALIC,
               "%*zu%*s%s" // test index, cell padding, CODE_ITALIC_CLR,
               "%-*s%*s"   // test name, cell padding,
               "%*s%*s"    // test result, right padding,
               "%s",       // CODE_CLEAR
               line_bg, left_padding, "", CODE_ITALIC,
               (int)l_column_size, i, cell_padding, "", CODE_ITALIC_CLR,
               (int)c_column_size, tests[i].name, cell_padding, "",
               (int)r_column_size, result_str, right_padding, "",
               CODE_CLEAR);
        fflush(stdout);       

        // don't call null functions. They fail the test.
        if (tests[i].unit_test == NULL) {
            result_str = failed_str;
            line_bg    = line_bg_fail;
        } else {
            struct result_void result = tests[i].unit_test();

            if (result.status == RESULT_OK) {
                result_str = passed_str;
                line_bg = line_bg_pass;
            } else {
                result_str = failed_str;
                line_bg = line_bg_fail;

                struct failed_test failed_test = {
                    .num = (int)i,
                    .name = tests[i].name,
                    .error = result.error,
                };                

                vec_push(errors, &failed_test);
            }
        }
        
        printf("\033[0G"); // go back to beginning of line.
        printf("%s%*s%s"   // line_bg, left_padding, CODE_ITALIC,
               "%*zu%*s%s" // test index, cell padding, CODE_ITALIC_CLR,
               "%-*s%*s"   // test name, cell padding,
               "%*s%*s"    // test result, right padding,
               "%s\n",     // CODE_CLEAR
               line_bg, left_padding, "", CODE_ITALIC,
               (int)l_column_size, i, cell_padding, "", CODE_ITALIC_CLR,
               (int)c_column_size, tests[i].name, cell_padding, "",
               (int)r_column_size, result_str, right_padding, "",
               CODE_CLEAR);
    }

    // TODO format this nicely like you did above.
    if (vec_len(errors) == 0)
        printf("REPORT: all tests passsed!\n");
    else 
        printf("REPORT: %zu / %zu tests failed\n",
               vec_len(errors), num_tests);

    for (size_t i = 0; i < vec_len(errors); i++) {
        struct failed_test test;
        vec_at(errors, i, &test);

        char *err_msg = describe_error(test.error);
        printf("[%d] %s: %s\n", test.num, test.name, err_msg);

        free(err_msg);
        free_error(test.error);
    }

    printf("\n");

    free_vector(errors);
    return;
}
