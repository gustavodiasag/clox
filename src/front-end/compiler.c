#include <stdio.h>
#include <stdlib.h>

#include "back-end/object.h"
#include "back-end/vm.h"
#include "common.h"
#include "front-end/compiler.h"
#include "front-end/scanner.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

parser_t parser;

// Stores all the bytecode generated during compilation.
chunk_t *compiling_chunk;

// Specifies the functions to compile a prefix expression starting with
// the entry token, an infix expression whose left operand is followed
// by the entry token and the token's level of precedence.
parse_rule_t rules[] = {
    [TOKEN_LEFT_PAREN]      = {grouping, NULL, PREC_NONE},
    [TOKEN_RIGHT_PAREN]     = {NULL, NULL, PREC_NONE},
    [TOKEN_LEFT_BRACE]      = {NULL, NULL, PREC_NONE},
    [TOKEN_RIGHT_BRACE]     = {NULL, NULL, PREC_NONE},
    [TOKEN_COMMA]           = {NULL, NULL, PREC_NONE},
    [TOKEN_DOT]             = {NULL, NULL, PREC_NONE},
    [TOKEN_MINUS]           = {unary, binary, PREC_TERM},
    [TOKEN_PLUS]            = {NULL, binary, PREC_TERM},
    [TOKEN_SEMICOLON]       = {NULL, NULL, PREC_NONE},
    [TOKEN_SLASH]           = {NULL, binary, PREC_FACTOR},
    [TOKEN_STAR]            = {NULL, binary, PREC_FACTOR},
    [TOKEN_BANG]            = {unary, NULL, PREC_NONE},
    [TOKEN_BANG_EQUAL]      = {NULL, binary, PREC_EQUAL},
    [TOKEN_EQUAL]           = {NULL, NULL, PREC_NONE},
    [TOKEN_EQUAL_EQUAL]     = {NULL, binary, PREC_EQUAL},
    [TOKEN_GREATER]         = {NULL, binary, PREC_COMPARE},
    [TOKEN_GREATER_EQUAL]   = {NULL, binary, PREC_COMPARE},
    [TOKEN_LESS]            = {NULL, binary, PREC_COMPARE},
    [TOKEN_LESS_EQUAL]      = {NULL, binary, PREC_COMPARE},
    [TOKEN_IDENTIFIER]      = {variable, NULL, PREC_NONE},
    [TOKEN_STRING]          = {string, NULL, PREC_NONE},
    [TOKEN_NUMBER]          = {number, NULL, PREC_NONE},
    [TOKEN_AND]             = {NULL, NULL, PREC_NONE},
    [TOKEN_CLASS]           = {NULL, NULL, PREC_NONE},
    [TOKEN_ELSE]            = {NULL, NULL, PREC_NONE},
    [TOKEN_FALSE]           = {literal, NULL, PREC_NONE},
    [TOKEN_FOR]             = {NULL, NULL, PREC_NONE},
    [TOKEN_FUN]             = {NULL, NULL, PREC_NONE},
    [TOKEN_IF]              = {NULL, NULL, PREC_NONE},
    [TOKEN_NIL]             = {literal, NULL, PREC_NONE},
    [TOKEN_OR]              = {NULL, NULL, PREC_NONE},
    [TOKEN_PRINT]           = {NULL, NULL, PREC_NONE},
    [TOKEN_RETURN]          = {NULL, NULL, PREC_NONE},
    [TOKEN_SUPER]           = {NULL, NULL, PREC_NONE},
    [TOKEN_THIS]            = {NULL, NULL, PREC_NONE},
    [TOKEN_TRUE]            = {literal, NULL, PREC_NONE},
    [TOKEN_VAR]             = {NULL, NULL, PREC_NONE},
    [TOKEN_WHILE]           = {NULL, NULL, PREC_NONE},
    [TOKEN_ERROR]           = {NULL, NULL, PREC_NONE},
    [TOKEN_EOF]             = {NULL, NULL, PREC_NONE}
};

static chunk_t *current_chunk()
{
    return compiling_chunk;
}

/// @brief Outputs a syntax error occurred at the specified token.
/// @param token
/// @param message description
static void error_at(token_t *token, const char *message)
{
    if (parser.panic)
        return;
        
    parser.panic = true;

    fprintf(stderr, "[line %d] Error", token->line);

    if (token->type == TOKEN_EOF)
        fprintf(stderr, " at end");
    // else if (token->type == TOKEN_ERROR)
        // Nothing.
    else
        fprintf(stderr, " at '%.*s'", token->length, token->start);

    fprintf(stderr, ": %s\n", message);

    parser.had_error = true;
}

static void error(const char *message)
{
    error_at(&parser.previous, message);
}

/// @brief Reports an error at the token that has just been consumed.
/// @param message description
static void error_at_current(const char *message)
{
    error_at(&parser.previous, message);
}

/// @brief Asks for a new token, considering the previous one has been consumed.
static void advance()
{
    parser.previous = parser.current;

    while (true) {
        parser.current = scan_token();
        
        if (parser.current.type != TOKEN_ERROR)
            break;

        error_at_current(parser.current.start);
    }
}

/// @brief Expects the specified token to be consumed, advancing the parser.
/// @param type 
/// @param message thrown as an error description if the token is not found
static void consume(token_type_t type, const char *message)
{
    if (parser.current.type == type) {
        advance();
        return;
    }

    error_at_current(message);
}

/// @brief Consumes the current token if it has the given type.
/// @param type 
/// @return whether the token was matched or not
static bool match(token_type_t type)
{
    if (parser.current.type != type)
        return false;

    advance();

    return true;
}

/// @brief Writes the specified operational code to the compiling chunk.
/// @param byte operational code
static void emit_byte(uint8_t byte)
{
    write_chunk(current_chunk(), byte, parser.previous.line);
}

/// @brief Deals with operations that are actually two bytes long.
/// @param byte_1 
/// @param byte_2 
static void emit_bytes(uint8_t byte_1, uint8_t byte_2)
{
    emit_byte(byte_1);
    emit_byte(byte_2);
}

static void emit_return()
{
    emit_byte(OP_RETURN);
}

/// @brief Adds an entry for the value to the chunk's constant table.
/// @param value
/// @return constant's position at the constant table
static uint8_t make_constant(value_t value)
{
    int constant = add_constant(current_chunk(), value);

    if (constant > UINT8_MAX) {
        error("Too many constants in one chunk");
        return 0;
    }

    return (uint8_t)constant;
}

/// @brief Emits the operational code related to a constant value.
/// @param value 
static void emit_constant(value_t value)
{
    emit_bytes(OP_CONSTANT, make_constant(value));
}

static void end_compiler()
{
    emit_return();
#ifdef DEBUG_PRINT_CODE
    if (!parser.had_error)
        disassemble_chunk(current_chunk());
#endif
}

bool compile(const char *source, chunk_t *chunk)
{
    init_scanner(source);

    compiling_chunk = chunk;
    parser.had_error = false;
    parser.panic = false;

    advance();
    
    while (!match(TOKEN_EOF)) {
        declaration();
    }

    consume(TOKEN_EOF, "Expect end of expression");

    end_compiler();

    return !parser.had_error;
}

/// @brief Parses a token considering the level of precedence specified.
/// @param precedence 
static void parse_precedence(precedence_t precedence)
{
    advance();

    parse_fn_t prefix_rule = rules[parser.previous.type].prefix;

    if (prefix_rule == NULL) {
        error("Expect expression.");
        return;
    }
    // Since assignment is the lowest-precedence expression, the only time
    // an assignment is allowed is when parsing an assignment expresion or
    // top-level expression like in an expresion statement.
    bool can_assign = (precedence <= PREC_ASSIGN);

    // Compiles the rest of the prefix expression, consuming the necessary tokens.
    prefix_rule(can_assign);

    while (precedence <= rules[parser.current.type].precedence) {
        advance();

        parse_fn_t infix_rule = rules[parser.previous.type].infix;
        infix_rule(can_assign);
    }

    // If the `=` doesn't get consumed as part of the expression, nothing else
    // is going to consume it.   
    if (can_assign && match(TOKEN_EQUAL))
        error("Invalid assignment target.");
}

/// @brief Outputs the bytecode for some new global variable.
/// @param var index of the variable string constant
static void define_var(uint8_t var)
{
    emit_bytes(OP_GLOBAL, var);
}

/// @brief Adds the given token's lexeme to the chunk's constant table as a string.
/// @param name variable name
/// @return position of the string in the constant table 
static uint8_t identifier_const(token_t *name)
{
    return make_constant(OBJ_VAL(copy_str(name->start, name->length)));
}

/// @brief Parses the current token as a variable identifier.
/// @param error thrown if the expected token is not found
/// @return index of the variable in the constant table
static uint8_t parse_var(const char *error)
{
    consume(TOKEN_IDENTIFIER, error);

    return identifier_const(&parser.previous);
}

/// @brief Parses an expression.
static void expression()
{
    parse_precedence(PREC_ASSIGN);
}

/// @brief Parses a variable declaration, together with its initializing expression.
static void var_declaration()
{
    uint8_t var = parse_var("Expect a variable name.");

    if (match(TOKEN_EQUAL)) {
        expression();
    } else {
        emit_byte(OP_NIL);
    }

    consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration.");

    define_var(var);
}

/// @brief Compiles an expression in a context where a statement is expected.
static void expr_stmt()
{
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
    emit_byte(OP_POP);
}

/// @brief Compiles a print statement.
static void print_stmt()
{
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after value");
    emit_byte(OP_PRINT);
}

/// @brief Minimizes the number of cascated compile errors reported.
static void syncronize() {
    parser.panic = false;

    while (parser.current.type != TOKEN_EOF) {
        // Statement boudaries are used as the point of syncronization.
        // A boundary is detected either by a token that ends or begins
        // a statement.
        if (parser.previous.type == TOKEN_SEMICOLON)
            return;

        switch (parser.current.type) {
            case TOKEN_CLASS:
            case TOKEN_FUN:
            case TOKEN_VAR:
            case TOKEN_FOR:
            case TOKEN_IF:
            case TOKEN_WHILE:
            case TOKEN_PRINT:
            case TOKEN_RETURN:
                return;

            default:
                ; // Do nothing.
        }

        advance();
    }
}

/// @brief Parses a single declaration.
static void declaration()
{
    if (match(TOKEN_VAR)) {
        var_declaration();
    } else {
        statement();
    }

    // If a compile error is detected while parsing the
    // previous statement, panic mode is entered.
    if (parser.panic)
        syncronize();
}

/// @brief Parses a single statement.
static void statement()
{
    if (match(TOKEN_PRINT))
        print_stmt();
    else
        expr_stmt();
}

/// @brief Converts the lexeme to a double value.
/// @param can_assign whether to consider assigment or not
static void number(bool can_assign)
{
    double value = strtod(parser.previous.start, NULL);

    emit_constant(NUM_VAL(value));
}

/// @brief Builds a string object directly from the parsed token's lexeme.
/// @param can_assign whether to consider assigment or not
static void string(bool can_assign)
{
    emit_constant(OBJ_VAL(copy_str(parser.previous.start + 1, parser.previous.length - 2)));
}

/// @brief Generates the instruction to load a variable with that name.
/// @param name variable name
/// @param can_assign whether to consider assigment or not
static void named_variable(token_t name, bool can_assign)
{
    uint8_t var = identifier_const(&name);

    // If an attribution operator is found, instead of emmiting code for a
    // variable access, the assigned value is compiled and an assignment
    // instruction is generated instead.  
    if (can_assign && match(TOKEN_EQUAL)) {
        expression();
        emit_bytes(OP_SET_GLOBAL, var);
    } else {
        emit_bytes(OP_GET_GLOBAL, var);
    }
}

/// @brief Parses a variable.
/// @param can_assign whether to consider assigment or not
static void variable(bool can_assign)
{
    named_variable(parser.previous, can_assign);
}

/// @brief Parses an expression between parenthesis, consuming the closing one.
static void grouping(bool can_assign)
{
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

/// @brief Parses an infix expression.
/// @param can_assign whether to consider assigment or not
static void binary(bool can_assign)
{
    token_type_t operator_type = parser.previous.type;
    parse_rule_t *rule = &rules[operator_type];
    
    // A higher precedence level is used because binary operators of the same
    // type are left-associative, so if parse_precedence finds further tokens
    // with the same level of precedence, it does not prioritizes them.
    parse_precedence((precedence_t)(rule->precedence + 1));

    switch (operator_type) {
        case TOKEN_BANG_EQUAL:
            emit_bytes(OP_EQUAL, OP_NOT);
            break;
        case TOKEN_EQUAL_EQUAL:
            emit_byte(OP_EQUAL);
            break;
        case TOKEN_GREATER:
            emit_byte(OP_GREATER);
            break;
        case TOKEN_GREATER_EQUAL:
            emit_bytes(OP_LESS, OP_NOT);
            break;
        case TOKEN_LESS:
            emit_byte(OP_LESS);
            break;
        case TOKEN_LESS_EQUAL:
            emit_bytes(OP_GREATER, OP_NOT);
            break;
        case TOKEN_PLUS:
            emit_byte(OP_ADD);
            break;
        case TOKEN_MINUS:
            emit_byte(OP_SUBTRACT);
            break;
        case TOKEN_STAR:
            emit_byte(OP_MULTIPLY);
            break;
        case TOKEN_SLASH:
            emit_byte(OP_DIVIDE);
            break;
        default:
            return;
    }
}

/// @brief Parses a prefix expression.
/// @param can_assign whether to consider assigment or not
static void unary(bool can_assign)
{
    token_type_t operator_type = parser.previous.type;

    parse_precedence(PREC_UNARY); // Parse the operand.

    switch (operator_type) { // Emit the operator instruction.
        case TOKEN_BANG:
            emit_byte(OP_NOT);
            break;
        case TOKEN_MINUS:
            emit_byte(OP_NEGATE);
            break;
        default:
            return;
    }
}

/// @brief Parses a literal expression.
/// @param can_assign whether to consider assigment or not
static void literal(bool can_assign)
{
    switch (parser.previous.type) {
        case TOKEN_FALSE:
            emit_byte(OP_FALSE);
            break;
        case TOKEN_TRUE:
            emit_byte(OP_TRUE);
            break;
        case TOKEN_NIL:
            emit_byte(OP_NIL);
            break;
        default:
            return;
    }
}