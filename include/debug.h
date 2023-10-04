#ifndef DEBUG_H
#define DEBUG_H

#include "back-end/chunk.h"

void disassemble_chunk(chunk_t* chunk, const char* name);
int disassemble_instruction(chunk_t* chunk, int offset);

#endif