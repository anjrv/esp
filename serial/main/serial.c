#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "serial.h"
#include "stack.h"
#include "dict.h"
#include "utils.h"
#include "commands.h"
#include "factors.h"

const TickType_t read_delay = 50 / portTICK_PERIOD_MS;
// Data structures and global variables to ease communication
// between the main loop and response function
dict* dictionary;
stack* stack_pointer;
int counter;
char q[MSG_BUFFER_LENGTH];


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
 * after splitting it into space delimited tokens
 * 
 * @param pvParameter function call parameter not used
 */
void respond(void *pvParameter) {
	char** split = NULL;
	char* duplicate = strdup(q);
	char* p = strtok(duplicate, " ");
    int quant = 0;

    while(p) {
        split = realloc(split, sizeof(char*) * ++quant);
        split[quant-1] = p;

        p = strtok(NULL, " ");
    }

    split = realloc(split, sizeof(char*) * (quant+1));
    split[quant] = '\0';

	// If there are no words present then
	// we skip searching for a command
	if (quant > 0) {
		// Cast command to uppercase to remove
		// Case sensitivity
		char* command = strupr(split[0]);

		// "Switch" through available commands
		if (strcmp(command, "PING") == 0) {
			command_ping();
		} else if (strcmp(command, "MAC") == 0) {
			command_mac();
		} else if (strcmp(command, "ID") == 0 ) {
			command_id();
		} else if (strcmp(command, "VERSION") == 0) {
			command_version();
		} else if (strcmp(command, "ERROR") == 0) {
			get_error();
		} else if (strcmp(command, "STORE") == 0) {
			command_store(quant, split, dictionary);
		} else if (strcmp(command, "QUERY") == 0) {
			command_query(quant, split, dictionary);
		} else if (strcmp(command, "PUSH") == 0) {
			command_push(quant, split, stack_pointer);
		} else if (strcmp(command, "POP") == 0) {
			command_pop(stack_pointer);
		} else if (strcmp(command, "ADD") == 0) {
			command_add(quant, split, stack_pointer);
		} else if (strcmp(command, "PS") == 0) {
			command_ps();
		} else if (strcmp(command, "RESULT") == 0) {
			command_result(quant, split);
		} else if (strcmp(command, "FACTOR") == 0) {
			command_factor(quant, split, counter++, stack_pointer, dictionary);
		} else {
			// Default case, command does not exist
			serial_out("command error");
		}
	} else {
		// No command input
		serial_out("command error");
	}

	// I assume the rule for realloc is the same as it is for malloc
	// We realloced the split variable to fit our tokens, now we free it
	free(split);
	vTaskDelete(NULL); // Respond deletes itself when done
}

/**
 * Main loop function of the app
 * 
 * Reads user input and creates
 * response function as needed
 * 
 * @param pvParameter function call parameter not used
 */
void main_task(void *pvParameter) {
	serial_out("firmware ready");

	while (true) {
		int complete = 0;
		int at = 0;
		int whitespace = 0;
		int consecutive_whitespace = 0;
		int leading_whitespace = 0;
		int trailing_whitespace = 0;

		memset(q, 0, MSG_BUFFER_LENGTH);

		while (!complete) {
			if (at >= 256) {
				serial_out("input length exceeded");
				break;
			}
			int result = fgetc(stdin);

			if (at == 0 && ((char)result == ' ')) {
				leading_whitespace = 1;
			}

			if (result == EOF) {
				vTaskDelay(read_delay);
				continue;
			} else if ((char)result == '\n') {
				if (whitespace) {
					trailing_whitespace = 1;
				}

				complete = true;
			} else {
				q[at++] = (char)result;
			}

			if ((char)result == ' ') {
				if (whitespace) {
					consecutive_whitespace = 1;
				}
				whitespace = 1;
			} else {
				whitespace = 0;
			}

		}

		if (complete) {
			if (leading_whitespace) {
				serial_out("command error");
			} else if (consecutive_whitespace || trailing_whitespace) {
				serial_out("argument error");
			} else {
				xTaskCreatePinnedToCore(
					&respond,
					"respond",
					8192,
					NULL,
					HIGH_PRIORITY,
					NULL,
					tskNO_AFFINITY // Use both cores based on availability
				);
			}
		}

		serial_out("");
	}
}

/**
 * Entry point function, creates the main loop
 */
void app_main(void) {
	dictionary = create_dict(DICT_CAPACITY);
	stack_pointer = create_stack(STACK_CAPACITY);
	counter = 0;

    xTaskCreate(
        &main_task,    // - function ptr
        "main_task",   // - arbitrary name
        2048,          // - stack size [byte]
        NULL,          // - optional data for task
        MAIN_PRIORITY, // - priority of the main task
        NULL		   // - handle to task (for control)
	);
}
