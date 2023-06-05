#pragma once

#include "common.h"
#include "value.h"

// Each instruction has a one-byte operational code
// (opcode) that defines its application.
typedef enum {
    OP_CONSTANT,
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_NEGATE,
    OP_RETURN
} op_code_t;

// Represents a dynamic array of instructions. Provides dense storage,
// constant-time indexed lookup and appending to the end of the array.
typedef struct {
    int count;
    int capacity;
    uint8_t *code;
    int *lines;
    value_array_t constants;
} chunk_t;

void init_chunk(chunk_t *chunk);
void free_chunk(chunk_t *chunk);
void write_chunk(chunk_t *chunk, uint8_t byte, int line);
int add_constant(chunk_t *chunk, value_t value);