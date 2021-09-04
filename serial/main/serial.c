#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "stack.h"
#include "dict.h"
#include "utils.h"
#include "commands.h"

#define MSG_BUFFER_LENGTH 256

const TickType_t read_delay = 50 / portTICK_PERIOD_MS;

// serial_out(..) method assumes string is null-terminated but does not
//	have the specification-mandated newline terminator.  This method applies
//	the newline terminator and writes the string over the serial connection.
void serial_out(const char* string) {
	int end = strlen(string);
	if (end >= MSG_BUFFER_LENGTH) {
		// Error: Output too long.
		return;
	}

	// NOTE: Working spec requires max of 256 bytes containing a newline terminator.
	//	Thus the storage buffer needs 256+1 bytes since we also need a null terminator
	//	to form a valid string.
	char msg_buffer[MSG_BUFFER_LENGTH+1];
	memset(msg_buffer, 0, MSG_BUFFER_LENGTH+1);
	strcpy(msg_buffer, string);
	msg_buffer[end] = '\n';
	printf(msg_buffer);
	fflush(stdout);
}

/**
 * respond(..) provides routing for the given query string 
 * Sends out the outcome of the command + argument combination
 */
void respond(char* q, stack *pt, dict* d) {
	// Split query on single space ' ' delimiter
	char** split = string_split(q, " ");

	int i = 0;
	// Count query entries (command + arguments)
	for(; split[i] != NULL; ++i) { }

	// If there are no words present then
	// we skip searching for a command entirely
	if (i > 0) {
		// Cast command to uppercase to remove
		// Case sensitivity
		const char* COMMAND = strupr(split[0]);

		// "Switch" through available commands
		if (strcmp(COMMAND, "PING") == 0) {
			serial_out(command_ping());
		} else if (strcmp(COMMAND, "MAC") == 0) {
			serial_out(command_mac());
		} else if (strcmp(COMMAND, "ID") == 0 ) {
			serial_out(command_id());
		} else if (strcmp(COMMAND, "VERSION") == 0) {
			serial_out(command_version());
		} else if (strcmp(COMMAND, "ERROR") == 0) {
			serial_out(get_error());
		} else if (strcmp(COMMAND, "STORE") == 0) {
			serial_out(command_store(i, split, d));
		} else if (strcmp(COMMAND, "QUERY") == 0) {
			serial_out(command_query(i, split, d));
		} else if (strcmp(COMMAND, "PUSH") == 0) {
			serial_out(command_push(i, split, pt));
		} else if (strcmp(COMMAND, "POP") == 0) {
			serial_out(command_pop(pt));
		} else if (strcmp(COMMAND, "ADD") == 0) {
			serial_out(command_add(i, split, pt));
		} else {
			// Default case, command does not exist
			set_error("command error", "");
			serial_out("command error");
		}
	} else {
		// No command input
		set_error("command error", "");
		serial_out("command error");
	} 

	free(split);
}

/**
 * Entry point main function
 * Reads in query string and validates length
 * 
 * If length is satisfied query string is
 * routed to the respond() function.
 */
void app_main(void)
{
	serial_out("firmware ready");

	char query[MSG_BUFFER_LENGTH];

	// Note: DO NOT CHANGE THESE CAPACITY
	// AND SIZE VARIABLES HERE
	//
	// For stack capacity can be changed in
	// stack.h
	//
	// For dict capacity can be changed in
	// dict.h
	stack *pt = create_stack(STACK_SIZE);
	dict* d = create_dict(CAPACITY); 

	while (true) {
		int complete = 0;
		int at = 0;
		memset(query, 0, MSG_BUFFER_LENGTH);

		while (!complete) {
			if (at >= 256) {
				// Error: Input too long.
				break;
			}
			int result = fgetc(stdin);
			if (result == EOF) {
				vTaskDelay(read_delay);
				continue;
			}
			else if ((char)result == '\n') {
				complete = true;
			}
			else {
				query[at++] = (char)result;
			}
		}

		respond(query, pt, d);
		serial_out("");
	}
}
