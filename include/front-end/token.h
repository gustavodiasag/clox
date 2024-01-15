#ifndef TOKEN_H
#define TOKEN_H

/**
 * Represents all of the tokens that could possibly be generated during lexing.
 */
typedef enum
{
    /* Single character. */
    TOKEN_LEFT_PAREN,
    TOKEN_RIGHT_PAREN,
    TOKEN_LEFT_BRACE,
    TOKEN_RIGHT_BRACE,
    TOKEN_COMMA,
    TOKEN_DOT,
    TOKEN_MINUS,
    TOKEN_PLUS,
    TOKEN_STAR,
    TOKEN_SLASH,
    TOKEN_SEMICOLON,
    TOKEN_BANG,
    TOKEN_EQUAL,
    TOKEN_GREATER,
    TOKEN_LESS,
    /* Two character. */
    TOKEN_BANG_EQUAL,
    TOKEN_EQUAL_EQUAL,
    TOKEN_GREATER_EQUAL,
    TOKEN_LESS_EQUAL,
    /* Literals. */
    TOKEN_IDENTIFIER,
    TOKEN_STRING,
    TOKEN_NUMBER,
    /* Keywords. */
    TOKEN_AND,
    TOKEN_CLASS,
    TOKEN_ELSE,
    TOKEN_FALSE,
    TOKEN_FOR,
    TOKEN_FUN,
    TOKEN_IF,
    TOKEN_NIL,
    TOKEN_OR,
    TOKEN_PRINT,
    TOKEN_RETURN,
    TOKEN_SUPER,
    TOKEN_THIS,
    TOKEN_TRUE,
    TOKEN_VAR,
    TOKEN_WHILE,
    /* Error. */
    TOKEN_ERROR,
    /* EOF. */
    TOKEN_EOF
} TokenType;

/**
 * Structure representing a token for the language.
 * 
 * `type` is the type of the token.
 * `starts` is a pointer to the lexeme's first character in the source stream.
 * `length` is the size of the lexeme.
 * `line` is where the token was found in the source.
 */
typedef struct
{
    TokenType   type;
    const char* start;
    int         length;
    int         line;
} Token;

#endif