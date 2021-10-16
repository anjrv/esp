#ifndef DICT_H_
#define DICT_H_

#define DICT_CAPACITY 1000

typedef struct
{
    char *key;
    int value;
} dict_item;

typedef struct linked_list linked_list;

struct linked_list
{
    dict_item *item;
    linked_list *next;
};

typedef struct
{
    dict_item **items;
    linked_list **overflow;
    int size;
    int count;
} dict;

dict *create_dict(int size);

int is_dict_full(dict *d);
void store(dict *d, char *key, int value);
int query(dict *d, char *key, int *ptr);

#endif
