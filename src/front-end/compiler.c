#include <stdio.h>
#include <stdlib.h>

#include "back-end/vm.h"
#include "common.h"
#include "front-end/compiler.h"
#include "front-end/scanner.h"

parser_t parser;
chunk_t *compiling_chunk;

bool compile(const char *source, chunk_t *chunk)
{
    init_scanner(source);

    compiling_chunk = chunk;
    parser.had_error = false;
    parser.panic = false;

    advance();
    expression();
    consume(TOKEN_EOF, "Expect end of expression");

    end_compiler();

    return !parser.had_error;
}

void advance()
{
    parser.previous = parser.current;

    while (true) {
        parser.current = scan_token();
        
        if (parser.current.type != TOKEN_ERROR)
            break;

        error_at_current(parser.current.start);
    }
}

void consume(token_type_t type, const char *message)
{
    if (parser.current.type == type) {
        advance();

        return;
    }

    error_at_current(message);
}

void emit_byte(uint8_t byte)
{
    write_chunk(current_chunk(), byte, parser.previous.line);
}

void emit_bytes(uint8_t byte_1, uint8_t byte_2)
{
    emit_byte(byte_1);
    emit_byte(byte_2);
}

chunk_t *current_chunk()
{
    return compiling_chunk;
}

void end_compiler()
{
    emit_return();
}

void emit_return()
{
    emit_byte(OP_RETURN);
}

void error_at_curret(const char *message)
{
    error_at(&parser.previous, message);
}

void error_at(token_t *token, const char *message)
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