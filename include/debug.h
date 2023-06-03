#pragma once

#include "chunk.h"

void disassemble_chunk(Chunk *chunk, const char *name);
int disassemble_instruction(Chunk *chunk, int offset);
static int simple_instruction(const char *name, int offset);
static int constant_instruction(const char *name, Chunk *chunk, int offset);