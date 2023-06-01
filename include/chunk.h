#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"a

/*
 * Each instruction has a one-byte operational code
 * (opcode) that defines its application.
 */
typedef enum {
    OP_RETURN, // Return from the current function.
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
} Chunk;

#endif