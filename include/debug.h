#ifndef DEBUG_H
#define DEBUG_H

#include "back-end/chunk.h"

/// @brief Prints all the instructions in a chunk.
/// @param chunk stores the bytecode to be analysed
/// @param name function's name
void disassemble_chunk(chunk_t* chunk, const char* name);

/// @brief Disassembles the instruction at the given offset.
/// @param chunk contains the instruction to be checked
/// @param offset used to access a specific instruction
/// @return the offset of the next instruction
int disassemble_instruction(chunk_t* chunk, int offset);

#endif