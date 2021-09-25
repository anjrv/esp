#include <math.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "factors.h"
#include "serial.h"
#include "utils.h"

// Constants for easier storage of a function state
#define ACTIVE 'A'
#define PENDING 'P'
#define COMPLETE0 'C'
#define COMPLETE1 'X'
#define RESPONSE_LENGTH 128

typedef struct node node;

struct node {
    char* id;
    char state;
    int value;
    char* factors;

    struct node *next;
};

struct node *head = NULL;
SemaphoreHandle_t head_access;

/**
 * Initializes the linked list semaphore
 * to be used by factor functions
 */
void initialize_factors() {
    head_access = xSemaphoreCreateBinary();
    assert(head_access != NULL);
    xSemaphoreGive(head_access);
}

/**
 * Helper function to check whether list is initialized or not
 */
int is_empty() {
    return head == NULL;
}

/**
 * Function to insert a node with an id and value at the start
 * of the factoring linked list
 * 
 * @param id the id to use for the node
 * @param value the value to be used for the node
 * 
 * @return the process exit state of the insertion attempt
 *         * -1 indicates that obtaining a semaphore failed
 *         * 0 indicates that insertion was a success
 */
int insert(char* id, int value) {
    if (xSemaphoreTake(head_access, WAIT_QUEUE) != pdTRUE) {
		return -1;
	}

    node *link = malloc(sizeof(node));

    link->id = malloc(strlen(id)+ 1);
    link->state = PENDING;
    link->value = value;
    link->factors = NULL;

    strcpy(link->id, id);

    link->next = head;
    head = link;

    xSemaphoreGive(head_access);
    return 0;
}

/**
 * Function to obtain the factoring result of a specific id
 * 
 * @param id the id to find the result of
 * 
 * @return the process exit state of the insertion attempt
 *         * -1 indicates that obtaining a semaphore failed
 *         * 1 indicates an invalid id
 *         * 0 indicates a success
 */
int result(char* id) {
    if (xSemaphoreTake(head_access, WAIT_QUEUE) != pdTRUE) {
		return -1;
	}

    if (is_empty()) {
        serial_out("undefined");
        return 1;
    }

    node *curr = head;

    while(strcmp(curr->id, id) != 0) {
        if (curr->next == NULL) {
            serial_out("undefined");
            return 1;
        } else {
            curr = curr->next;
        }
    }

    if (curr->state == COMPLETE0 || curr->state == COMPLETE1) {
        curr->state = COMPLETE1;
        serial_out(curr->factors);
    } else {
        serial_out("incomplete");
    }


    xSemaphoreGive(head_access);
    return 0;
}

/**
 * Function to obtain the value at the node indicated by the id
 * 
 * @param id the id of the node to find
 * 
 * @return the value to be factored or an error state ( numbers that cannot be factored are used )
 *         * -1 indicates that obtaining a semaphore failed
 *         * 1 indicates an invalid id
 *         * anything else is a valid number to be factored
 */
int get(char* id) {
    if (xSemaphoreTake(head_access, WAIT_QUEUE) != pdTRUE) {
		return -1;
	}
   
    if (is_empty()) {
        return 1;
    }

    node *curr = head;

    while(strcmp(curr->id, id) != 0) {
        if (curr->next == NULL) {
            return 1;
        } else {
            curr = curr->next;
        }
    }

    curr->state = ACTIVE;

    xSemaphoreGive(head_access);
    return curr->value;
}

/**
 * Function to change a node of the factoring linked list
 * 
 * @param id the id to use for the node
 * @param state the state to update to
 * @param factors the factoring result ( if complete )
 * 
 * @return the process exit state of the attempt
 *         * -1 indicates that obtaining a semaphore failed
 *         * 0 indicates that changing was a success
 */

int change(char* id, char state, char* factors) {
    if (xSemaphoreTake(head_access, WAIT_QUEUE) != pdTRUE) {
		return -1;
	}

    if (is_empty()) {
        return 1;
    }

    node *curr = head;

    while(strcmp(curr->id, id) != 0) {
        if (curr->next == NULL) {
            return 1;
        } else {
            curr = curr->next;
        }
    }

    if (factors != NULL) {
        curr->factors = malloc(strlen(factors)+ 1);
        strcpy(curr->factors, factors);
    }

    curr->state = state;
    xSemaphoreGive(head_access);
    return 0;
}

/**
 * Function to print the factoring processes
 * 
 * @return the process exit state of the printing attempt 
 *         * -1 indicates that obtaining a semaphore failed
 *         * 0 indicates a successful exit
 */
int display() {
    if (xSemaphoreTake(head_access, WAIT_QUEUE) != pdTRUE) {
		return -1;
	}

    struct node *ptr = head;
    char res[RESPONSE_LENGTH];

    if (is_empty()) {
        serial_out("empty set");
    } else {
        while (ptr != NULL) {
            memset(res, 0, RESPONSE_LENGTH);

            if (ptr->state == ACTIVE) {
                snprintf(res, RESPONSE_LENGTH, "%s %s %s", "factor", ptr->id, "active");
            } else if (ptr->state == PENDING) {
                snprintf(res, RESPONSE_LENGTH, "%s %s %s", "factor", ptr->id, "pending");
            } else if (ptr->state == COMPLETE0) {
                snprintf(res, RESPONSE_LENGTH, "%s %s %s", "factor", ptr->id, "complete");
            } else {
                snprintf(res, RESPONSE_LENGTH, "%s %s %s", "factor", ptr->id, "complete*");
            }

            serial_out(res);
            ptr = ptr->next;
        }
    }

    xSemaphoreGive(head_access);
    return 0;
}

/**
 * Main factoring function 
 * 
 * @param pvParameter should be the id of the node that the function needs to work on
 */
void factor(void *pvParameter) {
    char* id;
    id = (char*)pvParameter;
    int num = get(id);

    char res[RESPONSE_LENGTH];
    snprintf(res, 10, "%d", num);
    strcat(res, ":");

    // 1, 0 or invalid input, end early
    if (num == 1 || num == 0) {
        strcat(res, " no prime factors");
        change(id, COMPLETE0, res);
        vTaskDelete(NULL);
    }

    if (num < 0) {
        // Is negative, use absolute
        strcat(res, " -1");
        num = abs(num);
    }

    // Repeat until odd number
    while (num%2 == 0) {
        // Divisible by 2
        strcat(res, " 2");
        num = num/2;
    }

    for (int i = 3; i <= sqrt(num); i += 2) {
        while (num % i == 0) {
            // Divisible by i
            strcat(res, " ");
            strcat(res, long_to_string(i));
            num = num/i;
        }
    }

    if (num > 2) {
        // Remaining prime
        strcat(res, " ");
        strcat(res, long_to_string(num));
    }

    while (change(id, COMPLETE0, res) == -1) {
        vTaskDelay(DELAY);
    }

    vTaskDelete(NULL);
}

/**
 * Entry point function to create a factoring task
 * with the given number value and id definition
 * 
 * @param value
 * @param id
 * 
 * @return the process exit state of the task creation
 */
int prepare_factor(int value, char* id) {
    BaseType_t success;
    static char* tag = NULL;
    tag = malloc(strlen(id) + 1);
    strcpy(tag, id);

    while (insert(tag, value) == -1) {
        vTaskDelay(DELAY);
    }

    success = xTaskCreatePinnedToCore(
		&factor,
		id,
		4096,
		(void*)tag,
		LOW_PRIORITY,
		NULL,
		tskNO_AFFINITY
	);

    if (success == pdPASS) {
        return 0;
    }

    return 1;
}