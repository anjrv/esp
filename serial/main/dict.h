#ifndef DICT_H_ 
#define DICT_H_ 

#define CAPACITY 1000

typedef struct {
    char* key;
    int value;
} dict_item;

typedef struct {
    dict_item** items;
    int size;
    int count;
} dict;

dict_item* create_item(char* key, int value);
dict* create_dict(int size);

void store(dict* d, char* key, int value);
int query(dict* d, char* key);

#endif
