#include <time.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>
#include "misc.h"
#include "list.adt.h"
#include "string2.h"
#include "markdown.h"
#include "readline.h"

/*
`testcase_t` is a type for representing a testcase.

`name` means the name of this testcase.
`input` means the input to test.
`output` means the expected output for the `input`.
*/
typedef struct testcase {
   char *name;
   char *input;
   char *output;
} testcase_t;

extern const char escaper; /* defined in markdown.c */

/**
 * @brief `make_tclist` makes a list of testcases by parsing a text file
 *    consisting of testcases.
 * @param fp the stream to a text file consisting of testcases
 * @return a list consisting of `testcase_t` objects
 */
static list_t *make_tclist(FILE *fp);

/**
 * @brief `try_testcase` runs a testcase.
 * @param testcase a testcase to run
 * @return an `int` value representing the result:
 *    `-1`: not a valid testcase.
 *    ` 0`: a testcase failed.
 *    ` 1`: a testcase successful.
 */
static int try_testcase(const testcase_t *testcase);

/**
 * @brief `cleanup` frees `malloc`ed pointers which `testcase_t` objects inside `testcases` have.
 * @param data a testcase to clean
 * @param index This parameter is not used.
 * @note This function is used with `list_foreach`.
 *    That's why it has the unnecessary second parameter.
 */
static void cleanup(void *data, int index);

extern int perform_test(const char *filename) {
   FILE *fp = fopen(filename, "r");
   if (!fp) ERR2("failed to open %s.", filename);
   
   list_t *testcases = make_tclist(fp);
   int len_testcases = list_size(testcases);

   int ret = fclose(fp);
   if (ret == EOF) ERR2("failed to close %s.", filename);
   
   int count_passed = 0;
   int count_failed = 0;
   int count_non_tc = 0;

   for (int i = 0; i < len_testcases; i++) {
      testcase_t *testcase = list_peek(testcases, i);
      int result = try_testcase(testcase);
      switch (result) {
         case -1: count_non_tc++; break;
         case  0: count_failed++; break;
         case  1: count_passed++; break;
      }
   }

   int actual_len = len_testcases - count_non_tc;

   printf("\033[48;5;222;30m TEST RESULT\033[K\033[0m\n"
      "   \033[38;5;49;1mSUCCESS\033[0m %d   \033[91mFAILED\033[0m %d   TOTAL %d\n",
      count_passed, count_failed, actual_len);
   
   list_foreach(testcases, cleanup);
   list_destroy(testcases);

   return actual_len == count_passed ? EXIT_SUCCESS : EXIT_FAILURE;
}

/**
 * @brief `make_testcase` parses `line` and stores the result into a testcase object.
 * @param line a line to parse
 * @param line_num the line number of `line`
 * @return If the line is valid, it returns the pointer to a testcase filled with
 *    parsed results. Otherwise, it returns `NULL`.
 */
static testcase_t *make_testcase(char *line, int line_num);

static list_t *make_tclist(FILE *fp) {
   list_t *testcases;
   int line_num;
   
   testcases = list_create();

   line_num = 1;
   while (!feof(fp)) {
      char *line;
      testcase_t *testcase;

      line = readline(fp);
      testcase = make_testcase(line, line_num++);
      free(line);

      if (!testcase)
         continue;   /* an empty or comment line */

      list_push(testcases, testcase, sizeof *testcase);  /* copies testcase */
      free(testcase);   /* frees the original */
   }

   return testcases;
}

/**
 * @brief `handle_notice` takes a line with '#' at its first
 *    character. If the second character is ' ', it skips that and
 *    copies the rest after that into the testcase. If not, it
 *    copies the whole line except the first '#'.
 * @param testcase a testcase to store the string
 * @param line a line which starts with '#'
 * @return It returns its first parameter, `testcase`.
 */
static testcase_t *handle_notice(testcase_t *testcase, const char *line);

/**
 * @brief `handle_invalid` stops the program, printing the
 *    error message which says line number N has wrong format
 *    and therefore this program is not able to interpret that.
 * @param line_num the number of a problematic line
 */
static void handle_invalid(int line_num);

/**
 * @brief `memberof_testcase` returns the pointer to the `index`th member of a `testcase_t` object.
 * @param testcase the pointer to a `testcase_t` object to get the member of
 * @param index an index of a member of a `testcase_t` object
 * @return a pointer to the requested member
 * @note If there is no such member, it stops the program.
 */
static char **memberof_testcase(testcase_t *testcase, int index);

static testcase_t *make_testcase(char *line, int line_num) {
   /*
   The format of a line: NAME|INPUT|OUTPUT
   - Any number of whitespaces can exist at the beginning
      & the end and between each field.
   - If the value of a field is quoted with '"', that
     quotation gets a removal.
   - Fields may have no value.
   - The following combinations are escaped:
      FROM   \|,  \t,  \n,  \v,  \f,  \r
        TO    |, 0x9, 0xA, 0xB, 0xC, 0xD
   - Comments starts with "//". No preceding space is allowed.
     (The reason being, string manipulation in C has been making my life stressful.)
   - In lines starting with '#', the part after '#' will
     be printed verbatim in a conspicuous manner and one
     '\n' is added at the end. However, if there are
     spaces between '#' and the content, only one space
     is removed. No preceding space is allowed. (The same reason.)
   - Empty lines are skipped.
   */
   
   testcase_t *testcase = salloc(sizeof *testcase);

   if (strlen(line) == 0)            /* an empty line */
      return NULL;
   if (strncmp(line, "//", 2) == 0)  /* a comment line */
      return NULL;
   if (line[0] == '#')               /* a notice line */
      return handle_notice(testcase, line);

   
   list_t *fields;
   int len_fields;

   fields = list_create();
   str2_split(fields, line, '|', '\\');

   len_fields = list_size(fields);
   if (len_fields != 3)
      handle_invalid(line_num);  /* the program exits */

   static const char *escapee = "|tnvfr";
   static const char *translations = "|\t\n\v\f\r";
   char *field, *trimmed, **member;

   for (int i = 0; i < len_fields; i++) {
      field = list_peek(fields, i);
      trimmed = str2_trim(field, '"');
      member = memberof_testcase(testcase, i);
      *member = str2_escape(trimmed, escaper, escapee, translations);
      free(trimmed);    /* *member is freed later. */
   }
   list_destroy(fields);

   return testcase;
}

static testcase_t *handle_notice(testcase_t *testcase, const char *line) {
   const char *notice;
   char *buf;

   notice = line[1] == ' ' ? &line[2] : &line[1];
   buf = salloc(strlen(notice) + 1);
   strcpy(buf, notice);
   testcase->name = NULL;
   testcase->output = buf;

   return testcase;
}

static void handle_invalid(int line_num) {
   static const char *err_interpret = "\033[48;5;152;30m SYNTAX ERROR \033[0m "
   "The line %d does not have a valid format; not able to interpret.\n";

   fprintf(stderr, err_interpret, line_num);
   exit(EXIT_FAILURE);
}

static char **memberof_testcase(testcase_t *testcase, int index) {
   switch (index) {
      case 0: return &testcase->name;
      case 1: return &testcase->input;
      case 2: return &testcase->output;
   }

   ERR2("struct testcase doesn't have %dth member.", index);
}

static int try_testcase(const testcase_t *testcase) {
   static const char *notice_success =
      "\033[48;5;148;30m %s   \033[0m \033[38;5;49;1m%s\033[0m";
   static const char *notice_failed =
      "\033[101;97m %s   \033[0m\033[100;97m%s\033[0m";
   static const char *notice_time =
      "   \033[90m(Parsed in %fs)\033[0m\n";

   /* this is not a testcase but a notice line. */
   if (testcase->name == NULL) {
      printf("\033[48;5;253;30m %s\033[K\033[0m\n", testcase->output);
      return -1;
   }

   clock_t initial = clock();
   char *translated = translate_markdown(testcase->input);
   clock_t final = clock();
   double elapsed = (final - initial) / (double) CLOCKS_PER_SEC;
   bool passed;

   if (strcmp(testcase->output, translated) == 0) {
      passed = true;
      printf(notice_success, testcase->name, "passed!!");
   }
   else {
      passed = false;
      printf(notice_failed, testcase->name, " failed... ");
   }

   printf(notice_time, elapsed);
   printf("   input           = [%s]\n", testcase->input);
   printf("   expected_output = [%s]\n", testcase->output);
   printf("   actual_output   = [%s]\n", translated);

   free(translated);

   return passed ? 1 : 0;
}

static void cleanup(void *data, int index) {
   testcase_t *tc = data;

   free(tc->output);
   if (!tc->name)
      return;
   free(tc->name);
   free(tc->input);
}