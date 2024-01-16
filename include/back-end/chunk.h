#ifndef CHUNK_H
#define CHUNK_H

#include "common.h"
#include "value.h"

/**
 * Operational codes or bytecode instructions used by the virtual machine for
 * interpretation. Each of the them is 8-bit long and require different amounts
 * of operands.
 * 
 * All of the operations involve pushing into and/or popping out of the runtime
 * stack while modifying the values obtained in some way.
 */
typedef enum
{
    /* No operand. */
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
    OP_PRINT,
    OP_CLOSE_UPVALUE,
    OP_INHERIT,
    OP_RETURN,
    /* One operand. */
    OP_CONSTANT,
    OP_GET_LOCAL,
    OP_SET_LOCAL,
    OP_GLOBAL,
    OP_SET_GLOBAL,
    OP_GET_GLOBAL,
    OP_GET_UPVALUE, 
    OP_SET_UPVALUE,
    OP_GET_PROPERTY, 
    OP_SET_PROPERTY,
    OP_GET_SUPER,
    OP_CALL,
    OP_CLOSURE,
    OP_CLASS,
    OP_METHOD,
    /* Two operands. */
    OP_JUMP,
    OP_JUMP_FALSE,
    OP_LOOP,
    OP_INVOKE,
    OP_SUPER_INVOKE,
} OpCode;

/**
 * Structure representing a dynamic array of bytecode instructions.
 * 
 * `count` is the current number of instructions in the vector.
 * `capacity` is the size of the vector.
 * `code` is an array of instructions.
 * `lines` is an array of line positions for each instruction
 * `constants` is the contant values in the chunk's scope.
 */
typedef struct
{
    int         count;
    int         capacity;
    uint8_t*    code;
    int*        lines;
    ValueArray  constants;
} Chunk;

/** Allocates and initializes a chunk specified by `chunk`. */
void init_chunk(Chunk* chunk);

/** Frees and resets the memory of a chunk specified by `chunk`. */
void free_chunk(Chunk* chunk);

/** 
 * Inserts an instruction specified by `byte` emitted from a position `line` in
 * the program to a chunk specified by `chunk`.
 */
void write_chunk(Chunk* chunk, uint8_t byte, int line);

/**
 * Inserts a value specified by `value` to the constant table of a chunk
 * specified by `chunk`.
 * 
 * Returns the index of the newly provided constant. 
 */
int add_constant(Chunk* chunk, Value value);

#endif