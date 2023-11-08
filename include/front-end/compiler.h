#ifndef COMPILER_H
#define COMPILER_H

#include "back-end/object.h"
#include "scanner.h"

typedef void (*ParseFun)(bool can_assign);

typedef enum
{
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
} Precedence;

typedef struct
{
    // Variable name.
    Token name;
    // Records the scope depth of the block where
    // the local variable was declared.
    int depth;
    // Tells if the local is captured by a closure.
    bool is_captured;
} Local;

typedef struct
{
    // Stores which local slot the upvalue is capturing.
    uint8_t index;
    // TODO: description.
    bool is_local;
} UpValue;

// Used by the compiler to detect whether the code being
// processed corresponds to the top-level program or the
// body of a function.
typedef enum
{
    TYPE_FUNC,
    TYPE_INIT,
    TYPE_METHOD,
    TYPE_SCRIPT,
} FunType;

typedef struct
{
    Token current;
    Token previous;
    // Records whether any errors ocurred during compilation.
    bool had_error;
    // Used to enter panic mode, responsible for suppressing
    // cascated compile-time error reporting.
    bool panic;
} Parser;

typedef struct
{
    ParseFun prefix;
    ParseFun infix;
    Precedence precedence;
} ParseRule;

// TODO: Description.
ObjFun* compile(const char* source);

/// @brief Marks objects allocated on the heap by the compiler.
void mark_compiler_roots();

#endif