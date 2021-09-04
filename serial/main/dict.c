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

/**
 * Helper function for index hashing
 * 
 * @param   str the key to be indexed
 * @return  the created index of the given key
 */
unsigned long hash_function(char* str) {
    unsigned long i = 0;
    for (int j=0; str[j]; j++)
        i += str[j];
    
    // Hash is calculated from the constant CAPACITY
    // see dict.h for current maximum capacity
    return i % CAPACITY;
}

/**
 * Helper function to create a dictionary key value pair
 * 
 * @param key   the pointer to the start of the key String
 * @param value the int value to be stored at the key 
 * @return a pointer to the newly created dictionary item
 */
dict_item* create_item(char* key, int value) {
    dict_item* item = (dict_item*) malloc (sizeof(dict_item));
    item->key = (char*) malloc (strlen(key) + 1);
     
    strcpy(item->key, key);
    item->value = value;
 
    return item;
}

/**
 * Function to create a dictionary object of the given capacity
 * NOTE: Capacity is stored as a constant in dict.h
 * 
 * @param size the capacity to allocate to the new dictionary
 * @return a pointer to the newly created dictionary  
 */
dict* create_dict(int size) {
    dict* d = (dict*) malloc (sizeof(d));
    d->size = size;
    d->count = 0;
    d->items = (dict_item**) calloc (d->size, sizeof(dict_item*));

    for (int i = 0; i < d->size; i++)
        d->items[i] = NULL;
 
    return d;
}

/**
 * Helper function to free the memory allocated
 * by a dictionary item 
 * 
 * @param item the item to discard 
 */
void free_item(dict_item* item) {
    free(item->key);
    free(item);
}

/**
 * Function to free the memory allocated
 * by a dictionary 
 * 
 * @param d the dictionary to discard 
 */ 
void free_dict(dict* d) {
    for (int i = 0; i < d->size; i++) {
        dict_item* item = d->items[i];
        if (item != NULL)
            free_item(item);
    }
 
    free(d->items);
    free(d);
}

/**
 * Function to check whether the storage
 * of a dictionary is full 
 * 
 * @param d the dictionary to check 
 * @return an int true/false describing whether
 *         the dictionary is full
 */
int is_dict_full(dict* d) {
    return d->count == d->size;
}

/**
 * Helper function for collision handling of 
 * key indexes
 * 
 * TODO: Handle with some sort of overflow data structure? 
 * 
 * @param d     the dictionary being used
 * @param item  the item causing the collision
 */
void handle_collision(dict* d, dict_item* item) {
    // Uuhh.. linked list later maybe if this is a problem?
}

/**
 * Function to store a new key value pair in the
 * given dictionary
 * 
 * @param d     the dictionary to use for storage
 * @param key   the key of the item to be stored
 * @param value the value to be stored at that key 
 */
void store(dict* d, char* key, int value) {
    dict_item* item = create_item(key, value);
    int index = hash_function(key);
 
    dict_item* current_item = d->items[index];
     
    if (current_item == NULL) {
        // Should be manually checked before
        // inserting into the dictionary
        //
        // If user did not then kill the function
        if (is_dict_full(d)) {
            exit(EXIT_FAILURE);
        }
         
        d->items[index] = item; 
        d->count++;
    } else {
        if (strcmp(current_item->key, key) == 0) {
            d->items[index]->value = value;
            return;
        } else {
            // Collision handling is TODO
            handle_collision(d, item);
            return;
        }
    }
}

/**
 * Function to search for a stored item in a given dictionary
 * 
 * @param d     the dictionary to search in
 * @param key   the key to search for 
 * @return a pointer to the value stored at the key, 
 *         a null pointer if nothing is found
 */
int* query(dict* d, char* key) {
    int index = hash_function(key);
    dict_item* item = d->items[index];
 
    if (item != NULL) {
        if (strcmp(item->key, key) == 0) {
            // Initialize static variable
            static int val = 0;

            // Assign value of item
            val = item->value;

            // Send pointer to value
            return (&val);
        }
    }

    return NULL;
}
