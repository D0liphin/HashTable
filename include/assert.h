#include <stdbool.h>

/**
 * Make an assertion that a condition is true. This works on all opt levels. 
 * This will panic and point out the exact assertion that failed.
 */
#define ASSERT(COND) assert(__LINE__, __FILE__, #COND, COND)

/**
 * Terminate program if the condition is not true. The error message will 
 * contain the line number and file name where the assertion failed. You should
 * probably use `ASSERT()`.
 */
void assert(int line_nr, char const *filename, char const *assert_str, bool condition);

/**
 * Runs a test that should contain a number of assertions. e.g.
 * ```c
 * static void it_works();
 * 
 * int main()
 * {
 *         RUNTEST(it_works);
 * }
 * ```
 */
#define RUNTEST(TEST) run_test(#TEST, TEST)

/**
 * Run the specified test -- probably just use `RUN_TEST` 
 */
void run_test(char const *test_name, void (*testfn)());