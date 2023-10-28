#ifndef CHUNK_H
#define CHUNK_H

#include "common.h"
#include "value.h"

/** Opcodes supported by the language's virtual machine. */
typedef enum {
    OP_CONSTANT,
    OP_NIL,
    OP_TRUE,
    OP_FALSE,
    OP_EQUAL,
    OP_GREATER,
    OP_LESS,
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_NOT,
    OP_NEGATE,
    OP_POP,
    OP_GET_LOCAL,
    OP_SET_LOCAL,
    OP_GLOBAL,
    OP_SET_GLOBAL,
    OP_GET_GLOBAL,
    OP_GET_UPVALUE,
    OP_SET_UPVALUE,
    OP_GET_PROPERTY,
    OP_SET_PROPERTY,
    OP_PRINT,
    OP_JUMP,
    OP_JUMP_FALSE,
    OP_LOOP,
    OP_CALL,
    OP_INVOKE,
    OP_CLOSURE,
    OP_CLOSE_UPVALUE,
    OP_CLASS,
    OP_INHERIT,
    OP_METHOD,
    OP_RETURN
} OpCode; /* FIXME: briefly describe each operation. */

// FIXME: Represents a dynamic array of instructions. Provides dense storage,
// constant-time indexed lookup and appending to the end of the array.
typedef struct _Chunk {
    int         count;
    int         capacity;
    uint8_t*    code;
    int*        lines;
    ValueArray  constants;
} Chunk;

// FIXME: Description.
void init_chunk(Chunk* chunk);

// FIXME: Description.
void free_chunk(Chunk* chunk);

// FIXME: Description.
void write_chunk(Chunk* chunk, uint8_t byte, int line);

// FIXME: Description.
int add_constant(Chunk* chunk, Value value);

#endif