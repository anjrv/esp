#include <math.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "factors.h"
#include "serial.h"
#include "utils.h"

typedef struct data_node data_node;
typedef struct task_node task_node;

struct data_node
{
    char *dataset;
    char *source;
    int entries;
    SemaphoreHandle_t task_access;

    struct data_node *next;
    struct task_node *down;
};

struct task_node
{
    char *data;

    struct task_node *next;
    struct data_node *up;
};

struct data_node *data_head = NULL;
SemaphoreHandle_t dataset_access;

/**
 * Initializes the linked list semaphore
 * to be used by data set functions
 */
void initialize_bt_tasks()
{
    dataset_access = xSemaphoreCreateBinary();
    assert(dataset_access != NULL);
    xSemaphoreGive(dataset_access);
}

int dataset_exists(char *name)
{
    data_node *tmp = data_head;

    while (tmp != NULL)
    {
        if (strcmp(tmp->dataset, name) == 0)
            return 1;

        tmp = tmp->next;
    }

    return 0;
}

int insert_dataset(char *name, char *source)
{
    if (xSemaphoreTake(dataset_access, WAIT_QUEUE) != pdTRUE)
    {
        return -1;
    }

    if (dataset_exists(name))
    {
        xSemaphoreGive(dataset_access);
        return -2;
    }

    data_node *link = malloc(sizeof(data_node));

    link->dataset = malloc(strlen(name) + 1);
    strcpy(link->dataset, name);

    link->source = malloc(strlen(source) + 1);
    strcpy(link->source, source);

    link->entries = 0;
    link->task_access = xSemaphoreCreateBinary();

    link->down = NULL;

    link->next = malloc(sizeof(data_node));
    link->next = data_head;
    data_head = link;


    xSemaphoreGive(link->task_access);
    xSemaphoreGive(dataset_access);
    return 0;
}

/**
 * Attempts to create a new empty dataset with the given name
 * 
 * @param name the name to give the new dataset
 * @return 0 if creation was successful, -2 if the name already exists 
 */
int create_dataset(char *name, char *source)
{
    int res = insert_dataset(name, source);

    while (res == -1)
    {
        vTaskDelay(DELAY);
        res = insert_dataset(name, source);
    }

    return res;
}

int remove_entries(SemaphoreHandle_t access, task_node *list)
{
    if (xSemaphoreTake(access, WAIT_QUEUE) != pdTRUE)
    {
        return -1;
    }

    task_node *temp = list;
    while (list)
    {
        temp = list;
        list = list->next;
        free(temp->data);
        free(temp);
    }

    xSemaphoreGive(access);
    return 0;
}

int remove_dataset(char *name)
{
    if (xSemaphoreTake(dataset_access, WAIT_QUEUE) != pdTRUE)
    {
        return -1;
    }

    if (data_head && strcmp(data_head->dataset, name) == 0)
    {
        while (remove_entries(data_head->task_access, data_head->down) == -1)
        {
            vTaskDelay(DELAY);
        }

        free(data_head->dataset);
        free(data_head->source);
        free(data_head->task_access);

        if (data_head->next)
        {
            data_node *nxt = data_head->next;
            free(data_head);
            data_head = nxt;
        }
        else
        {
            data_head = NULL;
        }

        xSemaphoreGive(dataset_access);
        return 0;
    }

    if (data_head->next)
    {
        data_node *tmp_prev = data_head;
        data_node *tmp = data_head->next;

        while (tmp != NULL)
        {
            if (strcmp(tmp->dataset, name) == 0)
            {
                while (remove_entries(tmp->task_access, tmp->down) == -1)
                {
                    vTaskDelay(DELAY);
                }

                free(tmp->dataset);
                free(tmp->source);
                free(tmp->task_access);
                tmp_prev->next = tmp->next;

                free(tmp);

                xSemaphoreGive(dataset_access);
                return 0;
            }

            tmp_prev = tmp;
            tmp = tmp->next;
        }
    }

    xSemaphoreGive(dataset_access);
    return -2;
}

int destroy_dataset(char *name)
{
    int res = remove_dataset(name);

    while (res == -1)
    {
        vTaskDelay(DELAY);
        res = remove_dataset(name);
    }

    return res;
}
