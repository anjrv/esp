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
#include "tasks.h"
#include "client.h"
#include "data_tasks.h"
#include "wifi.h"

#define MSG_BUFFER_LENGTH 256

const TickType_t CONN_DELAY = 500 / portTICK_PERIOD_MS;

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
static char err_msg[MSG_BUFFER_LENGTH] = "no history";
static char mac_addr[18] = {0};
static char version[6] = {0};

/**
 * Used for setting the current error state
 * 
 * @param status    the status of the previous command 
 * @param command   the name of the previous command
 */
void set_error(char *status, char *command)
{
    memset(err_msg, 0, MSG_BUFFER_LENGTH);

    if (strcmp(command, "") == 0)
    {
        strcpy(err_msg, status);
    }
    else
    {
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
void get_error()
{
    serial_out(err_msg);
}

/**
 * Prints "pong"
 */
void command_ping()
{
    set_error("success", "");
    serial_out("pong");
}

/**
 * Prints the mac address of the device 
 * Leading zeroes are left in
 */
void command_mac()
{
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
        mac[5]);

    set_error("success", "");
    serial_out(mac_addr);
}

/**
 * Prints the id of the developer
 */
void command_id()
{
    set_error("success", "");
    serial_out(DEVELOPER_ID);
}

/**
 * Prints the firmware version of the device
 * defined in version.h
 */
void command_version()
{
    sprintf(
        version,
        "%d.%d.%d",
        MAJOR,
        MINOR,
        REVISION);

    set_error("success", "");
    serial_out(version);
}

int validate(char *key, char *pattern)
{
    regex_t re;

    if (regcomp(&re, pattern, REG_EXTENDED) != 0)
    {
        return -1;
    }

    int status = regexec(&re, key, (size_t)0, NULL, 0);
    regfree(&re);

    if (status != 0)
    {
        return -2;
    }

    return 0;
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
void command_store(int num_args, char **vars, dict *dictionary)
{
    char *res;

    if (num_args == 3 && strlen(vars[1]) <= 16)
    {
        if (validate(vars[1], "[a-zA-Z_]+") == 0)
        {
            int *var;
            var = malloc(sizeof(*var));

            if (parse_int(vars[2], var) == 0)
            {
                int *stored;
                stored = malloc(sizeof(*stored));

                if (query(dictionary, vars[1], stored) == 0)
                {
                    res = long_to_string(*stored);
                }
                else
                {
                    res = "undefined";
                }

                if (!is_dict_full(dictionary))
                {
                    store(dictionary, vars[1], *var);
                    set_error("success", "");
                }
                else
                {
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
void command_query(int num_args, char **vars, dict *dictionary)
{
    char *res;

    if (num_args == 2)
    {
        int *stored;

        stored = malloc(sizeof(*stored));
        if (query(dictionary, vars[1], stored) == 0)
        {
            res = long_to_string(*stored);
            set_error("success", "");
        }
        else
        {
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
void command_push(int num_args, char **vars, stack *stack_pointer)
{
    if (num_args != 2)
    {
        serial_out("argument error");
        return;
    }
    else if (is_stack_full(stack_pointer))
    {
        set_error("error: stack overflow", "push");
        serial_out("overflow");
        return;
    }

    int *value;
    value = malloc(sizeof(*value));

    if (parse_int(vars[1], value) == 0)
    {
        push(stack_pointer, *value);
        free(value);
        set_error("success", "");
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
 * @param dictionary    the dictionary currently in use by the device,
 */
void command_add(int num_args, char **vars, stack *stack_pointer, dict *dictionary)
{
    if (num_args < 2)
    {
        set_error("error: undefined variable", "add");
        serial_out("undefined");
        return;
    }

    if (num_args > 3)
    {
        serial_out("argument error");
        return;
    }

    int *var1;
    var1 = malloc(sizeof(*var1));

    if ((query(dictionary, vars[1], var1) == 0))
    {
        int *var2;
        var2 = malloc(sizeof(*var2));

        if (num_args > 2)
        {
            if ((query(dictionary, vars[2], var2) != 0))
            {
                serial_out("argument error");
                return;
            }

            char *res = long_to_string(*var1 + *var2);

            set_error("success", "");
            free(var1);
            free(var2);
            serial_out(res);
            return;
        }
        else if (is_stack_empty(stack_pointer))
        {
            free(var1);
            free(var2);
            set_error("error: undefined variable, stack is empty", "add");
            serial_out("undefined");
            return;
        }
        else
        {
            *var2 = peek(stack_pointer);
            char *res = long_to_string(*var1 + *var2);

            free(var1);
            free(var2);
            set_error("success", "");
            serial_out(res);
            return;
        }
    }
    else
    {
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
void command_pop(stack *stack_pointer)
{
    if (is_stack_empty(stack_pointer))
    {
        set_error("error: undefined variable, stack is empty", "pop");
        serial_out("undefined");
        return;
    }

    int val = pop(stack_pointer);
    char *res = long_to_string(val);

    set_error("success", "");
    serial_out(res);
}

/**
 * Prints the status of the existing factoring processes
 */
void command_ps()
{
    display_factors();
}

/**
 * Prints the result of a previous background task if a valid ID is given 
 * 
 * @param num_args      number of delimiter split inputs
 * @param vars          the query variables provided to the device
 */
void command_result(int num_args, char **vars)
{
    if (num_args == 2)
    {
        if (validate(vars[1], "id[0-9]+") == 0)
        {
            get_result(vars[1]);
        }
        else
        {
            serial_out("invalid id");
        }
    }
    else
    {
        serial_out("argument error");
    }
}

/**
 * Attempts to scan and connect to a bluetooth name + service
 * 
 * @param num_args      number of delimiter split inputs
 * @param vars          the query variables provided to the device
 */
void command_bt_connect(int num_args, char **vars)
{
    if (num_args != 3)
    {
        serial_out("argument error");
        return;
    }

    if (active_connection)
    {
        set_error("error: invalid connect", "bt_connect");
        serial_out("invalid connect");
        return;
    }

    strcpy(BT_DATA_SOURCE_DEVICE, vars[1]);
    strcpy(BT_DATA_SOURCE_SERVICE, vars[2]);

    int result = bt_scan_now();

    if (result != 0)
    {
        serial_out("connection failure");
        return;
    }

    int wait = 0;
    while (!active_connection && wait < 10)
    {
        vTaskDelay(CONN_DELAY);
        wait++;
    }

    if (active_connection)
    {
        serial_out("connection success");
    }
    else
    {
        serial_out("connection failure");
    }
}

/**
 * Prints some information about the current bluetooth connection 
 */
void command_bt_status()
{
    if (active_connection)
    {
        serial_out("Connection: OPEN");

        char device_buf[50];
        snprintf(device_buf, sizeof(device_buf), "%s%s", "Device: ", BT_DATA_SOURCE_DEVICE);
        serial_out(device_buf);

        char service_buf[50];
        snprintf(service_buf, sizeof(service_buf), "%s%s", "Service: ", BT_DATA_SOURCE_SERVICE);
        serial_out(service_buf);

        // TODO: Count pending tasks from data structure
    }
    else
    {
        serial_out("Connection: CLOSED");
    }
}

/**
 * Closes the current bluetooth connection
 */
void command_bt_close()
{
    if (active_connection)
    {
        active_connection = 0;
        bt_disconnect();
        serial_out("connection closed");
        return;
    }

    serial_out("invalid disconnect");
}

/**
 * Attempts to add a dataset with the given name and source
 * 
 * @param num_args      number of delimiter split inputs
 * @param vars          the query variables provided to the device
 */
void command_data_create(int num_args, char **vars)
{
    if (num_args != 3)
    {
        serial_out("argument error");
        return;
    }

    char tmp[33];
    memset(tmp, '\0', sizeof(tmp));
    strcpy(tmp, vars[2]);
    if (!(strcmp(strupr(tmp), "NOISE") == 0 || strcmp(strupr(tmp), "BT_DEMO") == 0))
    {
        serial_out("invalid source");
        return;
    }

    memset(tmp, '\0', sizeof(tmp));
    strcpy(tmp, vars[1]);
    if (strcmp(strupr(tmp), "NOISE") == 0 || strcmp(strupr(tmp), "BT_DEMO") == 0)
    {
        serial_out("invalid name");
        return;
    }

    if (create_dataset(vars[1], vars[2]) != 0)
    {
        serial_out("invalid name");
        return;
    }

    serial_out("data set created");
}

/**
 * Attempts to destroy a dataset ( and its entries ) with the given name
 * 
 * @param num_args      number of delimiter split inputs
 * @param vars          the query variables provided to the device
 */
void command_data_destroy(int num_args, char **vars)
{
    if (num_args != 2)
    {
        serial_out("argument error");
        return;
    }

    if (destroy_dataset(vars[1]) != 0)
    {
        serial_out("invalid name");
        return;
    }

    serial_out("data set destroyed");
}

/**
 * Attempts to print some information about a given dataset
 * 
 * @param num_args      number of delimiter split inputs
 * @param vars          the query variables provided to the device
 */
void command_data_info(int num_args, char **vars)
{
    if (num_args != 2)
    {
        serial_out("argument error");
        return;
    }

    char tmp[33];
    memset(tmp, '\0', sizeof(tmp));
    strcpy(tmp, vars[1]);

    if (strcmp(strupr(tmp), "NOISE") == 0)
    {
        serial_out("Status: Available");
        serial_out("Keys: a, b, c");
        return;
    }
    else if (strcmp(strupr(tmp), "BT_DEMO") == 0)
    {
        if (bt_demo_available())
        {
            serial_out("Status: Available");
        }
        else
        {
            serial_out("Status: Unavailable");
        }
        serial_out("Keys: main");
        return;
    }
    else
    {
        if (check_dataset(vars[1]) != 0)
            serial_out("invalid name");
    }
}

/**
 * Prints the raw data rows of the requested dataset[.key] to serial output, or a problem report 
 * 
 * @param num_args      number of delimiter split inputs
 * @param vars          the query variables provided to the device
 */
void command_data_raw(int num_args, char **vars)
{
    if (num_args != 2)
    {
        serial_out("argument error");
        return;
    }

    char *key;
    key = strstr(vars[1], ".");

    if (key)
    {
        char **split = NULL;
        char *duplicate = strdup(vars[1]);
        char *p = strtok(duplicate, ".");
        int quant = 0;

        while (p)
        {
            split = realloc(split, sizeof(char *) * ++quant);
            split[quant - 1] = p;

            p = strtok(NULL, ".");
        }

        split = realloc(split, sizeof(char *) * (quant + 1));
        split[quant] = '\0';

        if (strcmp(split[1], "a") == 0)
        {
            print_raw_data(split[0], 0);
        }
        else if (strcmp(split[1], "b") == 0)
        {
            print_raw_data(split[0], 1);
        }
        else if (strcmp(split[1], "c") == 0)
        {
            print_raw_data(split[0], 2);
        }
        else if (strcmp(split[1], "main") == 0)
        {
            print_raw_data(split[0], 3);
        }
        else
        {
            serial_out("invalid key");
        }

        free(split);
        free(duplicate);
    }
    else
    {
        print_raw_data(vars[1], -1);
    }
}

void command_net_locate()
{
    int peers = wifi_send_locate();

    char buf[50];
    snprintf(buf, sizeof(buf), "%s %d %s", "Linked", peers, "nodes");
    serial_out(buf);
}

void command_net_table()
{
    wifi_net_table();
}

void command_net_reset()
{
    wifi_net_reset();
}

void command_net_status() {
    wifi_send_status();
}

/////////////////////////
// BACKGROUND COMMANDS //
/////////////////////////

/**
 * Gets the prime factors of:
 * - A given integer value
 * - A given dictionary entry
 * - The number at the top of the stack. 
 * 
 * Prints the id of the factoring process upon creation or a state report.
 * 
 * @param num_args      number of delimiter split inputs
 * @param vars          the query variables provided to the device
 * @param counter       the current task number
 * @param stack_pointer stack currently in use by the device,
 * @param dictionary    the dictionary currently in use by the device,
 */
void command_factor(int num_args, char **vars, int counter, stack *stack_pointer, dict *dictionary)
{
    char id[16] = "id";
    snprintf(id, 16, "id%d", counter);

    int *var;
    var = malloc(sizeof(*var));

    if (num_args == 2)
    {
        if ((parse_int(vars[1], var) == 0) || (query(dictionary, vars[1], var) == 0))
        {
            if (prepare_factor(*var, id) == 0)
            {
                serial_out(id);
            }
            else
            {
                serial_out("could not start task");
                set_error("error: could not start task", "factor");
            }
        }
    }
    else if (num_args < 2 && !is_stack_empty(stack_pointer))
    {
        *var = peek(stack_pointer);
        if (prepare_factor(*var, id) == 0)
        {
            serial_out(id);
        }
        else
        {
            serial_out("could not start task");
            set_error("error: could not start task", "factor");
        }
    }
    else
    {
        set_error("error: undefined variable", "factor");
        serial_out("undefined");
    }
}

/**
 * Attempts to create a worker to append data to a dataset. 
 * 
 * Prints the id of the worker process upon creation or a state report. 
 * 
 * @param num_args      number of delimiter split inputs
 * @param vars          the query variables provided to the device
 * @param counter       the current task number
 */
void command_data_append(int num_args, char **vars, int counter)
{
    int *var;
    var = malloc(sizeof(*var));

    if (num_args != 3 || parse_int(vars[2], var) != 0)
    {
        serial_out("argument error");
        free(var);
        return;
    }

    if (*var < 1)
    {
        serial_out("invalid request");
        free(var);
        return;
    }

    char id[16] = "id";
    snprintf(id, 16, "id%d", counter);

    int res = prepare_append(*var, id, vars[1]);

    if (res == -3)
    {
        serial_out("source unavailable");
    }

    if (res == -2)
    {
        serial_out("invalid name");
    }

    if (res == 0)
    {
        serial_out(id);
    }

    free(var);
}

/**
 * Attempts to create a worker to gather information about a dataset. 
 * 
 * Prints the id of the worker process upon creation or a state report. 
 * 
 * @param num_args      number of delimiter split inputs
 * @param vars          the query variables provided to the device
 * @param counter       the current task number
 */
void command_data_stat(int num_args, char **vars, int counter)
{
    if (num_args != 2)
    {
        serial_out("argument error");
        return;
    }

    char *key;
    key = strstr(vars[1], ".");

    if (key)
    {
        char **split = NULL;
        char *duplicate = strdup(vars[1]);
        char *p = strtok(duplicate, ".");
        int quant = 0;

        char id[16] = "id";
        snprintf(id, 16, "id%d", counter);

        while (p)
        {
            split = realloc(split, sizeof(char *) * ++quant);
            split[quant - 1] = p;

            p = strtok(NULL, ".");
        }

        split = realloc(split, sizeof(char *) * (quant + 1));
        split[quant] = '\0';

        char *source = dataset_get_source(split[0]);

        if (source == NULL)
        {
            serial_out("invalid name");
            return;
        }

        int res = 0;

        if (strcmp(strupr(source), "BT_DEMO") == 0)
        {
            if (strcmp(split[1], "main") == 0)
            {
                res = prepare_stat(0, id, split[0]);
            }
            else
            {
                res = -4;
            }
        }
        else if (strcmp(strupr(source), "NOISE") == 0)
        {
            if (strcmp(split[1], "a") == 0)
            {
                res = prepare_stat(0, id, split[0]);
            }
            else if (strcmp(split[1], "b") == 0)
            {
                res = prepare_stat(1, id, split[0]);
            }
            else if (strcmp(split[1], "c") == 0)
            {
                res = prepare_stat(2, id, split[0]);
            }
            else
            {
                res = -4;
            }
        }
        else
        {
            res = -4;
        }

        switch (res)
        {
        case 0:
            serial_out(id);
            break;
        case -2:
            serial_out("invalid name");
            break;
        case -3:
            serial_out("empty set");
            break;
        case -4:
            serial_out("invalid key");
            break;
        }

        free(source);
        free(split);
        free(duplicate);
    }
    else
    {
        serial_out("invalid key");
    }
}