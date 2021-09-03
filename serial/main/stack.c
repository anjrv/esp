#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include "stack.h"

struct stack
{
    int maxsize;
    int top;
    int *items;
};

struct stack* newStack(int capacity) {
    struct stack *pt = (struct stack*)malloc(sizeof(struct stack));
 
    pt->maxsize = capacity;
    pt->top = -1;
    pt->items = (int*)malloc(sizeof(int) * capacity);
 
    return pt;
}
 
int size(struct stack *pt) {
    return pt->top + 1;
}
 
int isEmpty(struct stack *pt) {
    return pt->top == -1;
}
 
int isFull(struct stack *pt) {
    return pt->top == pt->maxsize - 1;
}
 
void push(struct stack *pt, int x) {
    if (isFull(pt)) {
        exit(EXIT_FAILURE);
    }

    pt->items[++pt->top] = x;
}
 
int peek(struct stack *pt) {
    if (!isEmpty(pt)) {
        return pt->items[pt->top];
    }
    else {
        exit(EXIT_FAILURE);
    }
}
 
int pop(struct stack *pt) {
    if (isEmpty(pt)) {
        exit(EXIT_FAILURE);
    }
 
    return pt->items[pt->top--];
}
