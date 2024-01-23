# clox

clox is a C99 implementation of a bytecode virtual machine interpreter for the [Lox programming language](https://www.craftinginterpreters.com/the-lox-language.html) following the third part of Robert Nystrom's [Crafting Interpreters](https://craftinginterpreters.com/).

Two versions of the interpreter are instructed by the author, the first one being written in Java, which uses an AST-based style of implementation.

The main reason for writing another Lox interpreter using a different approach is that walking through an Abstract Syntax Tree is not [memory-efficient](http://gameprogrammingpatterns.com/data-locality.html), since each piece of syntax in the language becomes a heap-stored AST node. To avoid that, this implementation focuses on exploring and taking advantage of CPU caching by ways of representing code in memory in a denser and more ordered way.

# Usage

## Build

The project relies on [CMake](https://cmake.org/download/) (3.16.3+), which is strictly required in order to build it. After installation, simply clone the project and run: 

```sh
$ mkdir build && cd build/    # CMake workspace.
$ cmake .. && cmake --build .
```

A few customizable build settings can be set using CMake's `CACHE` entries:

```
cmake [-D <option>=1 ] ..
```

Where options are non-exclusive and consist of:

- `DEBUG`: logs the bytecode compiled and the runtime stack state during the execution of each instruction.

- `LOG_GC`: triggers garbage collection more frequently, logging its [tracing](NOTES.md/#mark-sweep-garbage-collection) and the amount of memory reclaimed.

- `OPTIMIZE`: changes clox's representation of values, switching from tagged unions to NaN-boxing.

## Run

Lox programs can be interpreted as source files or through a REPL interface, by just omitting the file path. A few [example programs](examples/) are provided.

```sh
$ ./clox [filepath]
```

# Lox

In a brief description, Lox is a high-level, dynamically typed, garbage-collected programming language that borrows ideas from functional, procedural and object-oriented programming to provide the flexibility expected of a simple scripting language.

It provides most of the functionalities from a basic language implementation, such as data types, expressions (arithmetic, comparative and logical), statements, variables, control flow (conditionals and loops) and functions. Besides that, advanced features such as classes and closures are also specified, whose complexity is deserving of a more thorough depiction.

## Classes

A class and its methods can be declared in Lox like so:

```js
class Example {
    foo() {
        print "Foo.";
    }

    bar(baz) {
        print "This is" + baz + ".";
    }
}
```

When the declaration is executed, Lox creates a class object and stores it in a variable named after the class, which turns it to a first-class value in the language.

Classes in Lox are also factory functions for instances, producing them when called like so.

```js
var example = Example();
print example; // "Example instance".
```

## Grammar

The syntax rules in the [Extended Backus-Naur Form](https://en.wikipedia.org/wiki/Extended_Backus%E2%80%93Naur_form) (EBNF) notation for Lox are presented below.

```ebnf
program         = declaration * EOF ;

(* Declarations. *)

declaration     = class_decl | fun_decl | var_decl | statement ;

class_decl      = "class" IDENTIFIER ( "<" IDENTIFIER ) ? "{" function * "}"

fun_decl        = "fun" function ;

var_decl        = "var" IDENTIFIER ( "=" expression ) ? ";" ;

(* Statements. *)

statement       = expr_stmt
                | for_stmt
                | if_stmt
                | print_stmt
                | return_stmt
                | while_stmt
                | block
                ;

expr_stmt       = expression ";" ;

for_stmt        = "for" "(" ( var_decl | expr_stmt | ";" )
                    expression ? ";"
                    expression ? ")" statement

if_stmt         = "if" "(" expression ")" statement ("else" statement) ? ;

print_stmt      = "print" expression ";" ;

return_stmt     = "return" expression ? ";" ;

whle_stmt       = "while" "(" expression ")" statement ;

block           = "{" declaration * "}" ;

(* Expressions. *)

expression      = assignment ;

assignment      = ( call "." ) ? IDENTIFIER "=" assignment | logic_or ;

logic_or        = logic_and ( "or" logic_and ) * ;

logic_and       = equality ( "and" equality) * ;

equality        = comparison ( ( "!=" | "==" ) comparison ) * ;

comparison      = term ( ( ">" | ">=" | "<" | "<=" ) term ) * ;

term            = factor ( ( "-" | "+" ) factor ) * ;

factor          = unary ( ( "/" | "*" ) unary ) * ;

unary           = ( "!" | "-" ) unary | call ;

call            = primary ( "(" arguments ? ")" | "." IDENTIFIER ) * ;

primary         = "true"
                | "false"
                | "nil"
                | "this"
                | NUMBER
                | STRING
                | IDENTIFIER
                | "(" expression ")"
                | "super" "." IDENTIFIER
                ;

(* Utility rules. *)

function        = IDENTIFIER "(" parameters ? ")" block ;

parameters      = IDENTIFIER ( "," IDENTIFIER ) * ;

arguments       = expression ( "," expression ) * ;

(* Lexical grammar. *)

NUMBER          = DIGIT + ( "." DIGIT ) ? ;

STRING          = '"' ( ? all characters ? - '"' ) * '"' ;

IDENTIFIER      = ALPHA ( ALPHA | DIGIT ) * ;

ALPHA           = "a" ... "z" | "A" ... "Z" | "_" ;

DIGIT           = "0" ... "9" ;
```

# Notes

Besides the main purpose of the book, which is the actual implementation of the interpreters, a bunch of concepts and theorems regarding computer science as a whole is also presented throughout its content. Considering that some of this information, if not all of it, is crucial for one's path becoming a somewhat decent computer scientist, a whole [separate section](NOTES.md) is dedicated to it.

# License

This project is licensed under the [MIT License](LICENSE).