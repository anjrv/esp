#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>

#include "utils.h"

#define MSG_BUFFER_LENGTH 256

// These are assorted helper functions
// for use elsewhere in the project

/**
 * Converts a char* to an integer and stores the result
 * in the given pointer 
 * 
 * @param str The string to be converted to an integer 
 * @param ptr The pointer that will store the result
 */
int parse_int(char *str, int *ptr)
{
    char *endptr;

    if (strcmp(str, "0") == 0)
    {
        *ptr = 0;

        return 0;
    }

    long long x = strtoll(str, &endptr, 10);
    if (x >= INT_MIN && x <= INT_MAX && endptr > str && x != 0)
    {
        int res = x;
        *ptr = res;

        return 0;
    }

    return 1;
}

/**
 * Converts a number to a char* representation
 * with an ending null terminator and returns
 * the start pointer
 * 
 * NOTE: Used for serial response parsing,
 *       should not be used in parallel
 *       due to static storage
 * 
 * @param   x number to be converted
 * @return  A char array representation
 *          of x 
 */
char *long_to_string(long x)
{
    static char buffer[20];

    snprintf(buffer, 10, "%ld", x);
    return buffer;
}
