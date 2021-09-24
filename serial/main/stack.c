#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include "stack.h"

/**
 * Creates a new stack struct with the requested capacity
 * 
 * @param capacity how many values should the stack be able to hold
 * @return a pointer to the newly made stack struct
 */
stack* create_stack(int capacity) {
    stack *pt = malloc(sizeof(stack));
    pt->maxsize = capacity;
    pt->top = -1;
    pt->items = malloc(sizeof(int) * capacity);
 
    return pt;
}

/**
 * Returns the current quantity of values in the stack
 * 
 * @param pt pointer to the stack to check
 * @return the quantity of values in the stack
 */ 
int stack_size(stack *pt) {
    return pt->top + 1;
}
 
/**
 * Returns whether the stack is empty or not
 * 
 * @param pt pointer to the stack to check 
 * @return a boolean which indicates if the stack is empty
 */ 
int is_stack_empty(stack *pt) {
    return pt->top == -1;
}

/**
 * Returns whether the stack is full or not
 * 
 * @param pt pointer to the stack to check
 * @return a boolean which indicates if the stack is full 
 */ 
int is_stack_full(stack *pt) {
    return pt->top == pt->maxsize - 1;
}

/**
 * Pushes the int x onto the stack if it is not yet full
 * 
 * If the stack is full it will exit with a non 0 condition
 * 
 * @param x the int to push onto the stack
 * @param pt pointer to the stack to push to
 */ 
void push(stack *pt, int x) {
    if (is_stack_full(pt)) {
        exit(EXIT_FAILURE);
    }

    pt->items[++pt->top] = x;
}

/**
 * Returns the value of the first int on the stack
 * without popping it
 * 
 * If the stack is empty it will exit with a non 0 condition
 *
 * @param pt pointer to the stack to peek in 
 */ 
int peek(stack *pt) {
    if (!is_stack_empty(pt)) {
        return pt->items[pt->top];
    }
    else {
        exit(EXIT_FAILURE);
    }
}

/**
 * Pops the first value on the stack and returns it
 * If the stack is empty it will exit with a non 0 condition
 * 
 * @param pt pointer to the stack to pop
 */ 
int pop(stack *pt) {
    if (is_stack_empty(pt)) {
        exit(EXIT_FAILURE);
    }
 
    return pt->items[pt->top--];
}
