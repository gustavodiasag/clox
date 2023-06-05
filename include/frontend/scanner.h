#pragma once

#include "token.h"

typedef struct {
    const char *start; // Beginning of the current lexeme.
    const char *current;
    int line; // Useful for error reporting.
} scanner_t;

void init_scanner(const char *source);
bool is_alpha(char c);
bool is_at_end();
bool is_digit(char c);
bool match(char c);
char advance();
char peek();
char peek_next();
void skip_whitespace();
token_t scan_token();
token_t make_token();
token_t error_token();
token_t string();
token_t number();
token_t identifier();
token_type_t identifier_type();
token_type_t check_keyword(int start, int length, const char *rest, token_type_t type);