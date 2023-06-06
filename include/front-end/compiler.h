#pragma once

#include "scanner.h"

typedef void (*parse_fn_t)();

typedef struct {
    token_t current;
    token_t previous;
    bool had_error;
    bool panic;
} parser_t;

typedef struct {
    parse_fn_t prefix;
    parse_fn_t infix;
    precedence_t precedence;
} parse_rule_t;

typedef enum {
    PREC_NONE,
    PREC_ASSIGN,
    PREC_OR,
    PREC_AND,
    PREC_EQUAL,
    PREC_COMPARE,
    PREC_TERM,
    PREC_FACTOR,
    PREC_UNARY,
    PREC_CALL,
    PREC_PRIMARY
} precedence_t;


bool compile(const char *source, chunk_t *chunk);