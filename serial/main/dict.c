#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "dict.h"

struct dict_item {
    char* key;
    int value;
};

struct dict {
    dict_item** items;
    int size;
    int count;
};

unsigned long hash_function(char* str) {
    unsigned long i = 0;
    for (int j=0; str[j]; j++)
        i += str[j];
    return i % CAPACITY;
}

dict_item* create_item(char* key, int value) {
    dict_item* item = (dict_item*) malloc (sizeof(dict_item));
    item->key = (char*) malloc (strlen(key) + 1);
     
    strcpy(item->key, key);
    item->value = value;
 
    return item;
}

dict* create_dict(int size) {
    dict* d = (dict*) malloc (sizeof(d));
    d->size = size;
    d->count = 0;
    d->items = (dict_item**) calloc (d->size, sizeof(dict_item*));

    for (int i = 0; i < d->size; i++)
        d->items[i] = NULL;
 
    return d;
}

void free_item(dict_item* item) {
    free(item->key);
    free(item);
}
 
void free_dict(dict* d) {
    for (int i = 0; i < d->size; i++) {
        dict_item* item = d->items[i];
        if (item != NULL)
            free_item(item);
    }
 
    free(d->items);
    free(d);
}

void handle_collision(dict* d, dict_item* item) {
    // Uuhh.. linked list later maybe if this is a problem?
}

void store(dict* d, char* key, int value) {
    dict_item* item = create_item(key, value);
    int index = hash_function(key);
 
    dict_item* current_item = d->items[index];
     
    if (current_item == NULL) {
        if (d->count == d->size) {
            exit(EXIT_FAILURE);
        }
         
        d->items[index] = item; 
        d->count++;
    } else {
        if (strcmp(current_item->key, key) == 0) {
            d->items[index]->value = value;
            return;
        } else {
            handle_collision(d, item);
            return;
        }
    }
}

int query(dict* d, char* key) {
    int index = hash_function(key);
    dict_item* item = d->items[index];
 
    if (item != NULL) {
        if (strcmp(item->key, key) == 0)
            return item->value;
    }

    exit(EXIT_FAILURE);
}
