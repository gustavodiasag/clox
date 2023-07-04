#pragma once

#include "chunk.h"
#include "table.h"
#include "value.h"

#define STACK_MAX 256

typedef struct {
    chunk_t *chunk;
    // Keeps track of where the instruction about to be executed is.
    uint8_t *ip;
    value_t stack[STACK_MAX]; 
    value_t *stack_top;
    // Stores all strings created in the program.
    table_t strings;
    // Linked list keeping track of every heap-allocated object.
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