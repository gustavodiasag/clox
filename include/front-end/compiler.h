#pragma once

#include "scanner.h"

typedef struct {
    token_t current;
    token_t previous;
    bool had_error;
    bool panic;
} parser_t;

bool compile(const char *source, chunk_t *chunk);
void advance();
void consume(token_type_t type, const char *message);
void emit_byte(uint8_t byte);
void emit_bytes(uint8_t byte_1, uint8_t byte_2);
void error_at_current(const char *message);
void error_at(token_t *token, const char *message);
void end_compiler();
void emit_return(); 
chunk_t *current_chunk();