#include <math.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "tasks.h"
#include "serial.h"
#include "utils.h"
#include "noise.h"

typedef struct data_node data_node;
typedef struct task_node task_node;

struct data_node
{
    char *dataset;
    char *source;
    int entries;
    int memory;
    SemaphoreHandle_t task_access;

    struct data_node *next;
    struct task_node *down;
};

struct task_node
{
    char *data;

    struct task_node *next;
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

/**
 * Helper function to validate whether a dataset 
 * with the given name identifier exists. 
 *
 * This does not check whether it has 
 * the dataset_access semaphore, it needs to be 
 * obtained by the calling function before use
 * 
 * @param name the name of the dataset to look for
 */
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

/**
 * Helper function to find the listed source of a 
 * given dataset. 
 *
 * This does not check whether it has 
 * the dataset_access semaphore, it needs to be 
 * obtained by the calling function before use
 * 
 * @param name the name of the dataset to look for
 * @return a memory allocated copy of the source
 */
char *dataset_source(char *name)
{
    data_node *tmp = data_head;

    while (tmp != NULL)
    {
        if (strcmp(tmp->dataset, name) == 0)
        {
            char *res = strdup(tmp->source);
            return res;
        }

        tmp = tmp->next;
    }

    return NULL;
}

/**
 * Wrapper function to obtain the name of the source for a given dataset
 * Provides a semaphore waiting loop. 
 * 
 * The source obtained from this function is a memory allocated copy 
 * and must be freed by the caller.
 * 
 * @param the name of the dataset to look for
 * @return a memory allocated copy of the source
 */
char *get_source(char *name)
{
    while (xSemaphoreTake(dataset_access, WAIT_QUEUE) != pdTRUE)
    {
        vTaskDelay(DELAY);
    }

    char *res = dataset_source(name);
    xSemaphoreGive(dataset_access);

    return res;
}

/**
 * Creates an empty dataset, gives it a source 
 * and attempts to prepend it to the list of datasets
 * 
 * @param name the name of the dataset
 * @param source the source the dataset uses
 * @return an int:
 *         -1 for failure to obtain dataset semaphore, 
 *         -2 if a dataset with the given name already exists, 
 *          0 for normal exit
 */
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

    link->dataset = strdup(name);
    link->source = strdup(source);
    link->entries = 0;
    // Initialize memory with all the crap we stick into the data set head node, can then just add to it whenever tasks append
    // Values are the chars of the name and source strings (incl. null terminators), the entries and memory integers, the semaphore flag and the node pointers
    link->memory = ((strlen(name) + 1) + (strlen(source) + 1) + (sizeof(int) * 2) + sizeof(data_node) + sizeof(SemaphoreHandle_t));
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
 * * @return an int:
 *           -2 if a dataset with the given name already exists, 
 *            0 for normal exit
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

/**
 * Helper function to free the memory of a list
 * of dataset entries
 * 
 * @param list the first entry in the dataset
 *             ( represented by data_node->down )
 * @return 0 upon successful exit
 */
int remove_entries(task_node *list)
{
    task_node *temp = list;
    while (list)
    {
        temp = list;
        list = list->next;
        free(temp->data);
        free(temp);
    }

    return 0;
}

/**
 * Attempts to remove an existing dataset
 * 
 * @param name the name of the dataset
 * @return an int:
 *         -1 for failure to obtain dataset semaphore, 
 *         -2 if a dataset with the given name was not found,  
 *          0 for normal exit
 */
int remove_dataset(char *name)
{
    if (xSemaphoreTake(dataset_access, WAIT_QUEUE) != pdTRUE)
    {
        return -1;
    }

    if (data_head && strcmp(data_head->dataset, name) == 0)
    {
        while (xSemaphoreTake(data_head->task_access, WAIT_QUEUE) != pdTRUE)
        {
            vTaskDelay(DELAY);
        }

        remove_entries(data_head->down);
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
                while (xSemaphoreTake(tmp->task_access, WAIT_QUEUE) != pdTRUE)
                {
                    vTaskDelay(DELAY);
                }

                remove_entries(tmp->down);
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

/**
 * Wrapper function to remove a given dataset, 
 * provides a semaphore waiting loop
 * 
 * @param name the name of the dataset to remove
 * @return an int:
 *         -2 if a dataset with the given name was not found,  
 *          0 for normal exit
 */
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

/**
 * Attempts to obtain information about a dataset 
 * 
 * @param name the name of the dataset
 * @return an int:
 *         -1 for failure to obtain dataset semaphore, 
 *         -2 if a dataset with the given name was not found, 
 *          0 for normal exit
 */
int query_dataset(char *name)
{
    if (xSemaphoreTake(dataset_access, WAIT_QUEUE) != pdTRUE)
    {
        return -1;
    }

    data_node *tmp = data_head;

    while (tmp != NULL)
    {
        if (strcmp(tmp->dataset, name) == 0)
        {
            char info_buf[50];

            snprintf(info_buf, sizeof(info_buf), "%s %s", "Source:", tmp->source);
            serial_out(info_buf);

            snprintf(info_buf, sizeof(info_buf), "%s %d", "Entries:", tmp->entries);
            serial_out(info_buf);

            snprintf(info_buf, sizeof(info_buf), "%s %d %s", "Memory:", tmp->memory, "B");
            serial_out(info_buf);

            xSemaphoreGive(dataset_access);
            return 0;
        }

        tmp = tmp->next;
    }

    xSemaphoreGive(dataset_access);
    return -2;
}

/**
 * Wrapper function to query a given dataset, 
 * provides a semaphore waiting loop
 * 
 * @param name the name of the dataset to get info from 
 * @return an int:
 *         -2 if a dataset with the given name was not found, 
 *          0 for normal exit
 */
int check_dataset(char *name)
{
    int res = query_dataset(name);

    while (res == -1)
    {
        vTaskDelay(DELAY);
        res = query_dataset(name);
    }

    return res;
}

/**
 * Attempts to insert an entry into a given dataset,
 * provides it's own semaphore wait loops. 
 * 
 * @param name the name of the dataset to append to
 * @param row the row of information to append
 * @return an int:
 *         -2 if the dataset to add to was not found, 
 *          0 for normal exit
 */
int add_entry(char *name, char *row)
{
    while (xSemaphoreTake(dataset_access, WAIT_QUEUE) != pdTRUE)
    {
        vTaskDelay(DELAY);
    }

    data_node *tmp = data_head;

    while (tmp != NULL)
    {
        if (strcmp(tmp->dataset, name) == 0)
        {
            while (xSemaphoreTake(tmp->task_access, WAIT_QUEUE) != pdTRUE)
            {
                vTaskDelay(DELAY);
            }

            task_node *entry = tmp->down;
            task_node *link = malloc(sizeof(task_node));

            link->data = strdup(row);
            link->next = NULL;

            if (entry)
            {
                while (entry->next != NULL)
                {
                    entry = entry->next;
                }
                entry->next = link;
            }
            else
            {
                tmp->down = link;
            }

            tmp->memory += sizeof(task_node) + (strlen(row) + 1);
            tmp->entries++;

            xSemaphoreGive(tmp->task_access);
            xSemaphoreGive(dataset_access);

            return 0;
        }

        tmp = tmp->next;
    }

    xSemaphoreGive(dataset_access);
    return -2;
}

/**
 * Hell function to print raw info about the given dataset, 
 * In general this will handle printing by itself and the return values
 * are mostly for troubleshooting.
 * 
 * @param name the name of the dataset to print information about
 * @param key an identifier value of a valid source key, if -1 is given all columns are printed
 * @return an int:
 *         -4 data lock state ( semaphore not available ), 
 *         -2 dataset with the given name not found, 
 *          0 normal exit state ( note: empty set is not an unusual exit currently ) 
 */
int print_raw_data(char *name, int key)
{
    if (xSemaphoreTake(dataset_access, WAIT_QUEUE) != pdTRUE)
    {
        serial_out("data lock");
        return -4;
    }

    data_node *tmp = data_head;

    while (tmp != NULL)
    {
        if (strcmp(tmp->dataset, name) == 0)
        {
            // No entries, dont even bother requesting task access
            if (tmp->entries == 0)
            {
                serial_out("empty set");
                xSemaphoreGive(dataset_access);

                return 0;
            }

            // Some entries exist, queue for task access
            if (xSemaphoreTake(tmp->task_access, WAIT_QUEUE) != pdTRUE)
            {
                serial_out("data lock");
                return -4;
            }

            // Response buffer
            char info_buf[50];

            // Print header line
            if (key == -1)
            {
                snprintf(info_buf, sizeof(info_buf), "%s:%s %s:%d", "request", tmp->dataset, "entries", tmp->entries);
            }
            else if (key == 3)
            {
                snprintf(info_buf, sizeof(info_buf), "%s:%s.%s %s:%d", "request", tmp->dataset, "main", "entries", tmp->entries);
            }
            else
            {
                snprintf(info_buf, sizeof(info_buf), "%s:%s.%s %s:%d", "request", tmp->dataset, key == 0 ? "a" : key == 1 ? "b"
                                                                                                                          : "c",
                         "entries", tmp->entries);
            }

            serial_out(info_buf);

            int checksum = 0;

            task_node *entry = tmp->down;
            while (entry)
            {
                char **split = NULL;
                char *duplicate = strdup(entry->data);
                char *p = strtok(duplicate, " ");
                int quant = 0;

                while (p)
                {
                    split = realloc(split, sizeof(char *) * ++quant);
                    split[quant - 1] = p;

                    p = strtok(NULL, " ");
                }

                split = realloc(split, sizeof(char *) * (quant + 1));
                split[quant] = '\0';

                if (key == -1 || key == 3)
                {
                    if (quant > 1)
                    {
                        snprintf(info_buf, sizeof(info_buf), "%s:%s %s:%s %s:%s", "a", split[0], "b", split[1], "c", split[2]);
                    }
                    else
                    {
                        snprintf(info_buf, sizeof(info_buf), "%s:%s", "main", split[0]);
                    }
                }
                else
                {
                    snprintf(info_buf, sizeof(info_buf), "%s:%s", key == 0 ? "a" : key == 1 ? "b"
                                                                                            : "c",
                             split[key]);
                }

                int *curr;
                curr = malloc(sizeof(*curr));

                if (key == -1 || key == 3)
                {
                    for (int idx = 0; idx < quant; ++idx)
                    {
                        parse_int(split[idx], curr);
                        checksum += *curr;
                    }
                }
                else
                {
                    parse_int(split[key], curr);
                    checksum += *curr;
                }

                serial_out(info_buf);
                free(curr);
                free(split);
                free(duplicate);
                entry = entry->next;
            }

            snprintf(info_buf, sizeof(info_buf), "%s:%d", "CHK", checksum & 0xFF);
            serial_out(info_buf);
            xSemaphoreGive(tmp->task_access);
            xSemaphoreGive(dataset_access);

            return 0;
        }

        tmp = tmp->next;
    }

    serial_out("invalid name");
    xSemaphoreGive(dataset_access);
    return -2;
}
