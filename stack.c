// stack.c
// Implements stack API declared in stack.h

#include <stdlib.h>
#include <stdio.h>
#include "stack.h"
#include "stack_node.h"

/* create a new stack */
stack_t *make_stack(void) {
    stack_t *s = malloc(sizeof(*s));
    if (!s) {
        perror("make_stack malloc");
        return NULL;
    }
    s->top = NULL;
    return s;
}

/* push data onto the stack (allocates a new stack_node) */
void push(stack_t *stack, void *data) {
    if (!stack) return;
    stack_node_t *node = malloc(sizeof(*node));
    if (!node) {
        perror("push malloc");
        exit(EXIT_FAILURE);
    }
    node->data = data;
    node->next = stack->top;
    stack->top = node;
}

/* return top element pointer; exit on empty as header requires */
void *top(stack_t * stack) {
    if (!stack || stack->top == NULL) {
        fprintf(stderr, "attempt to retrieve top of an empty stack\n");
        exit(EXIT_FAILURE);
    }
    return stack->top->data;
}

/* pop (remove) top element and free node; exit on empty */
void pop(stack_t *stack) {
    if (!stack || stack->top == NULL) {
        fprintf(stderr, "attempt to pop from an empty stack\n");
        exit(EXIT_FAILURE);
    }
    stack_node_t *n = stack->top;
    stack->top = n->next;
    free(n);
}

/* return 0 if not empty, non-zero otherwise */
int empty_stack(stack_t * stack) {
    if (!stack) return 1;
    return (stack->top == NULL);
}

/* free entire stack and nodes; does not free node->data (caller should) */
void free_stack(stack_t * stack) {
    if (!stack) return;
    stack_node_t *cur = stack->top;
    while (cur) {
        stack_node_t *next = cur->next;
        free(cur);
        cur = next;
    }
    free(stack);
}
