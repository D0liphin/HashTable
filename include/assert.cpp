#include <cstdbool>
#include <cstdio>
#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <stdexcept>
#include "assert.h"

void panic(int line_nr, char const *filename, char const *format, ...)
{
#define STARS5 "*****"
#define STARS10 "**********"
#define STARS35 STARS10 STARS10 STARS10 STARS5
    va_list args;
    va_start(args, format);
    fprintf(stderr, "\e[0;31m" STARS35 "* panic! *" STARS35 "\e[0m\n");
    fprintf(stderr, "panicked at %s:%d\n", filename, line_nr);
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    va_end(args);
    throw std::runtime_error(":(");
}

void assert(int line_nr, char const *filename, char const *assert_str, bool condition)
{
    if (!condition) {
        panic(line_nr, filename, "assertion failed: %s", assert_str);
    }
}

void run_test(char const *test_name, void (*testfn)())
{
    testfn();
    printf("%s: \e[0;32msuccess!\e[0m\n", test_name);
}