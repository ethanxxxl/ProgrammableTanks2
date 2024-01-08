#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "vector.h"

/**
 * Main structure for unit testing.
 *
 * Associates a given test with a name. In the future, other data may be added,
 * but as of now, this simple structure will suffice.
 *
 * the function will return a pointer to an error message (c-str) that describes
 * how the test failed, or NULL.
 */
struct test {
    char* name;
    const char* (*unit_test)(void);
};

/**
 * A common error type for unit tests to return in general cases.
 */
const char GENERAL_ERROR[] = "GENERAL ERROR";
const char TEST_NOT_IMPLEMENTED_ERROR[] = "TEST NOT IMPLEMENTED";

struct failed_test {
    int num;
    const char* name;
    const char* error_message;
};

// color codes, only visible in this file.
static const char CODE_CLEAR[]      = "\033[0m";
static const char CODE_INVERT[]     = "\033[7m";
static const char CODE_BOLD[]       = "\033[1m";
static const char CODE_BOLD_CLR[]   = "\033[21m";
static const char CODE_ITALIC[]     = "\033[3m";
static const char CODE_ITALIC_CLR[] = "\033[23m";
static const char CODE_BG_GREEN[]   = "\033[30;42m";
static const char CODE_BG_RED[]     = "\033[30;41m";

/**
 * "main" function for all test suites.
 *
 * As tests in `tests` are run, they will be printed out in a neatly formatted
 * and aligned table. To indicate progress, before a test is ran, an
 * unhighlighted entry in the table is appended. Once the test is finished, that
 * entry is updated to match the test result.
 *
 * @param[in] tests an array of test structurs to be run.
 * @param[in] num_tests number of tests within `tests`
 * @param[in] test_suite_name name of the entire test suite. will be drawn in
 *            the table header.
 *
 * @return None.
 */
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

    struct vector errors;
    make_vector(&errors, sizeof(struct failed_test), 4);
    
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
        printf("\033[0G"); // go back to beginning of line.
        fflush(stdout);       

        // don't call null functions. They fail the test.
        if (tests[i].unit_test == NULL) {
            result_str = failed_str;
            line_bg    = line_bg_fail;
        } else {
            const char *result = tests[i].unit_test();
            result_str = (result == NULL) ? passed_str : failed_str;
            line_bg    = (result == NULL) ? line_bg_pass : line_bg_fail;

            struct failed_test failed_test = { (int)i, tests[i].name, result };
            if (result != NULL)
                vec_push(&errors, &failed_test);
        }
        
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
    if (errors.len == 0)
        printf("REPORT: all tests passsed!\n");
    else 
        printf("REPORT: %zu / %zu tests failed\n",
               errors.len, num_tests);

    for (size_t i = 0; i < errors.len; i++) {
        struct failed_test test;
        vec_at(&errors, i, &test);
        printf("[%d] %s: %s\n", test.num, test.name, test.error_message);
    }

    printf("\n");

    free_vector(&errors);
    return;
}
