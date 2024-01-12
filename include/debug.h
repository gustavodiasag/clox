#ifndef DEBUG_H
#define DEBUG_H

#include "back-end/chunk.h"

/**
 * Prints all the opcodes from a chunk specified by `chunk`, which is contained
 * by a function whose name is specified by `name`.
 */
void disassemble_chunk(Chunk* chunk, const char* name);

/**
 * Prints a single opcode and its respective operands from a chunk specified by
 * `chunk`, present at an offset specified by `offset`.
 * 
 * Returns the offset of the next instruction.
 */
int disassemble_instruction(Chunk* chunk, int offset);

#endif