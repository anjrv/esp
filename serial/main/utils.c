#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "utils.h"

#define MSG_BUFFER_LENGTH 256

// These are assorted helper functions
// for use elsewhere in the project

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
    int digit = log10(x) + 1;

    char* arr;
    char arr1[digit];
    arr = (char*)malloc(digit);
 
    int index = 0;
    while (x) {
        arr1[++index] = x % 10 + '0';
        x /= 10;
    }
 
    int i;
    for (i = 0; i < index; i++) {
        arr[i] = arr1[index - i];
    }
 
    arr[i] = '\0';
 
    return (char*)arr;
}

/**
 * Splits a given query char** into multiple char*
 * elements using the given delim argument
 * 
 * @param   query the char* to be split
 * @param   delim the delimiters to be used when splitting
 * @return  char** referring to the start of the split char*
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