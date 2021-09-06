#ifndef DICT_H_ 
#define DICT_H_ 

typedef struct {
    char* key;
    int value;
} key_value;

typedef struct {
    key_value** pairs;
    size_t capacity;
    size_t count;
} key_list;

key_list* new_list();
void free_list(key_list* collection);
int store(key_list* collection, int value, char* key, int* ptr);
int query(key_list* collection, int* ptr, char* key);

// #define CAPACITY 50
//
// typedef struct {
//     char* key;
//     int value;
// } dict_item;
// 
// typedef struct linked_list linked_list;
// 
// struct linked_list {
//     dict_item* item;
//     linked_list* next;
// };
// 
// typedef struct {
//     dict_item** items;
//     linked_list** overflow;
//     int size;
//     int count;
// } dict;
// 
// dict* create_dict(int size);
// 
// int is_dict_full(dict* d);
// void store(dict* d, char* key, int value);
// int query(dict* d, char* key, int* ptr);

#endif
