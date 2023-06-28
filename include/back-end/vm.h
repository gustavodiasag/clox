#pragma once

#include "chunk.h"
#include "value.h"

#define STACK_MAX 256

typedef struct {
    chunk_t *chunk;
    // Keeps track of where the instruction about to be executed is.
    uint8_t *ip;
    value_t stack[STACK_MAX]; 
    value_t *stack_top;
    // Linked list that stores every object allocated.
    obj_t *objects;
} vm_t;

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
} interpret_result_t;

// Exposes the global variable so that it can be used
// on other modules.
extern vm_t vm;

void init_vm();
void free_vm();
interpret_result_t interpret(const char *source);
void push(value_t value);
value_t pop();