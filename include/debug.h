#pragma once

#include "back-end/chunk.h"

void disassemble_chunk(chunk_t *chunk);
int disassemble_instruction(chunk_t *chunk, int offset);