#pragma once

#include "token.h"

typedef struct {
    const char *start; // Beginning of the current lexeme.
    const char *current;
    int line; // Useful for error reporting.
} scanner_t;

void init_scanner(const char *source);
token_t scan_token();