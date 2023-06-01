# clox

CLox is a C99 implementation of a bytecode virtual machine interpreter for the [Lox programming language](https://www.craftinginterpreters.com/the-lox-language.html) following the third part of Robert Nystrom's book [Crafting Interpreters](https://craftinginterpreters.com/).

The main reason for writing another Lox interpreter using a different approach is that walking through an Abstract Syntax Tree is not [memory-efficient](http://gameprogrammingpatterns.com/data-locality.html), since each piece of syntax in the language becomes a heap-stored AST node. To avoid that, this implementation focuses on exploring and taking advantage of CPU caching by ways of representing code in memory in a dense and ordered way.

# Notes