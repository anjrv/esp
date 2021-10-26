#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>

#include "utils.h"
#include "client.h"
#include "data_tasks.h"

#define MSG_BUFFER_LENGTH 256

// These are assorted helper functions
// for use elsewhere in the project

/**
 * Checks whether the BT_DEMO source is available
 * 
 * @return an int:
 *         1 if the connection to the BT_DEMO device/source is available
 *         0 if it is not
 */ 
int bt_demo_available()
{
    if (active_connection && strcmp(BT_DATA_SOURCE_DEVICE, BT_DEMO_SOURCE_DEVICE) == 0 && strcmp(BT_DATA_SOURCE_SERVICE, BT_DEMO_SOURCE_SERVICE) == 0)
        return 1;
    return 0;
}

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
