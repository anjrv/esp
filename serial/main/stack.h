#ifndef STACK_H_
#define STACK_H_ 

#define STACK_SIZE 32

typedef struct {
    int maxsize;    
    int top;
    int *items;
} stack;
 
stack* create_stack(int capacity);
int stack_size(stack *pt);
int is_stack_empty(stack *pt);
int is_stack_full(stack *pt);
void push(stack *pt, int x);
int peek(stack *pt);
int pop(stack *pt);

#endif
