#ifndef STACK_H_
#define STACK_H_ 

typedef struct {
    int maxsize;    
    int top;
    int *items;
} stack;
 
struct stack* newStack(int capacity);
 
int size(struct stack *pt);
 
int isEmpty(struct stack *pt);
 
int isFull(struct stack *pt);
 
void push(struct stack *pt, int x);
 
int peek(struct stack *pt);
 
int pop(struct stack *pt);

#endif
