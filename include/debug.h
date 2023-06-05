#pragma once

#include "backend/chunk.h"

void disassemble_chunk(chunk_t *chunk, const char *name);
int disassemble_instruction(chunk_t *chunk, int offset);