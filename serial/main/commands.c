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
char err_msg[MSG_BUFFER_LENGTH] = "no history";
char mac_addr[18] = { 0 };
char version[6] = { 0 };

/**
 * Used for setting the current error state
 * 
 * @param status    the status of the previous command 
 * @param command   the name of the previous command
 */
void set_error(char* status, char* command) {
    memset(err_msg, 0, MSG_BUFFER_LENGTH);

    if (strcmp(command, "") == 0) {
        strcpy(err_msg, status);
    } else {
        strcat(err_msg, status);
        strcat(err_msg, " on command - ");
        strcat(err_msg, command);
    }
}

/**
 * Returns information about the error state of the previous command
 * 
 * @return the error state of the previous command (if there was one)
 */
char* get_error() {
   return err_msg; 
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
 * @param num_args   number of delimiter split inputs
 * @param vars       the query variables provided to the device
 * @param dictionary the dictionary currently in use by the device,
 *                   this is initialized in the main function
 *                   in serial.c
 * 
 * @return the previously stored variable if there was one
 *         if there wasn't one returns "undefined" 
 */
char* command_store(int num_args, char** vars, dict* dictionary) {
    char* res;

    if (num_args == 3 && strlen(vars[1]) <= 16) {
        if (validate_name(vars[1])) {
            int *var;
            var = malloc(sizeof(*var));

            if (parse_int(vars[2], var)) {
                int *stored;
                stored = malloc(sizeof(*stored));

                if (query(dictionary, vars[1], stored)) {
                    res = long_to_string(*stored);
                } else {
                    res = "undefined";
                }

                store(dictionary, vars[1], *var);
                free(stored);
                free(var);
                set_error("success", "");
                return res;
            }

            free(var);
        }
    }

    res = "argument error";
    return res;
}

/**
 * Fetches a signed 32-bit integer from the device dictionary
 * 
 * @param num_args   number of delimiter split inputs
 * @param vars       the query variables provided to the device
 * @param dictionary the dictionary currently in use by the device,
 *                   this is initialized in the main function
 *                   in serial.c
 * 
 * @return the stored variable at the given name if there is one 
 *         if there isn't one returns "undefined" 
 */
char* command_query(int num_args, char** vars, dict* dictionary) {
    char* res;

    if (num_args == 2) {
        int *stored;

        stored = malloc(sizeof(*stored));
        if (query(dictionary, vars[1], stored)) {
            res = long_to_string(*stored);
            set_error("success","");
        } else {
            res = "undefined";
            set_error("error: undefined variable", "query");
        }

        free(stored);
        return res;
    }

    res = "argument error";
    return res;
}

/**
 * If there is space, pushes a signed 32-bit integer on to the device stack
 * 
 * @param num_args      number of delimiter split inputs
 * @param vars          the query variables provided to the device
 * @param stack_pointer the stack currently in use by the device,
 *                      this is initialized in the main function
 *                      in serial.c
 * 
 * @return "done" if the push succeeds, an error state otherwise 
 */
char* command_push(int num_args, char** vars, stack *stack_pointer) {
    if (num_args != 2) {
        return "argument error";
    } else if (is_stack_full(stack_pointer)) {
        set_error("error: stack overflow", "push");
        return "overflow";
    }

    int *value;
    value = malloc(sizeof(*value));

    if (parse_int(vars[1], value)) {
        push(stack_pointer, *value);
        free(value);
        set_error("success","");
        return "done";
    }

    return "argument error";
}

/**
 * If one valid defined value is given the function returns the outcome of
 * adding that value to the first value on the stack (non destructive)
 * 
 * If two valid defined values are given the function returns the outcome
 * of adding those t wo values together
 *  
 * @param num_args      number of delimiter split inputs
 * @param vars          the query variables provided to the device
 * @param stack_pointer stack currently in use by the device,
 *                      this is initialized in the main function
 *                      in serial.c
 * 
 * @return the outcome of the calculation if valid values are found 
 */
char* command_add(int num_args, char** vars, stack *stack_pointer) {
    if (num_args < 2) {
        set_error("error: undefined variable", "add");
        return "undefined";
    } 

    int *var1;
    var1 = malloc(sizeof(*var1));

    if (parse_int(vars[1], var1)) {
        int *var2;
        var2 = malloc(sizeof(*var2));

        if (num_args > 2 && num_args < 4) {
            if (!parse_int(vars[2], var2)) {
                return "argument error";
            }

            char* res = long_to_string(*var1 + *var2);

            set_error("success", "");
            free(var1);
            free(var2);
            return res;   
        } else if (is_stack_empty(stack_pointer)) {
            free(var1);
            free(var2);
            set_error("error: undefined variable, stack is empty", "add");
            return "undefined";
        } else {
            *var2 = peek(stack_pointer);
            char* res = long_to_string(*var1 + *var2);

            free(var1);
            free(var2);
            set_error("success", "");
            return res;   
        }
    } else {
        return "argument error";
    }

    set_error("error: undefined variable", "add");
    return "undefined";
}

/**
 * If there are values on the stack, pops the top of the stack
 * and returns the value. If there are no values left on the stack
 * the function returns "undefined"
 * 
 * @param num_args      number of delimiter split inputs
 * @param stack_pointer the stack currently in use by the device,
 *                      this is initialized in the main function
 *                      in serial.c
 * 
 * @return the value if the pop succeeds, an error state otherwise
 */
char* command_pop(stack *stack_pointer) {
    if (is_stack_empty(stack_pointer)) {
        set_error("error: undefined variable, stack is empty", "pop");
        return "undefined";
    } 

    int val  = pop(stack_pointer);
    char* res = long_to_string(val);

    set_error("success", "");
    return res;
}
