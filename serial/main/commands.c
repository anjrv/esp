#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>

#include "esp_wifi.h"
#include "version.h"
#include "dict.h"
#include "stack.h"
#include "utils.h"
#include "commands.h"

#define MSG_BUFFER_LENGTH 256

// These are the command endpoints
// routed to by the respond function
// in serial.c
//
// Generally these set the correct error state
// and return the respective response string
// for the command in question

// NOTE: Any incoming number of arguments
// will be offset +1 as that would be the
// command String this should match the
// index for vars

// Non local variables
char error[MSG_BUFFER_LENGTH] = "no command history";
char mac_addr[18] = { 0 };
char version[6] = { 0 };

/**
 * Used for setting the current error state
 * 
 * @param status    the status of the previous command 
 * @param command   the name of the previous command
 */
void set_error(char* status, char* command) {
    memset(error, 0, MSG_BUFFER_LENGTH);

    if (strcmp(command, "") == 0) {
        strcpy(error, status);
    } else {
        strcat(error, command);
        strcat(error, ": ");
        strcat(error, status);
    }
}

/**
 * Returns information about the error state of the previous command
 * 
 * @return the error state of the previous command (if there was one)
 */
char* get_error() {
   return error; 
}

/**
 * Returns "pong"
 * 
 * @return "pong"
 */
char* command_ping() {
    set_error("success", "");
    return "pong";
}

/**
 * Returns the mac address of the device 
 * Leading zeroes are left in
 * 
 * @return the mac address of the device
 */
char* command_mac() {
    uint8_t mac[6];
    esp_efuse_mac_get_default(mac);
    sprintf(
        mac_addr,
        "%02X:%02X:%02X:%02X:%02X:%02X",
        mac[0],
        mac[1],
        mac[2],
        mac[3],
        mac[4],
        mac[5]
    );

    set_error("success", "");
    return mac_addr;
}

/**
 * Returns the id of the developer
 * 
 * @return the id of the developer
 */
char* command_id() {
    set_error("success", "");
    return DEVELOPER_ID;
}

/**
 * Returns the firmware version of the device
 * defined in version.h
 * 
 * @return the firmware version of the device 
 */
char* command_version() {
    sprintf(
        version,
        "%d.%d.%d",
        MAJOR,
        MINOR,
        REVISION
    );

    set_error("success", "");
    return version;
}

int validate_name(char* key) {
    regex_t re;

    const char* pattern = "[a-zA-Z_]+";
    // "[a-z_A-Z]{1,16} refuses to check count??
    
    if (regcomp(&re, pattern, REG_EXTENDED) != 0) {
        return 0;
    }

    int status = regexec(&re, key, (size_t)0, NULL, 0);
    regfree(&re);

    if (status != 0) { 
        return 0;
    }

    return 1;
}

/**
 * Stores a signed 32-bit integer onto the device dictionary
 * 
 * @param num_args  number of delimiter split inputs
 * @param vars      the query variables provided to the device
 * @param d         the dictionary currently in use by the device,
 *                  this is initialized in the main function
 *                  in serial.c
 * 
 * @return the previously stored variable if there was one
 *         if there wasn't one returns "undefined" 
 */
char* command_store(int num_args, char** vars, dict* d) {
    char* res;

    if (num_args > 2 && strlen(vars[1]) <= 16) {
        if (validate_name(vars[1])) {
            int *var;
            var = malloc(sizeof(*var));
            parse_int(vars[2], var);

            if (var) {
                int *stored;
                stored = malloc(sizeof(*stored));
                if (query(d, vars[1], stored)) {
                    res = long_to_string(*stored);
                } else {
                    res = "undefined";
                }

                store(d, vars[1], *var);

                free(stored);
                free(var);
                set_error("success", "");
                return res;
            }
        }
    }

    // Insufficient arguments given or invalid name
    res = "argument error";
    set_error("argument error", "store");
    return res;
}

/**
 * Fetches a signed 32-bit integer from the device dictionary
 * 
 * @param num_args  number of delimiter split inputs
 * @param vars      the query variables provided to the device
 * @param d         the dictionary currently in use by the device,
 *                  this is initialized in the main function
 *                  in serial.c
 * 
 * @return the stored variable at the given name if there is one 
 *         if there isn't one returns "undefined" 
 */
char* command_query(int num_args, char** vars, dict* d) {
    char* res;
    int *stored;

    stored = malloc(sizeof(*stored));
    if (query(d, vars[1], stored)) {
        res = long_to_string(*stored);
        set_error("success","");
    } else {
        res = "undefined";
        set_error("undefined", "query");
    }

    free(stored);
    return res;
}

/**
 * If there is space, pushes a signed 32-bit integer on to the device stack
 * 
 * @param num_args  number of delimiter split inputs
 * @param vars      the query variables provided to the device
 * @param pt        the stack currently in use by the device,
 *                  this is initialized in the main function
 *                  in serial.c
 * 
 * @return "done" if the push succeeds, an error state otherwise 
 */
char* command_push(int num_args, char** vars, stack *pt) {
    if (num_args < 2) {
        set_error("argument error", "push");
        return "argument error";
    } else if (is_stack_full(pt)) {
        set_error("overflow", "push");
        return "overflow";
    }

    int *res;
    res = malloc(sizeof(*res));
    parse_int(vars[1], res);

    if (res) {
        push(pt, *res);
        free(res);
        set_error("success","");
        return "done";
    }

    // Catchall return for NaN
    set_error("argument error", "push");
    return "argument error";
}

/**
 * If one valid defined value is given the function returns the outcome of
 * adding that value to the first value on the stack (non destructive)
 * 
 * If two valid defined values are given the function returns the outcome
 * of adding those t wo values together
 *  
 * @param num_args  number of delimiter split inputs
 * @param vars      the query variables provided to the device
 * @param pt        the stack currently in use by the device,
 *                  this is initialized in the main function
 *                  in serial.c
 * 
 * @return the outcome of the calculation if valid values are found 
 */
char* command_add(int num_args, char** vars, stack *pt) {
    if (num_args < 2) {
        set_error("undefined", "add");
        return "undefined";
    } 

    int *var1;
    var1 = malloc(sizeof(*var1));
    parse_int(vars[1], var1);

    if (var1) {
        int *var2;
        var2 = malloc(sizeof(*var2));

        if (num_args > 2) {
            parse_int(vars[2], var2);

            if (!var2) {
                set_error("argument error", "add");
                return "argument error";
            }

            char* res = long_to_string(*var1 + *var2);

            set_error("success", "");
            free(var1);
            free(var2);
            return res;   
        } else if (!is_stack_empty(pt)) {
            *var2 = peek(pt);
            char* res = long_to_string(*var1 + *var2);

            free(var1);
            free(var2);
            set_error("success", "");
            return res;   
        }
    } else {
        set_error("argument error", "add");
        return "argument error";
    }

    set_error("undefined", "add");
    return "undefined";
}

/**
 * If there are values on the stack, pops the top of the stack
 * and returns the value. If there are no values left on the stack
 * the function returns "undefined"
 * 
 * @param num_args  number of delimiter split inputs
 * @param pt        the stack currently in use by the device,
 *                  this is initialized in the main function
 *                  in serial.c
 * 
 * @return the value if the pop succeeds, an error state otherwise
 */
char* command_pop(stack *pt) {
    if (is_stack_empty(pt)) {
        set_error("undefined", "pop");
        return "undefined";
    } 

    int val  = pop(pt);
    char* res = long_to_string(val);

    set_error("success", "");
    return res;
}
