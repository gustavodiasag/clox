#pragma once

#include "common.h"
#include "value.h"

// Each instruction has a one-byte operational code
// (opcode) that defines its application.
typedef enum {
    OP_CONSTANT,
    OP_RETURN,
} OpCode;

// Represents a dynamic array of instructions. Provides dense storage,
// constant-time indexed lookup and appending to the end of the array.
typedef struct {
    int count;
    int capacity;
    uint8_t *code;
    int *lines;
    ValueArray constants;
} Chunk;

void init_chunk(Chunk *chunk);
void free_chunk(Chunk *chunk);
void write_chunk(Chunk *chunk, uint8_t byte, int line);
int add_constant(Chunk *chunk, Value value);