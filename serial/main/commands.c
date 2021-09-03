#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

char error[MSG_BUFFER_LENGTH];

void set_error(char* err, char* command) {
    memset(error, 0, MSG_BUFFER_LENGTH);

    error[0] = '!';
    error[1] = '\n';
}

char* get_error(int num_args) {
   return error; 
}

char* command_ping(int num_args) {
    if (num_args == 1) {
        return "pong";
    } else {
        set_error("argument error", "pong");
        return "argument error";
    }
}

char* command_mac(int num_args) {
    return "mac TODO";
}

char* command_id(int num_args) {
    return "id TODO";
}

char* command_version(int num_args) {
    return "version TODO";
}

char* command_store(int num_args, char** vars) {
    return "store TODO";
}

char* command_query(int num_args, char** vars) {
    return "query TODO";
}

char* command_push(int num_args, char** vars, struct stack *pt) {
    if (num_args != 2) {
        set_error("argument error", "push");
        return "argument error";
    } else if (isFull(pt)) {
        set_error("overflow", "push");
        return "overflow";
    }

    // Failed atoi parsing will return 0
    // Need to check previously if number
    // was actually 0
    if (strcmp(vars[1], "0") == 0) {
        push(pt, 0);
        return "done";
    }

    // If we get 0 here then value is NaN
    int res = atoi(vars[1]);
    if (res != 0) {
        push(pt, res);
        return "done";
    }

    // Catchall return for NaN
    set_error("argument error", "push");
    return "argument error";
}

char* command_pop(int num_args, struct stack *pt) {
    if (num_args != 1) {
        set_error("argument error", "pop");
        return "argument error";
    } else if (isEmpty(pt)) {
        set_error("undefined", "pop");
        return "undefined";
    } 

    int val  = pop(pt);
    char* res = int_to_string(val);

    return res;
}

char* command_add(int num_args, char** vars, struct stack *pt) {
    return "add TODO";
}
