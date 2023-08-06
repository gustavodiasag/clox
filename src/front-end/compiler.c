#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "back-end/object.h"
#include "back-end/vm.h"
#include "common.h"
#include "front-end/compiler.h"
#include "front-end/scanner.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

parser_t parser;
compiler_t *current = NULL;

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
    [TOKEN_AND]             = {NULL, and_, PREC_AND},
    [TOKEN_CLASS]           = {NULL, NULL, PREC_NONE},
    [TOKEN_ELSE]            = {NULL, NULL, PREC_NONE},
    [TOKEN_FALSE]           = {literal, NULL, PREC_NONE},
    [TOKEN_FOR]             = {NULL, NULL, PREC_NONE},
    [TOKEN_FUN]             = {NULL, NULL, PREC_NONE},
    [TOKEN_IF]              = {NULL, NULL, PREC_NONE},
    [TOKEN_NIL]             = {literal, NULL, PREC_NONE},
    [TOKEN_OR]              = {NULL, or_, PREC_OR},
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
    return &current->func->chunk;
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
/// @param type token type
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

/// @brief Emits an instruction to jump back an amount of operations.
/// @param loop_start position before the loop condition
static void emit_loop(int loop_start)
{
    emit_byte(OP_LOOP);

    int offset = current_chunk()->count - loop_start + 2;

    if (offset > UINT16_MAX)
        error("Loop body too large.");

    emit_byte((offset >> 8) & 0xff);
    emit_byte(offset & 0xff);
}

/// @brief Emits the jump instruction and a placeholder for the offset.
/// @param instruction jump instruction
/// @return offset of the emitted instruction in the chunk
static int emit_jump(uint8_t instruction) {
    emit_byte(instruction);
    // Two bytes are used for the jump offset operand
    emit_bytes(0xff, 0xff);

    return current_chunk()->count - 2;
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

/// @brief Replaces the operand at the given location with the jump offset. 
/// @param offset jump offset
static void patch_jump(int offset)
{
    int jump = current_chunk()->count - offset - 2;

    if (jump > UINT16_MAX)
        error("Too much code to jump over.");

    current_chunk()->code[offset] = (jump >> 8) & 0xff;
    current_chunk()->code[offset + 1] = jump & 0xff;
}

/// @brief Initializes the compiler with the global scope.
/// @param compiler current compiler
/// @param type top-level scope
static void init_compiler(compiler_t *compiler, func_type_t type)
{
    compiler->func = NULL;
    compiler->type = type;
    compiler->local_count = 0;
    compiler->scope_depth = 0;
    compiler->func = new_func();
    current = compiler;

    local_t *local = &current->locals[current->local_count++];
    local->depth = 0;
    local->name.start = "";
    local->name.length = 0;
}

static obj_func_t *end_compiler()
{
    emit_return();
    obj_func_t *func = current->func;
#ifdef DEBUG_PRINT_CODE
    if (!parser.had_error)
        disassemble_chunk(current_chunk(), (func->name)
            ? func->name->chars : "<script>");
#endif
    return func;
}

/// @brief Creates a new level of depth for the next scope.
static void begin_scope()
{
    current->scope_depth++;
}

/// @brief Decreases the level of depth after a block is closed.
static void end_scope()
{
    current->scope_depth--;

    // Discards any variable declared at the scope depth that has just ended.
    while (current->local_count > 0
        && current->locals[current->local_count - 1].depth > current->scope_depth) {
        // Local variables occupy slots in the virtual machine's stack, so 
        // when going out of scope, the correspondent slot should be freed.
        emit_byte(OP_POP);
        current->local_count--;
    }
}

obj_func_t *compile(const char *source)
{
    init_scanner(source);

    compiler_t compiler;
    init_compiler(&compiler, TYPE_SCRIPT);

    parser.had_error = false;
    parser.panic = false;

    advance();
    
    while (!match(TOKEN_EOF))
        declaration();

    obj_func_t *func = end_compiler();
    
    return (parser.had_error) ? NULL : func;
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
    // Since assignment is the lowest precedence expression, the only time
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

/// @brief Marks that the latest declared variable contains a value.
static void mark_initialized()
{
    current->locals[current->local_count - 1].depth = current->scope_depth;
}

/// @brief Outputs the bytecode for some new variable.
/// @param var index of the variable string constant
static void define_var(uint8_t var)
{
    if (current->scope_depth > 0) {
        mark_initialized();
        return;
    }

    emit_bytes(OP_GLOBAL, var);
}

/// @brief Parses an and operation using short-circuit.
/// @param can_assign whether to consider assignment or not
static void and_(bool can_assign)
{
    // At this point, the left-hand side of the expression is sitting at
    // the top of the stack,  
    int end_jump = emit_jump(OP_JUMP_FALSE);

    // The pop is reached when the left-hand side evaluates to true.
    emit_byte(OP_POP);
    // If the left operand is discarted, the right one becomes
    // the result of the whole expression.
    parse_precedence(PREC_AND);

    patch_jump(end_jump);
}

/// @brief Parses an or operation using short-circuit.
/// @param can_assign whether to consider assignment or not
static void or_(bool can_assign)
{
    int else_jump = emit_jump(OP_JUMP_FALSE);
    int end_jump = emit_jump(OP_JUMP);

    // If the left-hand side evaluates to false, it jumps over the
    // opcode that short-circuits the expression to evaluate the
    // right operand.
    patch_jump(else_jump);
    emit_byte(OP_POP);

    parse_precedence(PREC_OR);
    patch_jump(end_jump);
}

/// @brief Adds the given token's lexeme to the chunk's constant table as a string.
/// @param name variable name
/// @return position of the string in the constant table 
static uint8_t identifier_const(token_t *name)
{
    return make_constant(OBJ_VAL(copy_str(name->start, name->length)));
}

/// @brief Checks if two identifiers have the same name.
/// @param a first variable
/// @param b second variable
/// @return whether the variables are the same
static bool identifier_equal(token_t *a, token_t *b)
{
    if (a->length != b->length)
        return false;

    return memcmp(a->start, b->start, a->length) == 0;
}

/// @brief Searches for the given variable in the compiler's local array.
/// @param compiler 
/// @param name variable name
/// @return index of the given local variable
static int resolve_local(compiler_t *compiler, token_t *name)
{
    // The locals array is traversed backwards so that the last
    // declared variable with the expected identifier is found,
    // ensuring that inner variables correcly shadow the ones
    // with the same name in surrounding scopes.
    for (int i = compiler->local_count - 1; i >= 0; i--) {
        local_t *local = &compiler->locals[i];

        if (identifier_equal(name, &local->name)) {
            if (local->depth == -1)
                error("Can't read variable in its own initializer");

            return i;
        }

    }
    // Flag representing that the given variable is not a local. 
    return -1;
}

/// @brief Initializes a local variable in the compiler's array of variables.
/// @param name variable name
static void add_local(token_t name)
{
    if (current->local_count == UINT8_COUNT) {
        error("Too many variables in scope.");
        return;
    }
    
    local_t *local = &current->locals[current->local_count++];
    local->name = name;
    local->depth = -1;
}

/// @brief Records the existence of a local variable.
static void declare_var()
{
    if (current->scope_depth == 0)
        return;

    token_t *name = &parser.previous;

    // Checks for an existing variable with the same name at the
    // same scope level.
    for (int i = current->local_count - 1; i >=0; i--) {
        local_t *local = &current->locals[i];

        if (local->depth != -1 && local->depth < current->scope_depth)
            break;

        if (identifier_equal(name, &local->name))
            error("Already a variable with this name in this scope.");
    }

    add_local(*name);
}

/// @brief Parses the current token as a variable identifier.
/// @param error thrown if the expected token is not found
/// @return index of the variable in the constant table
static uint8_t parse_var(const char *error)
{
    consume(TOKEN_IDENTIFIER, error);

    declare_var();

    // At runtime, local variables aren't looked up by name.  
    if (current->scope_depth > 0)
        return 0;

    return identifier_const(&parser.previous);
}

/// @brief Parses an expression.
static void expression()
{
    parse_precedence(PREC_ASSIGN);
}

/// @brief Parses declarations and statements nested in a block.
static void block()
{
    while (parser.current.type != TOKEN_RIGHT_BRACE && parser.current.type != TOKEN_EOF)
        declaration();

    consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
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

/// @brief Parses an expression in a context where a statement is expected.
static void expr_stmt()
{
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
    emit_byte(OP_POP);
}

static void for_stmt()
{
    begin_scope();

    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");

    if (match(TOKEN_SEMICOLON)) {
        // No initializer.
    } else if (match(TOKEN_VAR)) {
        var_declaration();
    } else {
        expr_stmt();
    }

    int loop_start = current_chunk()->count;
    
    int exit_jump = -1;

    if (!match(TOKEN_SEMICOLON)) {
        expression();
        consume(TOKEN_SEMICOLON, "Expect ';' after loop condition.");
        // Jump out of the loop if the condition is false.
        exit_jump = emit_jump(OP_JUMP_FALSE);
        emit_byte(OP_POP);
    }

    if (!match(TOKEN_RIGHT_PAREN)) {
        int body_jump = emit_jump(OP_JUMP);
        int inc_start = current_chunk()->count;

        expression();
        emit_byte(OP_POP);
        consume(TOKEN_RIGHT_PAREN, "Expect ')' after for clauses.");

        emit_loop(loop_start);
        loop_start = inc_start;
        patch_jump(body_jump);
    }

    statement();
    emit_loop(loop_start);

    // Done only if there is a condition clause.
    if (exit_jump != -1) {
        patch_jump(exit_jump);
        emit_byte(OP_POP);
    }

    end_scope();
}

/// @brief Parses a conditional statement.
static void if_stmt() {
    consume(TOKEN_LEFT_PAREN, "Expect '(' after statement.");
    // At runtime, the condition value for the if statement
    // will be located at the top of the stack.
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

    // The instruction has an operand for how much to offset the
    // instruction pointer if the expression is false.
    int jump = emit_jump(OP_JUMP_FALSE);
    // The result of the conditional expression must be removed
    // from the stack after the statement is evaluated.
    emit_byte(OP_POP);
    statement();

    patch_jump(jump);

    int else_jump = emit_jump(OP_JUMP);

    if (match(TOKEN_ELSE))
        statement();
    
    patch_jump(else_jump);
}

/// @brief Parses a print statement.
static void print_stmt()
{
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after value");
    emit_byte(OP_PRINT);
}

/// @brief Parses a while statement.
static void while_stmt()
{
    // At this point, the position to where the loop could go
    // back to has already been compiled and sits just before
    // the loop expression.
    int loop_start = current_chunk()->count;

    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

    int exit_jump = emit_jump(OP_JUMP_FALSE);
    emit_byte(OP_POP);

    statement();
    // After executing the loop body, the instruction pointer must
    // jump all the way back to before the condition, that way the
    // expression is evaluated at each loop iteration.
    emit_loop(loop_start);

    patch_jump(exit_jump);
    emit_byte(OP_POP);
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
    if (match(TOKEN_PRINT)) {
        print_stmt();
    } else if (match(TOKEN_FOR)) {
        for_stmt();
    } else if (match(TOKEN_IF)) {
        if_stmt();
    } else if (match(TOKEN_WHILE)) {
        while_stmt();
    } else if (match(TOKEN_LEFT_BRACE)) {
        begin_scope();
        block();
        end_scope();
    } else {
        expr_stmt();
    }
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
    uint8_t get_op, set_op;
    int var = resolve_local(current, &name);

    if (var != -1) {
        get_op = OP_GET_LOCAL;
        set_op = OP_SET_LOCAL;
    } else {
        var = identifier_const(&name);
        get_op = OP_GET_GLOBAL;
        set_op = OP_SET_GLOBAL;
    }

    // If an attribution operator is found, instead of emmiting code for a
    // variable access, the assigned value is compiled and an assignment
    // instruction is generated instead.  
    if (can_assign && match(TOKEN_EQUAL)) {
        expression();
        emit_bytes(set_op, (uint8_t)var);
    } else {
        emit_bytes(get_op, (uint8_t)var);
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