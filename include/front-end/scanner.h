#ifndef SCANNER_H
#define SCANNER_H

#include "token.h"

typedef struct {
    // Beginning of the current lexeme.
    const char* start;
    // Points to the current character being looked at.
    const char* current;
    // Used for error reporting.
    int line;
} scanner_t;

// TODO: Description.
void init_scanner(const char* source);

// TODO: Description.
token_t scan_token();

#endif