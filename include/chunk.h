#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"
#include "value.h"

/*
 * Each instruction has a one-byte operational code
 * (opcode) that defines its application.
 */
typedef enum {
    OP_CONSTANT,
    OP_RETURN,
} OpCode;

/*
 * Represents a dynamic array of instructions. Some of
 * the benefits provided include dense storage, which
 * is cache-friendly, constant-time indexed element
 * lookup and appending to the end of the array.
 */
typedef struct {
    int count; // Allocated entries in use.
    int capacity; // Number of elements allocated.
    uint8_t *code;
    int *lines;
    ValueArray constants;
} Chunk;

void init_chunk(Chunk *chunk);
void free_chunk(Chunk *chunk);
void write_chunk(Chunk *chunk, uint8_t byte, int line);
int add_constant(Chunk *chunk, Value value);

#endif