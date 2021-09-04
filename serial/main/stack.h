#ifndef STACK_H_
#define STACK_H_ 

typedef struct {
    int maxsize;    
    int top;
    int *items;
} stack;
 
stack* newStack(int capacity);
int size(stack *pt);
int isEmpty(stack *pt);
int isFull(stack *pt);
void push(stack *pt, int x);
int peek(stack *pt);
int pop(stack *pt);

#endif
