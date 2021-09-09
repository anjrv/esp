#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
void respond(char* q, stack *stack_pointer, dict* dictionary) {
	// Split query on single space ' ' delimiter
	char** split = string_split(q, " ");

	int quant = 0;
	for(; split[quant] != NULL; ++quant) { }

	// If there are no words present then
	// we skip searching for a command entirely
	if (quant > 0) {
		// Cast command to uppercase to remove
		// Case sensitivity
		char* command = strupr(split[0]);

		// "Switch" through available commands
		if (strcmp(command, "PING") == 0) {
			serial_out(command_ping());
		} else if (strcmp(command, "MAC") == 0) {
			serial_out(command_mac());
		} else if (strcmp(command, "ID") == 0 ) {
			serial_out(command_id());
		} else if (strcmp(command, "VERSION") == 0) {
			serial_out(command_version());
		} else if (strcmp(command, "ERROR") == 0) {
			serial_out(get_error());
		} else if (strcmp(command, "STORE") == 0) {
			serial_out(command_store(quant, split, dictionary));
		} else if (strcmp(command, "QUERY") == 0) {
			serial_out(command_query(quant, split, dictionary));
		} else if (strcmp(command, "PUSH") == 0) {
			serial_out(command_push(quant, split, stack_pointer));
		} else if (strcmp(command, "POP") == 0) {
			serial_out(command_pop(quant, stack_pointer));
		} else if (strcmp(command, "ADD") == 0) {
			serial_out(command_add(quant, split, stack_pointer));
		} else {
			// Default case, command does not exist
			set_error("error: invalid command", "");
			serial_out("command error");
		}
	} else {
		// No command input
		set_error("error: invalid command", "");
		serial_out("command error");
	}
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
	dict* dictionary = create_dict(CAPACITY); 
	stack *stack_pointer = create_stack(STACK_SIZE);

	while (true) {
		int complete = 0;
		int at = 0;
		int consecutive_whitespace = 0;

		memset(query, 0, MSG_BUFFER_LENGTH);

		while (!complete) {
			if (at >= 256) {
				serial_out("error: input length exceeded");
				break;
			}
			int result = fgetc(stdin);

			if (at == 0 && ((char)result == ' ')) {
				serial_out("error: leading whitespace");
				break;
			}

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

			if ((char)result == ' ') {
				if (complete && consecutive_whitespace) {
					serial_out("error: trailing whitespace");
					complete = false;
					break;
				} else if (consecutive_whitespace) {
					serial_out("error: consecutive whitespace");
					complete = false;
					break;
				} 

				consecutive_whitespace = 1;
			} else {
				consecutive_whitespace = 0;
			}
		}

		if (complete) { 
			respond(query, stack_pointer, dictionary);
		}

		serial_out("");
	}
}
