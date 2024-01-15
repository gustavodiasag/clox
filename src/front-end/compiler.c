#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "back-end/garbage_collector.h"
#include "back-end/object.h"
#include "back-end/vm.h"
#include "common.h"
#include "front-end/compiler.h"
#include "front-end/scanner.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

/**
 * Structure of a local variable in the language.
 * 
 * `name` is the token containing the variable's lexeme.
 * `depth` is the scope index of the block where the variable was declared.
 * `is_captured` is whether the variable is captured by a closure.
 */
typedef struct
{
    Token   name;
    int     depth;
    bool    is_captured;
} Local;

/**
 * Structure of an upvalue from a closure in the language.
 * 
 * `index` is the constant table position of the variable being captured.
 * `is_local` is whether the upvalue references a local or enclosing variable.
 */
typedef struct
{
    uint8_t index;
    bool    is_local;
} UpValue;

/**
 * Different types of functions that can be found during compilation.
 * 
 * `TYPE_FUNC` is an arbitrary function.
 * `TYPE_INIT` is a function serving as a constructor for a class.
 * `TYPE_METHOD` is a function declared inside the body of a class.
 * `TYPE_SCRIPT` is the top-level program.
 */
typedef enum
{
    TYPE_FUNC,
    TYPE_INIT,
    TYPE_METHOD,
    TYPE_SCRIPT,
} FunType;

/**
 * Represents the required information for parsing, which takes a predictive
 * approach.
 * 
 * `current` is the most recent token generated from the scanner.
 * `previous` is the previous token retrieved.
 * `had_error` is whether some compilation error occurred.
 * `panic` is whether compilation should handle errors in panic mode.
 */
typedef struct
{
    Token   current;
    Token   previous;
    bool    had_error;
    bool    panic;
} Parser;

/**
 * Structure for a compiler in the context of functions. Every one of those
 * defined in a program require its own compiler, helping organizing the
 * bytecode generated in different sections.
 * 
 * `enclosing` is the compiler for the surrounding environment.
 * `fun` is the function being compiled.
 * `type` is the type of `fun`.
 * `locals` is the list of variables in scope during the current compilation.
 * `local_count` is the number of locals.
 * `upvalues` is a list of variables captured by the function if it is used as
 *            a closure.
 * `scope_depth` is the number of blocks surrounding the code being compiled.
 */
typedef struct _Compiler
{
    struct _Compiler*   enclosing;
    ObjFun*             fun;
    FunType             type;
    Local               locals[UINT8_COUNT];
    int                 local_count;
    UpValue             upvalues[UINT8_COUNT];
    int                 scope_depth;
} Compiler;

/** FIXME: Improve description
 * Linked-list from the compiling class to its enclosing ones.
 * 
 * `has_superclass` is whether the surrounding class is a subclass or not.
 */
typedef struct _ClassCompiler
{
    struct _ClassCompiler*  enclosing;
    bool                    has_superclass;
} ClassCompiler;

/**
 * Levels of precedence used for parsing all the expressions in the language.
 * Their order is directly defined by the enum's positions, where the latter
 * ones correspond to the higher levels.
 */
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

/** Structure for all the parsing functions. */
typedef void (*ParseFun)(bool);

/**
 * Structure for the rules to be applied when a token is being parsed.
 * 
 * `prefix` is a parsing function to compile a prefix expression starting with
 *          a specific token.
 * `infix` is a parsing function to compile an infix expression whose left
 *         operand is followed by a specific token.
 * `precedence` is the precedence level of a specific token.
 */
typedef struct
{
    ParseFun    prefix;
    ParseFun    infix;
    Precedence  precedence;
} ParseRule;

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

static void and(bool can_assign);

static void or(bool can_assign);

static void call(bool can_assign);

static void dot(bool can_assign);

static void this(bool can_assign);

static void super(bool can_assign);

/* The parser is only needed during the compilation process. */
Parser parser;

/* References the currently active compiler. */
Compiler* current = NULL;

/* References the current class being compiled, if any. */
ClassCompiler* current_class = NULL;

/* References the chunk currently receiving the bytecode being compiled. */
Chunk* compiling_chunk;

/* Parsing rules for every token of the language. */
static ParseRule rules[] = {
    [TOKEN_LEFT_PAREN]      = {grouping, call, PREC_CALL},
    [TOKEN_RIGHT_PAREN]     = {NULL, NULL, PREC_NONE},
    [TOKEN_LEFT_BRACE]      = {NULL, NULL, PREC_NONE},
    [TOKEN_RIGHT_BRACE]     = {NULL, NULL, PREC_NONE},
    [TOKEN_COMMA]           = {NULL, NULL, PREC_NONE},
    [TOKEN_DOT]             = {NULL, dot, PREC_CALL},
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
    [TOKEN_AND]             = {NULL, and, PREC_AND},
    [TOKEN_CLASS]           = {NULL, NULL, PREC_NONE},
    [TOKEN_ELSE]            = {NULL, NULL, PREC_NONE},
    [TOKEN_FALSE]           = {literal, NULL, PREC_NONE},
    [TOKEN_FOR]             = {NULL, NULL, PREC_NONE},
    [TOKEN_FUN]             = {NULL, NULL, PREC_NONE},
    [TOKEN_IF]              = {NULL, NULL, PREC_NONE},
    [TOKEN_NIL]             = {literal, NULL, PREC_NONE},
    [TOKEN_OR]              = {NULL, or, PREC_OR},
    [TOKEN_PRINT]           = {NULL, NULL, PREC_NONE},
    [TOKEN_RETURN]          = {NULL, NULL, PREC_NONE},
    [TOKEN_SUPER]           = {super, NULL, PREC_NONE},
    [TOKEN_THIS]            = {this, NULL, PREC_NONE},
    [TOKEN_TRUE]            = {literal, NULL, PREC_NONE},
    [TOKEN_VAR]             = {NULL, NULL, PREC_NONE},
    [TOKEN_WHILE]           = {NULL, NULL, PREC_NONE},
    [TOKEN_ERROR]           = {NULL, NULL, PREC_NONE},
    [TOKEN_EOF]             = {NULL, NULL, PREC_NONE},
};

static Chunk* current_chunk()
{
    return &current->fun->chunk;
}

static void error_at(Token* token, const char* message)
{
    if (parser.panic) {
        return;
    }
    parser.panic = true;
    fprintf(stderr, "[line %d] Error", token->line);

    if (token->type == TOKEN_EOF) {
        fprintf(stderr, " at end");
    } else {
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    }
    fprintf(stderr, ": %s\n", message);

    parser.had_error = true;
}

static void error(const char* message)
{
    error_at(&parser.previous, message);
}

static void error_at_current(const char* message)
{
    error_at(&parser.previous, message);
}

static void advance()
{
    parser.previous = parser.current;

    while (true) {
        parser.current = scan_token();
        if (parser.current.type != TOKEN_ERROR) {
            break;
        }
        error_at_current(parser.current.start);
    }
}

static void consume(TokenType type, const char* message)
{
    if (parser.current.type == type) {
        advance();
        return;
    }
    error_at_current(message);
}

static bool check(TokenType type)
{
    /* Does not consume the token. */
    return parser.current.type == type;
}

static bool match(TokenType type)
{
    if (parser.current.type != type) {
        return false;
    }
    /* If matched, the token is consumed. */
    advance();

    return true;
}

static void emit_byte(uint8_t byte)
{
    write_chunk(current_chunk(), byte, parser.previous.line);
}

static void emit_bytes(uint8_t byte_1, uint8_t byte_2)
{
    emit_byte(byte_1);
    emit_byte(byte_2);
}

static void emit_loop(int loop_start)
{
    emit_byte(OP_LOOP);

    int offset = current_chunk()->count - loop_start + 2;
    if (offset > UINT16_MAX) {
        error("Loop body too large.");
    }
    emit_byte((offset >> 8) & 0xff);
    emit_byte(offset & 0xff);
}

static int emit_jump(uint8_t instruction)
{
    emit_byte(instruction);
    /* Two bytes are used for the jump offset operand. */
    emit_bytes(0xff, 0xff);

    return current_chunk()->count - 2;
}

static void emit_return()
{   
    /*
     * Whenever the compiler emits the implicit return at the end of a
     * body, the function type is checked to decide whether to insert
     * the initializer-specific behavior.
     */
    if (current->type == TYPE_INIT) {
        emit_bytes(OP_GET_LOCAL, 0);
    } else {
        emit_byte(OP_NIL);
    }
    emit_byte(OP_RETURN);
}

static uint8_t make_constant(Value value)
{
    int constant = add_constant(current_chunk(), value);

    if (constant > UINT8_MAX) {
        error("Too many constants in one chunk");
        return 0;
    }
    return (uint8_t)constant;
}

static void emit_constant(Value value)
{
    emit_bytes(OP_CONSTANT, make_constant(value));
}

static void patch_jump(int offset)
{
    int jump = current_chunk()->count - offset - 2;

    if (jump > UINT16_MAX) {
        error("Too much code to jump over.");
    }
    current_chunk()->code[offset] = (jump >> 8) & 0xff;
    current_chunk()->code[offset + 1] = jump & 0xff;
}

static void init_compiler(Compiler* compiler, FunType type)
{
    compiler->enclosing = current;
    compiler->fun = NULL;
    compiler->type = type;
    compiler->local_count = 0;
    compiler->scope_depth = 0;
    compiler->fun = new_func();

    current = compiler;

    if (type != TYPE_SCRIPT) {
        current->fun->name = copy_str(parser.previous.start,
                                       parser.previous.length);
    }
    Local* local = &current->locals[current->local_count++];
    local->depth = 0;
    local->is_captured = false;

    if (type != TYPE_FUNC) {
        local->name.start = "this";
        local->name.length = 4;
    } else {
        local->name.start = "";
        local->name.length = 0;
    }
}

static ObjFun* end_compiler()
{
    emit_return();

    ObjFun* func = current->fun;
#ifdef DEBUG_PRINT_CODE
    if (!parser.had_error) {
        disassemble_chunk(current_chunk(),
            (func->name) ? func->name->chars : "<script>");
    }
#endif
    /*
     * The current compiler removes itself from the linked-list by restoring
     * the previous one.
     */
    current = current->enclosing;
    return func;
}

static void begin_scope()
{
    current->scope_depth++;
}

static void end_scope()
{
    current->scope_depth--;

    /* Variables declared at the scope that has just ended are discarded. */
    while (current->local_count > 0 &&
        current->locals[current->local_count - 1].depth > current->scope_depth)
    {
        /*
         * Locals occupy slots in the vm stack, so when going out of scope, the
         * correspondent slot should be freed. However, if a local has been
         * captured by a closure, it must be transferred to the heap.
         */
        if (current->locals[current->local_count - 1].is_captured) {
            emit_byte(OP_CLOSE_UPVALUE);
        } else {
            emit_byte(OP_POP);
        }
        current->local_count--;
    }
}

static void parse_precedence(Precedence precedence)
{
    advance();

    ParseFun prefix_rule = rules[parser.previous.type].prefix;
    if (prefix_rule == NULL) {
        error("Expect expression.");
        return;
    }
    /*
     * Since assignment is the lowest precedence expression, the only time an
     * assignment is allowed is when parsing an assignment expression or
     * top-level expression like in an expresion statement.
     */
    bool can_assign = (precedence <= PREC_ASSIGN);

    prefix_rule(can_assign);

    while (precedence <= rules[parser.current.type].precedence) {
        advance();

        ParseFun infix_rule = rules[parser.previous.type].infix;
        infix_rule(can_assign);
    }
    if (can_assign && match(TOKEN_EQUAL)) {
        error("Invalid assignment target.");
    }
}

static void mark_initialized()
{
    if (current->scope_depth == 0) {
        return;
    }
    current->locals[current->local_count - 1].depth = current->scope_depth;
}

static void define_var(uint8_t var)
{
    if (current->scope_depth > 0) {
        mark_initialized();
        return;
    }
    emit_bytes(OP_GLOBAL, var);
}

static uint8_t arg_list()
{
    uint8_t args = 0;
    if (!check(TOKEN_RIGHT_PAREN)) {
        do {
            /*
             * Each argument expression emits code that leaves its value on the
             * stack in preparation for the call.
             */
            expression();
            if (args == UINT8_MAX) {
                error("Number of arguments exceeded.");
            }
            args++;
        } while (match(TOKEN_COMMA));
    }
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after arguments.");

    return args;
}

static void and(bool can_assign)
{
    int end_jump = emit_jump(OP_JUMP_FALSE);

    emit_byte(OP_POP);

    parse_precedence(PREC_AND);
    patch_jump(end_jump);
}

static void or(bool can_assign)
{
    int else_jump = emit_jump(OP_JUMP_FALSE);
    int end_jump = emit_jump(OP_JUMP);

    /*
     * If the left-hand side evaluates to false, it jumps over the opcode that
     * short-circuits the expression to evaluate the right operand.
     */
    patch_jump(else_jump);
    emit_byte(OP_POP);

    parse_precedence(PREC_OR);
    patch_jump(end_jump);
}

static uint8_t identifier_const(Token* name)
{
    return make_constant(OBJ_VAL(copy_str(name->start, name->length)));
}

static bool identifier_equal(Token* a, Token* b)
{
    if (a->length != b->length) {
        return false;
    }
    return memcmp(a->start, b->start, a->length) == 0;
}

static int resolve_local(Compiler* compiler, Token* name)
{
    /*
     * The locals array is traversed backwards so the last declared variable
     * with the expected identifier is found, ensuring that inner variables
     * correcly shadow the ones with the same name in surrounding scopes.
     */
    for (int i = compiler->local_count - 1; i >= 0; i--) {
        Local* local = &compiler->locals[i];

        if (identifier_equal(name, &local->name)) {
            if (local->depth == -1) {
                error("Can't read variable in its own initializer");
            }
            return i;
        }
    }
    /* Flag representing that the given variable is not a local. */
    return -1;
}

static int add_upvalue(Compiler* compiler, uint8_t index, bool is_local)
{
    int upvalue_count = compiler->fun->upvalue_count;
    /*
     * A closure may reference a variable in a function multiple times. Before
     * adding a new upvalue, checking if the function already has an upvalue
     * that closes over that variable avoids unnecessary use of memory.
     */
    for (int i = 0; i < upvalue_count; i++) {
        UpValue* upvalue = &compiler->upvalues[i];

        if (upvalue->index == index && upvalue->is_local == is_local) {
            return i;
        }
    }
    if (upvalue_count == UINT8_COUNT) {
        error("Too many closure variables in function.");
        return 0;
    }
    compiler->upvalues[upvalue_count].is_local = is_local;
    compiler->upvalues[upvalue_count].index = index;

    return compiler->fun->upvalue_count++;
}

static int resolve_upvalue(Compiler* compiler, Token* name)
{
    /* Top-level function contains only global variables. */
    if (!compiler->enclosing) {
        return -1;
    }
    int local = resolve_local(compiler->enclosing, name);
    
    if (local != -1) {
        compiler->enclosing->locals[local].is_captured = true;

        return add_upvalue(compiler, (uint8_t)local, true);
    }
    int upvalue = resolve_upvalue(compiler->enclosing, name);
    
    if (upvalue != -1) {
        return add_upvalue(compiler, (uint8_t)upvalue, false);
    }
    return -1;
}

static void add_local(Token name)
{
    if (current->local_count == UINT8_COUNT) {
        error("Too many variables in scope.");
        return;
    }
    Local* local = &current->locals[current->local_count++];
    local->name = name;
    local->depth = -1;
    local->is_captured = false;
}

static void declare_var()
{
    if (current->scope_depth == 0) {
        return;
    }
    Token* name = &parser.previous;
    /* Duplicated declarations must be checked and reported. */
    for (int i = current->local_count - 1; i >= 0; i--) {
        Local* local = &current->locals[i];

        if (local->depth != -1 && local->depth < current->scope_depth) {
            break;
        }
        if (identifier_equal(name, &local->name)) {
            error("Already a variable with this name in this scope.");
        }
    }
    add_local(*name);
}

static uint8_t parse_var(const char* error)
{
    consume(TOKEN_IDENTIFIER, error);

    declare_var();

    if (current->scope_depth > 0) {
        return 0;
    }
    return identifier_const(&parser.previous);
}

static void named_variable(Token name, bool can_assign)
{
    uint8_t get_op, set_op;
    int var = resolve_local(current, &name);

    if (var != -1) {
        get_op = OP_GET_LOCAL;
        set_op = OP_SET_LOCAL;
    } else if ((var = resolve_upvalue(current, &name)) != -1) {
        get_op = OP_GET_UPVALUE;
        set_op = OP_SET_UPVALUE;
    } else {
        var = identifier_const(&name);
        get_op = OP_GET_GLOBAL;
        set_op = OP_SET_GLOBAL;
    }
    /*
     * If an attribution operator is found, instead of emmiting code for a
     * variable access, the assigned value is compiled and an assignment
     * instruction is generated instead.
     */
    if (can_assign && match(TOKEN_EQUAL)) {
        expression();
        emit_bytes(set_op, (uint8_t)var);
    } else {
        emit_bytes(get_op, (uint8_t)var);
    }
}

static void variable(bool can_assign)
{
    named_variable(parser.previous, can_assign);
}

static Token synth_token(const char* data)
{
    Token token;
    token.start = data;
    token.length = (int)strlen(data);
    
    return token;
}

static void super(bool can_assign)
{
    if (!current_class) {
        error("Can't use 'super' outside of a class.");
    } else if (!current_class->has_superclass) {
        error("Can't use 'super' in a class with no superclass.");
    }
    consume(TOKEN_DOT, "Expect '.' after 'super'.");
    consume(TOKEN_IDENTIFIER, "Expect superclass method name.");
    
    uint8_t name = identifier_const(&parser.previous);

    named_variable(synth_token("this"), false);

    if (match(TOKEN_LEFT_PAREN)) {
        uint8_t args = arg_list();
        named_variable(synth_token("super"), false);
        emit_bytes(OP_SUPER_INVOKE, name);
        emit_byte(args);
    } else {
        named_variable(synth_token("super"), false);
        emit_bytes(OP_GET_SUPER, name);
    }
}

static void this(bool can_assign)
{
    if (!current_class) {
        error("Can't use 'this' outside of a class.");
        return;
    }
    /*
     * When the parser function is called, the `this` token has just been
     * consumed and is stored as the previous token. No assignment can be done
     * to `this`, so `false` disallows it.
     */
    variable(false);
}

static void expression()
{
    parse_precedence(PREC_ASSIGN);
}

static void block()
{
    while (parser.current.type != TOKEN_RIGHT_BRACE &&
           parser.current.type != TOKEN_EOF)
    {
        declaration();
    }
    consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

static void function(FunType type)
{
    /*
     * To handle the compilation of nested functions, a separate compiler is
     * created for each of them. Initializing a function's compiler sets it to
     * be the current one, so when compiling the function's body, all of the
     * emitted bytecode is written to the chunk owned by the new compiler.
     */
    Compiler compiler;
    init_compiler(&compiler, type);
    begin_scope();

    consume(TOKEN_LEFT_PAREN, "Expect '(' after function name.");

    if (!check(TOKEN_RIGHT_PAREN)) {
        do {
            current->fun->arity++;
            if (current->fun->arity > UINT8_MAX) {
                error_at_current("Number of parameters exceeded.");
            }
            uint8_t constant = parse_var("Expect parameter name.");

            define_var(constant);
        } while (match(TOKEN_COMMA));
    }
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");
    consume(TOKEN_LEFT_BRACE, "Expect '{' before function body");
    block();

    ObjFun* function = end_compiler();

    emit_bytes(OP_CLOSURE, make_constant(OBJ_VAL(function)));

    for (int i = 0; i < function->upvalue_count; i++) {
        emit_byte(compiler.upvalues[i].is_local ? 1 : 0);
        emit_byte(compiler.upvalues[i].index);
    }
}

static void method()
{
    consume(TOKEN_IDENTIFIER, "Expect method name.");

    uint8_t constant = identifier_const(&parser.previous);
    FunType type = TYPE_METHOD;

    if (parser.previous.length == 4 &&
        !memcmp(parser.previous.start, "init", 4))
    {
        type = TYPE_INIT;
    }
    function(type);

    emit_bytes(OP_METHOD, constant);
}

static void class_declaration()
{
    consume(TOKEN_IDENTIFIER, "Expect class name.");
    Token class_name = parser.previous;

    uint8_t name = identifier_const(&parser.previous);
    declare_var();

    emit_bytes(OP_CLASS, name);
    define_var(name);
    
    ClassCompiler class_compiler;
    class_compiler.has_superclass = false;
    class_compiler.enclosing = current_class;
    current_class = &class_compiler;
    /* Check the existence of a possible inheritance. */
    if (match(TOKEN_LESS)) {
        consume(TOKEN_IDENTIFIER, "Expect superclass name.");

        variable(false);

        if (identifier_equal(&class_name, &parser.previous)) {
            error("A class can't inherit from itself.");
        }
        begin_scope();
        add_local(synth_token("super"));
        define_var(0);
        /* Loads the inheriting subclass onto the stack. */
        named_variable(class_name, false);
        emit_byte(OP_INHERIT);
        
        class_compiler.has_superclass = true;
    }
    /*
     * Provides a way to reference the class when parsing its methods so they
     * can be binded to the object.
     */
    named_variable(class_name, false);

    consume(TOKEN_LEFT_BRACE, "Expect '{' before class body.");

    while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
        /* Only method declarations are allowed in a class definition. */
        method();
    }
    consume(TOKEN_RIGHT_BRACE, "Expect '}' after class body.");
    /* Values in the stack that result from the declaration must be popped. */
    emit_byte(OP_POP);
    /* The scope opened for the superclass variable must be closed. */
    if (class_compiler.has_superclass) {
        end_scope();
    }
    // At the end of the class body, the enclosing one is restored.   
    current_class = current_class->enclosing;
}

static void fun_declaration()
{
    uint8_t global = parse_var("Expect function name.");
    mark_initialized();

    function(TYPE_FUNC);
    define_var(global);
}

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
        /* No initializer. */
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
        /* Jump out of the loop if the condition is false. */
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

    /* Done only if there is a condition clause. */
    if (exit_jump != -1) {
        patch_jump(exit_jump);
        emit_byte(OP_POP);
    }
    end_scope();
}

static void if_stmt()
{
    consume(TOKEN_LEFT_PAREN, "Expect '(' after statement.");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");
    /*
     * The instruction has an operand for how much to offset the instruction
     * pointer if the expression is false.
     */
    int jump = emit_jump(OP_JUMP_FALSE);
    /*
     * The result of the conditional expression must be removed from the stack
     * after the statement is evaluated.-
     */
    emit_byte(OP_POP);
    statement();
    patch_jump(jump);

    int else_jump = emit_jump(OP_JUMP);

    if (match(TOKEN_ELSE)) {
        statement();
    }
    patch_jump(else_jump);
}

static void print_stmt()
{
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after value.");
    emit_byte(OP_PRINT);
}

static void return_stmt()
{
    /* A `return` statement outside of any function is a compile-time error. */
    if (current->type == TYPE_SCRIPT) {
        error("Can't return from top-level code.");
    }
    /*
     * Returning a value is optional, so its presence has to be checked. If
     * there is no return value, the statement implicitly returns `nil`.
     */
    if (match(TOKEN_SEMICOLON)) {
        emit_return();
    } else {
        if (current->type == TYPE_INIT) {
            error("Can't return a value from an initializer.");
        }
        expression();
        consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
        emit_byte(OP_RETURN);
    }
}

static void while_stmt()
{
    int loop_start = current_chunk()->count;

    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

    int exit_jump = emit_jump(OP_JUMP_FALSE);
    emit_byte(OP_POP);

    statement();
    /*
     * After executing the loop body, the instruction pointer must jump to
     * before the condition, so the expression is evaluated at each iteration.
     */
    emit_loop(loop_start);

    patch_jump(exit_jump);
    emit_byte(OP_POP);
}

static void syncronize()
{
    parser.panic = false;

    while (parser.current.type != TOKEN_EOF) {
        /* Statement boudaries are used as the point of syncronization. */
        if (parser.previous.type == TOKEN_SEMICOLON) {
            return;
        }
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
            ; /* Do nothing. */
        }
        advance();
    }
}

static void declaration()
{
    if (match(TOKEN_CLASS)) {
        class_declaration();
    } else if (match(TOKEN_FUN)) {
        fun_declaration();
    } else if (match(TOKEN_VAR)) {
        var_declaration();
    } else {
        statement();
    }
    if (parser.panic) {
        syncronize();
    }
}

static void statement()
{
    if (match(TOKEN_PRINT)) {
        print_stmt();
    } else if (match(TOKEN_FOR)) {
        for_stmt();
    } else if (match(TOKEN_IF)) {
        if_stmt();
    } else if (match(TOKEN_RETURN)) {
        return_stmt();
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

static void number(bool can_assign)
{
    double value = strtod(parser.previous.start, NULL);
    emit_constant(NUM_VAL(value));
}

static void string(bool can_assign)
{
    emit_constant(OBJ_VAL(copy_str(parser.previous.start + 1, parser.previous.length - 2)));
}

static void grouping(bool can_assign)
{
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void binary(bool can_assign)
{
    TokenType operator_type = parser.previous.type;
    ParseRule* rule = &rules[operator_type];
    /*
     * A higher precedence is used because equivalent binary operators are left
     * associative. So further tokens with same precedence are not prioritized.
    */
    parse_precedence((Precedence)(rule->precedence + 1));

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

static void call(bool can_assign)
{
    uint8_t args = arg_list();
    emit_bytes(OP_CALL, args);
}

static void dot(bool can_assign)
{
    consume(TOKEN_IDENTIFIER, "Expect property name after '.'.");
    uint8_t name = identifier_const(&parser.previous);

    if (can_assign && match(TOKEN_EQUAL)) {
        expression();
        emit_bytes(OP_SET_PROPERTY, name);
    } else if (match(TOKEN_LEFT_PAREN)) {
        uint8_t args = arg_list();
        emit_bytes(OP_INVOKE, name);
        emit_byte(args);
    } else {
        emit_bytes(OP_GET_PROPERTY, name);
    }
}

static void unary(bool can_assign)
{
    TokenType operator_type = parser.previous.type;

    parse_precedence(PREC_UNARY);

    switch (operator_type) {
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

ObjFun* compile(const char* source)
{
    init_scanner(source);

    Compiler compiler;
    init_compiler(&compiler, TYPE_SCRIPT);

    parser.had_error = false;
    parser.panic = false;

    advance();

    while (!match(TOKEN_EOF)) {
        declaration();
    }
    ObjFun* fun = end_compiler();

    return (parser.had_error) ? NULL : fun;
}

void mark_compiler_roots()
{
    Compiler* compiler = current;

    while (compiler) {
        mark_object((Obj*)compiler->fun);
        compiler = compiler->enclosing;
    }
}