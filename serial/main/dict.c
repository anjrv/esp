#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "dict.h"

key_list* new_list() {
    key_list* res = (key_list*) malloc (sizeof(key_list));
    res->count = 0;

    return res;
}

void free_list(key_list* collection) {
    if (collection == NULL) {
        return;
    }

    for (size_t i = 0; i < collection->count; i++) {
        free(collection->pairs[i].key);
    }

    free(collection);
}

int store(key_list* collection, int value, char* key) {
    // Need to make this iterate through and find the key
    // TODO

    if (collection->count == CAPACITY) {
        return 0;
    }

    int count = collection->count;
    collection->pairs[count].key = strdup(key);
    collection->pairs[count].value = value;
    collection->count++;

    return 1;
}

int query(key_list* collection, int* ptr, char* key) {
    for (size_t i = 0; i < collection->count; i++) {
        if(strcmp(collection->pairs[i].key, key) == 0) {
            *ptr = collection->pairs[i].value;

            return 1;
        }
    } 

    return 0;
}

// As it turns out once overflow protection is added this is a bit problematic for memory
// ......
//
// static linked_list* allocate_list() {
//     linked_list* list = (linked_list*) malloc (sizeof(linked_list));
//     return list;
// }
// 
// static linked_list* list_insert(linked_list* list, dict_item* item) {
//     if (!list) {
//         linked_list* head = allocate_list();
//         head->item = item;
//         head->next = NULL;
//         list = head;
//         return list;
//     } else if (list->next == NULL) {
//         linked_list* node = allocate_list();
//         node->item = item;
//         node->next = NULL;
//         list->next = node;
//         
//         return list;
//     }
// 
//     linked_list* temp = list;
//     while (temp->next->next) {
//         temp = temp->next;
//     }
// 
//     linked_list* node = allocate_list();
//     node->item = item;
//     node->next = NULL;
//     temp->next = node;
// 
//     return list;
// }
// 
// static void free_list(linked_list* list) {
//     linked_list* temp = list;
//     while (list) {
//         temp = list;
//         list = list->next;
// 
//         free(temp->item->key);
//         free(temp->item);
//         free(temp);
//     }
// }
// 
// static linked_list** create_overflow(dict* d) {
//     linked_list** buckets = (linked_list**) calloc (d->size, sizeof(linked_list*));
//     for (int i=0; i<d->size; i++) {
//         buckets[i] = NULL;
//     }
// 
//     return buckets;
// }
// 
// static void free_overflow(dict* d) {
//     linked_list** buckets = d->overflow;
//     for (int i=0; i<d->size; i++) {
//         free_list(buckets[i]);
//     }
// 
//     free(buckets);
// }
// 
// /**
//  * Helper function for collision handling of 
//  * key indexes
//  * 
//  * @param d     the dictionary being used
//  * @param index the index of the collision
//  * @param item  the item causing the collision
//  */
// void handle_collision(dict* d, unsigned long index, dict_item* item) {
//     linked_list* head = d->overflow[index];
//  
//     if (head == NULL) {
//         head = allocate_list();
//         head->item = item;
//         d->overflow[index] = head;
//         return;
//     }
//     else {
//         d->overflow[index] = list_insert(head, item);
//         return;
//     }
// }
// 
// /**
//  * Helper function for index hashing
//  * 
//  * @param   str the key to be indexed
//  * @return  the created index of the given key
//  */
// unsigned long hash_function(char* str) {
//     unsigned long i = 0;
//     for (int j=0; str[j]; j++)
//         // Small prime of 31
//         i = (31 * i + str[j]) % CAPACITY;
//     
//     return i;
// }
// 
// /**
//  * Helper function to create a dictionary key value pair
//  * 
//  * @param key   the pointer to the start of the key String
//  * @param value the int value to be stored at the key 
//  * @return a pointer to the newly created dictionary item
//  */
// dict_item* create_item(char* key, int value) {
//     dict_item* item = (dict_item*) malloc (sizeof(dict_item));
//     item->key = (char*) malloc (strlen(key) + 1);
//      
//     strcpy(item->key, key);
//     item->value = value;
//  
//     return item;
// }
// 
// /**
//  * Function to create a dictionary object of the given capacity
//  * NOTE: Capacity is stored as a constant in dict.h
//  * 
//  * @param size the capacity to allocate to the new dictionary
//  * @return a pointer to the newly created dictionary  
//  */
// dict* create_dict(int size) {
//     dict* d = (dict*) malloc (sizeof(d));
//     d->size = size;
//     d->count = 0;
//     d->items = (dict_item**) calloc (d->size, sizeof(dict_item*));
// 
//     for (int i = 0; i < d->size; i++) {
//         d->items[i] = NULL;
//     }
// 
//     d->overflow = create_overflow(d);
//  
//     return d;
// }
// 
// /**
//  * Helper function to free the memory allocated
//  * by a dictionary item 
//  * 
//  * @param item the item to discard 
//  */
// void free_item(dict_item* item) {
//     free(item->key);
//     free(item);
// }
// 
// /**
//  * Function to free the memory allocated
//  * by a dictionary 
//  * 
//  * @param d the dictionary to discard 
//  */ 
// void free_dict(dict* d) {
//     for (int i = 0; i < d->size; i++) {
//         dict_item* item = d->items[i];
//         if (item != NULL)
//             free_item(item);
//     }
//  
//     free_overflow(d);
//     free(d->items);
//     free(d);
// }
// 
// /**
//  * Function to check whether the storage
//  * of a dictionary is full 
//  * 
//  * @param d the dictionary to check 
//  * @return an int true/false describing whether
//  *         the dictionary is full
//  */
// int is_dict_full(dict* d) {
//     return d->count == d->size;
// }
// 
// /**
//  * Function to store a new key value pair in the
//  * given dictionary
//  * 
//  * @param d     the dictionary to use for storage
//  * @param key   the key of the item to be stored
//  * @param value the value to be stored at that key 
//  */
// void store(dict* d, char* key, int value) {
//     dict_item* item = create_item(key, value);
//     unsigned long index = hash_function(key);
//  
//     dict_item* current_item = d->items[index];
//      
//     if (current_item == NULL) {
//         // Should be manually checked before
//         // inserting into the dictionary
//         if (is_dict_full(d)) {
//             free_item(item);
//             return;
//         }
//          
//         d->items[index] = item; 
//         d->count++;
//     } else {
//         if (strcmp(current_item->key, key) == 0) {
//             d->items[index]->value = value;
//             return;
//         } else {
//             handle_collision(d, index, item);
//             return;
//         }
//     }
// }
// 
// /**
//  * Function to search for a stored item in a given dictionary
//  * 
//  * NOTE: Some nonsense pointer passing was done here to be able
//  *       to return values that indicate whether a pair was actually
//  *       found or not ...
//  * 
//  *       Cannot indicate TRUE/FALSE with a simple int return: 
//  *       Values such as 0 and -1 are valid integers that can be stored
//  * 
//  * @param d     the dictionary to search in
//  * @param key   the key to search for 
//  * @param ptr   the pointer to store the result
//  * @return TRUE/FALSE referring to whether the key
//  *         was found
//  */
// int query(dict* d, char* key, int* ptr) {
//     unsigned long index = hash_function(key);
//     dict_item* item = d->items[index];
//     linked_list* head = d->overflow[index];
//  
//     while (item != NULL) {
//         if (strcmp(item->key, key) == 0) {
//             *ptr = item->value;
// 
//             return 1;
//         }
// 
//         if (head == NULL) {
//             return 0;
//         }
// 
//         item = head->item;
//         head = head->next;
//     }
// 
//     return 0;
// }
// 
// void dict_delete(dict* d, char* key) {
//     int index = hash_function(key);
//     dict_item* item = d->items[index];
//     linked_list* head = d->overflow[index];
//  
//     if (item == NULL) {
//         return;
//     } else {
//         if (head == NULL && strcmp(item->key, key) == 0) {
//             d->items[index] = NULL;
//             free_item(item);
//             d->count--;
//             return;
//         } else if (head != NULL) {
//             if (strcmp(item->key, key) == 0) {
//                 free_item(item);
//                 linked_list* node = head;
//                 head = head->next;
//                 node->next = NULL;
//                 d->items[index] = create_item(node->item->key, node->item->value);
//                 free_list(node);
//                 d->overflow[index] = head;
//                 return;
//             }
//  
//             linked_list* curr = head;
//             linked_list* prev = NULL;
//              
//             while (curr) {
//                 if (strcmp(curr->item->key, key) == 0) {
//                     if (prev == NULL) {
//                         free_list(head);
//                         d->overflow[index] = NULL;
//                         return;
//                     }
//                     else {
//                         prev->next = curr->next;
//                         curr->next = NULL;
//                         free_list(curr);
//                         d->overflow[index] = head;
//                         return;
//                     }
//                 }
// 
//                 curr = curr->next;
//                 prev = curr;
//             }
//         }
//     }
// }
// 