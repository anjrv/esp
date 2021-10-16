#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

#include "tasks.h"

#define MSG_LENGTH 256
#define MSG_QUEUE_SLOTS 4

#define DELAY_SERIAL ((TickType_t)(50 / portTICK_PERIOD_MS))

#define WAIT_QUEUE ((TickType_t)(10 / portTICK_PERIOD_MS))

/*
* DATA INVARIANT:
*  'raw' shall always be a null-terminated valid C-string so long as prepare_message(..) has
*	been invoked on the structure at least once.
*/
typedef struct Message {
	char raw[MSG_LENGTH + 1];
} Message;



/*
* DATA INVARIANT:
*  NO write occurs on the structure's members without acquiring the 'access' semaphore,
*	with the EXCEPTION of a single call to prepare_queue(..) which initializes the 
*	structure.
*  
*  0 <= next < MSG_QUEUE_SLOTS
*  
*  0 <= size <= MSG_QUEUE_SLOTS
*/
struct MessageQueue {
	SemaphoreHandle_t access;
	Message data[MSG_QUEUE_SLOTS];
	volatile int next;
	volatile int size;
} input, output;




void prepare_queue(struct MessageQueue* A_buffer) {
	assert(A_buffer != NULL);

	A_buffer->access = NULL;
	A_buffer->access = xSemaphoreCreateBinary();
	assert(A_buffer->access != NULL);

	memset(A_buffer->data, 0, sizeof(Message) * MSG_QUEUE_SLOTS);
	A_buffer->next = 0;
	A_buffer->size = 0;

	xSemaphoreGive(A_buffer->access);
}

void prepare_message(Message* A_msg) {
	assert(A_msg != NULL);

	memset(A_msg, 0, sizeof(Message));
}

/*
* push_message(..) method attempts to commit a message to a queue.
* 
* IF the call succeeds, the return value is 0.  In this case, the access semaphore
*  was successfully acquired, space was available for the message, and the message
*  was successfully written into the queue, followed by releasing the semaphore.
* 
* IF the call fails to acquire the semaphore, -1 is returned.
* 
* IF the call acquires the semaphore, but no space exists on the queue, -2 is returned.
* 
* Should this call not succeed, NO MODIFICATIONS are made to the queue.
*/
int push_message(struct MessageQueue* A_buffer, Message* A_in) {
	assert(A_buffer != NULL);
	assert(A_in != NULL);

	if (xSemaphoreTake(A_buffer->access, WAIT_QUEUE) != pdTRUE) {
		return -1;
	}

	if (A_buffer->size == MSG_QUEUE_SLOTS) {
		xSemaphoreGive(A_buffer->access);
		return -2;
	}

	int target = (A_buffer->next + A_buffer->size) % MSG_QUEUE_SLOTS;
	memcpy(A_buffer->data + target, A_in, sizeof(Message));
	A_buffer->size += 1;
	xSemaphoreGive(A_buffer->access);
	return 0;
}

/*
* pop_message(..) method attempts to retrieve a message from a queue.
* 
* IF the call succeeds, the return value is 0.  In this case, the access
*  semaphore was successfully acquired, and one or more messages were present
*  in the queue.  The eldest message was successfully removed from the queue,
*  and the semaphore released.
* 
* IF the call fails to acquire the semaphore, -1 is returned.
* 
* IF the call acquires the semaphore, but no messages exist on the queue, -2 is returned.
* 
* Should this call not succeed, NO MODIFICATIONS are made to the queue.
*/
int pop_message(struct MessageQueue* A_buffer, Message* A_out) {
	assert(A_buffer != NULL);
	assert(A_out != NULL);

	if (xSemaphoreTake(A_buffer->access, WAIT_QUEUE) != pdTRUE) {
		return -1;
	}

	if (A_buffer->size == 0) {
		xSemaphoreGive(A_buffer->access);
		return -2;
	}

	memcpy(A_out, A_buffer->data + A_buffer->next, sizeof(Message));
	A_buffer->next = (A_buffer->next + 1) % MSG_QUEUE_SLOTS;
	A_buffer->size -= 1;
	xSemaphoreGive(A_buffer->access);
	return 0;
}


void serial_prepare() {
	prepare_queue(&input);
	prepare_queue(&output);
}

void serial_service(void* A_param) {
	/*
	* GENERAL OUTLINE:
	*  Serial service produces Messages from serial input, and sends output Messages back as
	*	serial output.  While any input is forthcoming, this service continues appending it
	*	char by char to a new message.  When no more input is forthcoming, this method checks whether
	*	any output messages are prepared -- if so it posts them over the serial connection.
	*  While no input is available, and no output is ready, the service sleeps.
	* 
	*  If an input message is read, and no space exists in the buffer to commit it, then the
	*	service does not process any more input until space for a valid commit is available.  During
	*	this wait the service may still post output messages.
	* 
	*  NOTE: This design has a flaw -- if the ability of a task to produce output is dependent on
	*	FUTURE input (i.e. all command processing is not entirely independent) then a state can
	*	occur wherein the input queue is full, but command processing cannot complete.  We avoid
	*	this circumstance by careful design of commands -- all commands require only their single
	*	input message to complete entirely.
	* 
	*  The serial service should be a high priority task.
	*/
	Message work_in;
	Message work_out;
	int in_at = 0;
	int in_pending = 0;

	prepare_message(&work_in);
	prepare_message(&work_out);

	while (true) {
		// Input:
		// If a complete message is pending, i.e. was received but could not be added
		//	to the input queue, then we ignore further input for now and try and push
		//	it again.

		// NOTE: This service DOES NOT include the terminating '\n' character in input
		//	messages.
		if (!in_pending) {
			int next = fgetc(stdin);
			while (next != EOF) {
				assert(in_at < MSG_LENGTH);

				char c = (char)next;

				if (c == '\n') {
					if (!push_message(&input, &work_in)) {
						prepare_message(&work_in);
						in_at = 0;
					}
					else {
						in_pending = 1;
						break;
					}
				}
				else {
					work_in.raw[in_at++] = c;
				}

				next = fgetc(stdin);
			}
		}
		else {
			if (!push_message(&input, &work_in)) {
				prepare_message(&work_in);
				in_at = 0;
				in_pending = 0;
			}
		}

		// Output:
		while (!pop_message(&output, &work_out)) {
			printf(work_out.raw);
			fflush(stdout);
			prepare_message(&work_out);
		}

		vTaskDelay(DELAY_SERIAL);
	}
}

void command_service(void* A_param) {
	// The command service is quite rudimentary -> we attempt to retrieve any
	//	available input messages, and echo them back out with a small modification.

	// Our processing prefixes the input number to the output, and echos back the input.
	// We automatically truncate the input string to a max of 220 characters, leaving
	//	sufficient space for our prefix, and newline terminator.
	int counter = 0;
	int trunc = 220;

	Message work;
	Message out;
	Message term;

	prepare_message(&work);
	prepare_message(&out);
	prepare_message(&term);

	term.raw[0] = '\n';

	while (true) {
		while (!pop_message(&input, &work)) {
			work.raw[trunc] = '\0';
			if (snprintf(out.raw, MSG_LENGTH, "%d %s\n", counter++, work.raw) < 0) {
				abort();
			}

			while (push_message(&output, &out) != 0) {
				vTaskDelay(DELAY_SERIAL);
			}

			while (push_message(&output, &term) != 0) {
				vTaskDelay(DELAY_SERIAL);
			}
		}
		vTaskDelay(DELAY_SERIAL);
	}
}
