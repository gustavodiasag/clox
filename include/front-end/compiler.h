#pragma once

#include "back-end/object.h"
#include "scanner.h"

#define UINT8_COUNT (UINT8_MAX + 1)

typedef void (*parse_fn_t)(bool can_assign);

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

typedef struct {
    // Variable name.
    token_t name;
    // Records the scope depth of the block where
    // the local variable was declared.
    int depth;
} local_t;

typedef struct {
    // Locals that are in scope during each point in the
    // compilation process.
    local_t locals[UINT8_COUNT];
    // Tracks how many locals are in scope. 
    int local_count;
    // Number of blocks surrounding the code being compiled.
    int scope_depth;
} compiler_t;

typedef struct {
    token_t current;
    token_t previous;
    // Records whether any errors ocurred during compilation.
    bool had_error;
    // Used to enter panic mode, responsible for suppressing
    // cascated compile-time error reporting.
    bool panic;
} parser_t;

typedef struct {
    parse_fn_t prefix;
    parse_fn_t infix;
    precedence_t precedence;
} parse_rule_t;


bool compile(const char *source, chunk_t *chunk);
static void expression();
static void statement();
static void declaration();
static void grouping(bool can_assign);
static void binary(bool can_assign);
static void unary(bool can_assign);
static void number(bool can_assign);
static void literal(bool can_assign);
static void string(bool can_assign);
static void variable(bool can_assign);