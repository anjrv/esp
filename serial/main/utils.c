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
 * Converts a char* to an integer and returns its pointer 
 * 
 * @param str The string to be converted to an integer 
 * @return a pointer to the value of the parsed integer, 
 *         a null pointer if parsing was unsuccesful
 */
int* parse_int(char* str) {
    char *endptr;
    static int val = 0;

    if (strcmp(str, "0") == 0) {
        return (&val);
    }

    long long x = strtoll(str, &endptr, 10);
    if (x >= INT_MIN && x <= INT_MAX && endptr > str && x != 0) {
        val = x;
        return (&val);
    }

    return NULL;
}

/**
 * Converts a number to a char* representation
 * with an ending null terminator and returns
 * the start pointer
 * 
 * @param   x number to be converted
 * @return  A char array representation
 *          of x 
 */
char* long_to_string(long x) {
    static char buffer[17];

    snprintf(buffer, 10, "%ld", x);
    return buffer;
}

/**
 * Splits a given query char** into multiple char*
 * elements using the given delim argument
 * 
 * @param   query the char* to be split
 * @param   delim the delimiters to be used when splitting
 * @return  char** pointer to the start of the split char*
 */
char** string_split(char* query, char* delim) {
    static char store[MSG_BUFFER_LENGTH];
	strcpy(store, query);

    char** res = NULL;
    char* p = strtok(store, delim);
    int space = 0;

    while(p) {
        res = realloc(res, sizeof(char*) * ++space);
        res[space-1] = p;

        p = strtok(NULL, delim);
    }

    res = realloc(res, sizeof(char*) * (space+1));
    res[space] = '\0';

    return res;
}