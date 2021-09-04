#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>

#include "utils.h"

#define MSG_BUFFER_LENGTH 256

// These are assorted helper functions
// for use elsewhere in the project

char buffer[11];

/**
 * Converts a char* to an integer and returns its value
 * FAILURE: If an empty string is given or the value is
 *          not a number, the function returns 0
 * 
 * @param str The string to be converted to an integer 
 * @return a parsed integer if conversion was possible,
 *         0 if conversion was not possible 
 */
int parse_int(char* str) {
    char *endptr;
    long x = strtol(str, &endptr, 10);
    if (x >= INT_MIN && x <= INT_MAX && endptr > str) {
        int res = x;
        return res;
    }

    return 0;
}

/**
 * Converts an int to a char* representation
 * with an ending null terminator and returns
 * the start pointer
 * 
 * @param   x number to be converted
 * @return  A char array representation
 *          of x 
 */
char* int_to_string(int x) {
    snprintf(buffer, 10, "%d", x);
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
	char store[MSG_BUFFER_LENGTH];
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
    res[space] = 0;

    return res;
}