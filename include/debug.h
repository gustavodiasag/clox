#ifndef clox_debug_h
#define clox_debug_h

#include "chunk.h"

void disassemble_chunk(Chunk *chunk, const char *name); // Prints all the instructions in a chunk.
int disassemble_instruction(Chunk *chunk, int offset); // Disassembles only a single instruction.
static int simple_instruction(const char *name, int offset);
static int constant_instruction(const char *name, Chunk *chunk, int offset);

#endif