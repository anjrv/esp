#include <math.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "tasks.h"
#include "serial.h"
#include "utils.h"
#include "noise.h"
#include "data_tasks.h"
#include "client.h"

// Constants for easier storage of a function state
#define NOISE 'N'
#define BT_DEMO 'B'
#define RESPONSE_LENGTH 128

typedef struct node node;

struct node
{
    char *id;
    char *task;
    char state;
    int value;
    char *results;

    struct node *next;
};

static struct node *head = NULL;
static SemaphoreHandle_t head_access;

/**
 * Initializes the linked list semaphore
 * to be used by factor functions
 */
void initialize_tasks()
{
    head_access = xSemaphoreCreateBinary();
    assert(head_access != NULL);
    xSemaphoreGive(head_access);
}

/**
 * Helper function to check whether list is initialized or not
 */
int is_empty()
{
    return head == NULL;
}

/**
 * Function to insert a node with an id and value at the start
 * of the factoring linked list
 * 
 * @param id the id to use for the node
 * @param task the type of task
 * @param value the value to be used for the node
 * 
 * @return the process exit state of the insertion attempt
 *         * -1 indicates that obtaining a semaphore failed
 *         * 0 indicates that insertion was a success
 */
int insert_task(char *id, char *task, int value)
{
    if (xSemaphoreTake(head_access, WAIT_QUEUE) != pdTRUE)
    {
        return -1;
    }

    node *link = malloc(sizeof(node));

    link->id = malloc(strlen(id) + 1);
    link->task = malloc(strlen(task) + 1);
    link->state = PENDING;

    // Same invalid factor as 1 so well keep -1
    // to indicate semaphore state
    if (value == -1)
    {
        link->value = abs(value);
    }
    else
    {
        link->value = value;
    }

    link->results = NULL;

    strcpy(link->id, id);
    strcpy(link->task, task);

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
int result(char *id)
{
    if (xSemaphoreTake(head_access, WAIT_QUEUE) != pdTRUE)
    {
        return -1;
    }

    if (is_empty())
    {
        serial_out("undefined");
        return 1;
    }

    node *curr = head;

    while (strcmp(curr->id, id) != 0)
    {
        if (curr->next == NULL)
        {
            serial_out("undefined");
            return 1;
        }
        else
        {
            curr = curr->next;
        }
    }

    if (curr->state == COMPLETE0 || curr->state == COMPLETE1)
    {
        curr->state = COMPLETE1;
        serial_out(curr->results);
    }
    else
    {
        serial_out("incomplete");
    }

    xSemaphoreGive(head_access);
    return 0;
}

void get_result(char *id)
{
    while (result(id) == -1)
    {
        vTaskDelay(DELAY);
    }
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
int get_task(char *id)
{
    if (xSemaphoreTake(head_access, WAIT_QUEUE) != pdTRUE)
    {
        return -1;
    }

    if (is_empty())
    {
        xSemaphoreGive(head_access);
        return 1;
    }

    node *curr = head;

    while (strcmp(curr->id, id) != 0)
    {
        if (curr->next == NULL)
        {
            xSemaphoreGive(head_access);
            return 1;
        }
        else
        {
            curr = curr->next;
        }
    }

    curr->state = ACTIVE;

    xSemaphoreGive(head_access);
    return curr->value;
}

/**
 * Function to change a node of the task list
 * 
 * @param id the id to use for the node
 * @param state the state to update to
 * @param results the results of the task
 * 
 * @return the process exit state of the attempt
 *         * -1 indicates that obtaining a semaphore failed
 *         * 0 indicates that changing was a success
 */
int change_task(char *id, char state, char *results)
{
    if (xSemaphoreTake(head_access, WAIT_QUEUE) != pdTRUE)
    {
        return -1;
    }

    if (is_empty())
    {
        xSemaphoreGive(head_access);
        return 1;
    }

    node *curr = head;

    while (strcmp(curr->id, id) != 0)
    {
        if (curr->next == NULL)
        {
            xSemaphoreGive(head_access);
            return 1;
        }
        else
        {
            curr = curr->next;
        }
    }

    if (results != NULL)
    {
        curr->results = malloc(strlen(results) + 1);
        strcpy(curr->results, results);
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
int display()
{
    if (xSemaphoreTake(head_access, WAIT_QUEUE) != pdTRUE)
    {
        return -1;
    }

    struct node *ptr = head;

    if (is_empty())
    {
        serial_out("empty set");
    }
    else
    {
        char res[RESPONSE_LENGTH];
        while (ptr != NULL)
        {
            memset(res, 0, RESPONSE_LENGTH);

            if (ptr->state == ACTIVE)
            {
                snprintf(res, RESPONSE_LENGTH, "%s %s %s", ptr->task, ptr->id, "active");
            }
            else if (ptr->state == PENDING)
            {
                snprintf(res, RESPONSE_LENGTH, "%s %s %s", ptr->task, ptr->id, "pending");
            }
            else if (ptr->state == COMPLETE0)
            {
                snprintf(res, RESPONSE_LENGTH, "%s %s %s", ptr->task, ptr->id, "complete");
            }
            else
            {
                snprintf(res, RESPONSE_LENGTH, "%s %s %s", ptr->task, ptr->id, "complete*");
            }

            serial_out(res);
            ptr = ptr->next;
        }
    }

    xSemaphoreGive(head_access);
    return 0;
}

/**
 * Print factor tasks out to serial
 */
void display_factors()
{
    while (display() == -1)
    {
        vTaskDelay(DELAY);
    }
}

/**
 * Main factoring function 
 * 
 * @param pvParameter should be the id of the node that the function needs to work on
 *                    the memory of this pointer is freed at the end of factoring
 */
void factor(void *pvParameter)
{
    char *id;
    id = (char *)pvParameter;

    int num = get_task(id);
    while (num == -1)
    {
        vTaskDelay(DELAY);
        num = get_task(id);
    }

    char res[RESPONSE_LENGTH];
    snprintf(res, 10, "%d", num);
    strcat(res, ":");

    // 0 and 1 are not valid requests, output just mirrors input for now to provide a number
    if (num == 0)
    {
        strcat(res, " 0");
        change_task(id, COMPLETE0, res);
        free(id);
        vTaskDelete(NULL);
    }

    if (num == 1)
    {
        strcat(res, " 1");
        change_task(id, COMPLETE0, res);
        free(id);
        vTaskDelete(NULL);
    }

    if (num < 0)
    {
        // Is negative, use absolute
        strcat(res, " -1");
        num = abs(num);
    }

    // Repeat until odd number
    while (num % 2 == 0)
    {
        vTaskDelay(DELAY);
        // Divisible by 2
        strcat(res, " 2");
        num = num / 2;
    }

    for (int i = 3; i <= sqrt(num); i += 2)
    {
        vTaskDelay(DELAY);
        while (num % i == 0)
        {
            // Divisible by i
            strcat(res, " ");
            strcat(res, long_to_string(i));
            num = num / i;
        }
    }

    if (num > 2)
    {
        // Remaining prime
        strcat(res, " ");
        strcat(res, long_to_string(num));
    }

    // Try write result to list
    while (change_task(id, COMPLETE0, res) == -1)
    {
        vTaskDelay(DELAY);
    }

    free(id);
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
int prepare_factor(int value, char *id)
{
    BaseType_t success;
    char *tag = NULL;
    // Task frees this pointer before deletion
    tag = malloc(strlen(id) + 1);
    strcpy(tag, id);

    while (insert_task(tag, "factor", value) == -1)
    {
        vTaskDelay(DELAY);
    }

    success = xTaskCreatePinnedToCore(
        &factor,
        id,
        2048,
        (void *)tag,
        LOW_PRIORITY,
        NULL,
        tskNO_AFFINITY);

    if (success == pdPASS)
    {
        return 0;
    }

    return 1;
}

int get_data_row(char src, char *buf)
{
    if (src == NOISE)
    {
        noise(buf);
        return 0;
    }

    return -1;
}

/**
 * Dataset worker that fetches from the noise source. 
 * 
 * Because noise is locally available we don't concern ourselves with storing rows 
 * within the function itself, we simply attempt to get a row of noise and then request 
 * that it be appended to the dataset, any semaphore acquisition is waited for.
 * 
 * @param pvParameter in this case we take in a composite char* pointer which gives us
 *                    both the dataset we should append to and the ID of the task for state management
 */
void append_noise(void *pvParameter)
{
    char *id;
    id = (char *)pvParameter;

    // Split to find dataset and ID
    char **split = NULL;
    char *p = strtok(id, " ");
    int quant = 0;

    while (p)
    {
        split = realloc(split, sizeof(char *) * ++quant);
        split[quant - 1] = p;

        p = strtok(NULL, " ");
    }

    split = realloc(split, sizeof(char *) * (quant + 1));
    split[quant] = '\0';

    int iter = get_task(split[0]);
    while (iter == -1)
    {
        vTaskDelay(DELAY);
        iter = get_task(split[0]);
    }

    int i = 0;
    int exists = 1;

    char buf[40];
    while (i < iter && exists)
    {
        vTaskDelay(DELAY);
        noise(buf);
        char *c = strdup(buf);

        if (add_entry(split[1], c) != 0)
            exists = 0;

        free(c);
        i++;
    }

    if (exists)
    {
        snprintf(buf, sizeof(buf), "%d %s %s", i, "entries", "recovered");
    }
    else
    {
        snprintf(buf, sizeof(buf), "%s", "early termination: set destroyed");
    }

    while (change_task(split[0], COMPLETE0, buf) == -1)
    {
        vTaskDelay(DELAY);
    }

    free(split);
    free(id);

    vTaskDelete(NULL);
}

/**
 * Entry point function to create an append task with the given
 * value ( in this case it is used as the amount of rows to try fetch ) 
 * current incremental id and the dataset to append rows to. 
 * 
 * A worker thread will be created according to the source the dataset uses
 * 
 * @param value the number of rows to try fetch
 * @param id the current task id
 * @param dataset the dataset to append to
 * 
 * @return an id: 
 *         1 if creating the task was unsuccessful, this indicates lack of memory, 
 *        -2 a source was not found ( dataset does not exist ), 
 *        -3 source is unavailable ( bt_demo with no connection ), 
 *         0 task successfully created, 
 */
int prepare_append(int value, char *id, char *dataset)
{
    BaseType_t success;
    char *tag = NULL;
    tag = malloc((strlen(id) + strlen(dataset) + 2));
    strcpy(tag, id);
    strcat(tag, " ");
    strcat(tag, dataset);

    char *ptr = dataset_get_source(dataset);

    if (ptr == NULL)
    {
        // No datasource, dont create task
        // Do not free the null pointer
        free(tag);
        return -2;
    }

    while (insert_task(id, "data_append", value) == -1)
    {
        vTaskDelay(DELAY);
    }

    if (strcmp(strupr(ptr), "NOISE") == 0)
    {
        success = xTaskCreatePinnedToCore(
            &append_noise,
            id,
            2048,
            (void *)tag,
            LOW_PRIORITY,
            NULL,
            tskNO_AFFINITY);
    }
    else
    {
        // No source
        if (!bt_demo_available())
            return -3;

        // Give BT append more memory to store rows while polling
        success = xTaskCreatePinnedToCore(
            &append_bt,
            id,
            2048 + 64 * value,
            (void *)tag,
            LOW_PRIORITY,
            NULL,
            tskNO_AFFINITY);
    }

    // Free the requested copy of the dataset source
    free(ptr);

    if (success == pdPASS)
    {
        return 0;
    }

    return 1;
}

/**
 * Entry point function to create an stat task with the given
 * value ( in this case it is used as the column number to work with ) 
 * current incremental id and the dataset to append rows to. 
 * 
 * @param value the column to work with
 * @param id the current task id
 * @param dataset the dataset to work with 
 * 
 * @return an id: 
 *         1 if creating the task was unsuccessful, this indicates lack of memory, 
 *        -2 a dataset was not found
 *        -3 an empty dataset was found
 *         0 task successfully created, 
 */
int prepare_stat(int value, char *id, char *dataset)
{
    BaseType_t success;
    char *tag = NULL;
    tag = malloc((strlen(id) + strlen(dataset) + 2));
    strcpy(tag, id);
    strcat(tag, " ");
    strcat(tag, dataset);

    // This mostly provides a scuffed way to check if the dataset is present
    int n = dataset_entry_count(dataset);
    if (n == -2)
    {
        // No datasource, dont create task
        // Do not free the null pointer
        free(tag);
        return -2;
    }
    else if (n == 0)
    {
        free(tag);
        return -3;
    }

    // Value in this case is the index of the column
    while (insert_task(id, "data_stat", value) == -1)
    {
        vTaskDelay(DELAY);
    }

    // The memory for this function is somewhat weird
    // Hard to guarantee enough memory with static allocation
    // The number of entries will increase the amount of memory needed
    // Therefore we add 64 for each entry to prevent stack overflow
    success = xTaskCreatePinnedToCore(
        &parse_dataset,
        id,
        2048 + 64 * n,
        (void *)tag,
        LOW_PRIORITY,
        NULL,
        tskNO_AFFINITY);

    if (success == pdPASS)
    {
        return 0;
    }

    return 1;
}
