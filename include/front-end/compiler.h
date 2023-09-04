#pragma once

#include "back-end/object.h"
#include "scanner.h"

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
    // Tells if the local is captured by a closure.
    bool is_captured;
} local_t;

typedef struct {
    // Stores which local slot the upvalue is capturing.
    uint8_t index;
    // TODO: description.
    bool is_local;
} upvalue_t;

// Used by the compiler to detect whether the code being
// processed corresponds to the top-level program or the
// body of a function.
typedef enum {
    TYPE_FUNC,
    TYPE_SCRIPT
} func_type_t;

typedef struct compiler_t {
    // Linked-list used to provide access to the surrounding
    // compiler and its bytecode chunk.
    struct compiler_t* enclosing;
    obj_func_t* func;
    func_type_t type;
    // Locals that are in scope at each point in the
    // compilation process.
    local_t locals[UINT8_COUNT];
    // Tracks how many locals are in scope.
    int local_count;
    // Upvalues looked-up by the function being parsed.
    upvalue_t upvalues[UINT8_COUNT];
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

obj_func_t* compile(const char* source);
void mark_compiler_roots();
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
static void and_(bool can_assign);
static void or_(bool can_assign);
static void call(bool can_assign);
static void dot(bool can_assign);