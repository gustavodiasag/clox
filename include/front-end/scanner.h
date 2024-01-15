#ifndef SCANNER_H
#define SCANNER_H

#include "token.h"

/**
 * Initializes a scanner pointing at the beginning of a character stream
 * specified by `source`.
 */
void init_scanner(const char* source);

/**
 * Sequentially looks for a token from the stream.
 * 
 * Returns the token found or a special one that stores an error message as its
 * lexeme.
 */
Token scan_token();

#endif