#include <math.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "factors.h"
#include "serial.h"
#include "utils.h"

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

int is_empty() {
    return head == NULL;
}

void insert(char* id, int value) {
    node *link = malloc(sizeof(node));

    link->id = malloc(strlen(id)+ 1);
    link->state = PENDING;
    link->value = value;
    link->factors = NULL;

    strcpy(link->id, id);

    link->next = head;
    head = link;
}

int result(char* id) {
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

    return 0;
}

int get(char* id) {
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
    return curr->value;
}

int change(char* id, char state, char* factors) {
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
    return 0;
}

void display() {
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
}

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

    change(id, COMPLETE0, res);
    vTaskDelete(NULL);
}

int prepare_factor(int value, char* id) {
    BaseType_t success;
    static char* tag = NULL;
    tag = malloc(strlen(id) + 1);
    strcpy(tag, id);

    insert(tag, value);

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