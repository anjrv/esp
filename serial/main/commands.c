#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>

#include "esp_wifi.h"
#include "version.h"
#include "dict.h"
#include "stack.h"
#include "utils.h"
#include "serial.h"
#include "commands.h"
#include "factors.h"

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

////////////////////////
// IMMEDIATE COMMANDS //
////////////////////////

/**
 * Prints information about the error state of the previous command
 */
void get_error() {
   serial_out(err_msg); 
}

/**
 * Prints "pong"
 */
void command_ping() {
    set_error("success", "");
    serial_out("pong");
}

/**
 * Prints the mac address of the device 
 * Leading zeroes are left in
 */
void command_mac() {
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
    serial_out(mac_addr);
}

/**
 * Prints the id of the developer
 */
void command_id() {
    set_error("success", "");
    serial_out(DEVELOPER_ID);
}

/**
 * Prints the firmware version of the device
 * defined in version.h
 */
void command_version() {
    sprintf(
        version,
        "%d.%d.%d",
        MAJOR,
        MINOR,
        REVISION
    );

    set_error("success", "");
    serial_out(version);
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
 */
void command_store(int num_args, char** vars, dict* dictionary) {
    char* res;

    if (num_args == 3 && strlen(vars[1]) <= 16) {
        if (validate_name(vars[1])) {
            int *var;
            var = malloc(sizeof(*var));

            if (parse_int(vars[2], var) == 0) {
                int *stored;
                stored = malloc(sizeof(*stored));

                if (query(dictionary, vars[1], stored) == 0) {
                    res = long_to_string(*stored);
                } else {
                    res = "undefined";
                }

                if (!is_dict_full(dictionary)) {
                    store(dictionary, vars[1], *var);
                    set_error("success", "");
                } else {
                    set_error("error: storage full", "store");
                    res = "storage exceeded";
                }


                free(stored);
                free(var);
                serial_out(res);
                return;
            }

            free(var);
        }
    }

    serial_out("argument error");
}

/**
 * Fetches a signed 32-bit integer from the device dictionary
 * 
 * @param num_args   number of delimiter split inputs
 * @param vars       the query variables provided to the device
 * @param dictionary the dictionary currently in use by the device,
 *                   this is initialized in the main function
 *                   in serial.c
 */
void command_query(int num_args, char** vars, dict* dictionary) {
    char* res;

    if (num_args == 2) {
        int *stored;

        stored = malloc(sizeof(*stored));
        if (query(dictionary, vars[1], stored) == 0) {
            res = long_to_string(*stored);
            set_error("success","");
        } else {
            res = "undefined";
            set_error("error: undefined variable", "query");
        }

        free(stored);
        serial_out(res);
        return;
    }

    serial_out("argument error");
}

/**
 * If there is space, pushes a signed 32-bit integer on to the device stack
 * 
 * @param num_args      number of delimiter split inputs
 * @param vars          the query variables provided to the device
 * @param stack_pointer the stack currently in use by the device,
 *                      this is initialized in the main function
 *                      in serial.c
 */
void command_push(int num_args, char** vars, stack *stack_pointer) {
    if (num_args != 2) {
        serial_out("argument error");
        return;
    } else if (is_stack_full(stack_pointer)) {
        set_error("error: stack overflow", "push");
        serial_out("overflow");
        return;
    }

    int *value;
    value = malloc(sizeof(*value));

    if (parse_int(vars[1], value) == 0) {
        push(stack_pointer, *value);
        free(value);
        set_error("success","");
        serial_out("done");
        return;
    }

    serial_out("argument error");
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
 */
void command_add(int num_args, char** vars, stack *stack_pointer) {
    if (num_args < 2) {
        set_error("error: undefined variable", "add");
        serial_out("undefined");
        return;
    }

    if (num_args > 3) {
        serial_out("argument error");
        return;
    }

    int *var1;
    var1 = malloc(sizeof(*var1));

    if (parse_int(vars[1], var1) == 0) {
        int *var2;
        var2 = malloc(sizeof(*var2));

        if (num_args > 2) {
            if (parse_int(vars[2], var2) != 0) {
                serial_out("argument error");
                return;
            }

            char* res = long_to_string(*var1 + *var2);

            set_error("success", "");
            free(var1);
            free(var2);
            serial_out(res);
            return;   
        } else if (is_stack_empty(stack_pointer)) {
            free(var1);
            free(var2);
            set_error("error: undefined variable, stack is empty", "add");
            serial_out("undefined");
            return;
        } else {
            *var2 = peek(stack_pointer);
            char* res = long_to_string(*var1 + *var2);

            free(var1);
            free(var2);
            set_error("success", "");
            serial_out(res);
            return;
        }
    } else {
        serial_out("argument error");
        return;
    }

    set_error("error: undefined variable", "add");
    serial_out("undefined");
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
 */
void command_pop(stack *stack_pointer) {
    if (is_stack_empty(stack_pointer)) {
        set_error("error: undefined variable, stack is empty", "pop");
        serial_out("undefined");
        return;
    } 

    int val  = pop(stack_pointer);
    char* res = long_to_string(val);

    set_error("success", "");
    serial_out(res);
}

/**
 * Prints the status of the existing factoring processes
 */ 
void command_ps() {
    display();
}

/**
 * Prints the result ( or status if not complete ) of
 * a previous factoring process if a valid ID is given
 * 
 * @param num_args      number of delimiter split inputs
 * @param vars          the query variables provided to the device
 */
void command_result(int num_args, char** vars) {
    if (num_args == 2) {
        result(vars[1]);
    } else {
        serial_out("argument error");
    }
}

/////////////////////////
// BACKGROUND COMMANDS //
/////////////////////////

/**
 * Gets the prime factors of:
 * - A given integer value
 * - A given dictionary entry
 * - The number at the top of the stack
 * 
 * Prints the id of the factoring process upon creation.
 * 
 * @param num_args      number of delimiter split inputs
 * @param vars          the query variables provided to the device
 * @param counter       the current task number
 * @param stack_pointer stack currently in use by the device,
 * @param dictionary    the dictionary currently in use by the device,
 */ 
void command_factor(int num_args, char** vars, int counter, stack *stack_pointer, dict* dictionary) {
    char id[16] = "id";
    snprintf(id, 16, "id%d", counter);

    int *var;
    var = malloc(sizeof(*var));

    if (num_args == 2) {
        if ((parse_int(vars[1], var) == 0)|| (query(dictionary, vars[1], var) == 0)) {
            if(prepare_factor(*var, id) == 0) {
                serial_out(id);
            } else {
                serial_out("could not start task");
                set_error("error: could not start task", "factor");
            }
        }
    } else if (num_args < 2 && !is_stack_empty(stack_pointer)) {
        *var = peek(stack_pointer);
        if (prepare_factor(*var, id) == 0) {
            serial_out(id);
        } else {
            serial_out("could not start task");
            set_error("error: could not start task", "factor");
        }
    } else {
        set_error("error: undefined variable", "factor");
        serial_out("undefined");
    }
}