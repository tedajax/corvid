#include <stdio.h>

void _corvid_internal_print_assert(const char* exp, const char* msg)
{
    fprintf(stderr, "\n[ASSERTION FAILED]  (%s) : %s\n\n", exp, msg);
}