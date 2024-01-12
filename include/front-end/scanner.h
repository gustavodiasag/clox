#ifndef SCANNER_H
#define SCANNER_H

#include "token.h"

/**
 * Initializes a scanner pointing at the beginning of a character stream
 * specified by `source`.
 */
void init_scanner(const char* source);

/** Returns the next token that the scanner has found. */
Token scan_token();

#endif