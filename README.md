# clox

clox is a C99 implementation of a bytecode virtual machine interpreter for the [Lox programming language](https://www.craftinginterpreters.com/the-lox-language.html) following the third part of Robert Nystrom's [Crafting Interpreters](https://craftinginterpreters.com/).

Two versions of the interpreter are instructed by the author, the first one being written in Java, which uses an AST-based style of implementation.

The main reason for writing another Lox interpreter using a different approach is that walking through an Abstract Syntax Tree is not [memory-efficient](http://gameprogrammingpatterns.com/data-locality.html), since each piece of syntax in the language becomes a heap-stored AST node. To avoid that, this implementation focuses on exploring and taking advantage of CPU caching by ways of representing code in memory in a denser and more ordered way.

# Notes

Besides the main purpose of the book, which is the actual implementation of the interpreters, a bunch of concepts and theorems regarding computer science as a whole is also presented throughout its content. Considering that some of this information, if not all of it, is crucial for one's path becoming a somewhat decent computer scientist, a whole [separate section](NOTES.md) is dedicated to it.

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

- `DEBUG`: logs the bytecode generated after compilation and the runtime stack state during the execution of each instruction.

- `LOG_GC`: triggers garbage collection more frequently, logging its [tracing](NOTES.md/#mark-sweep-garbage-collection) and the amount of memory reclaimed.

- `OPTIMIZE`: changes clox's representation of values, switching from tagged unions to NaN-boxing.

## Run

Lox programs can be interpreted as source files or through a REPL interface, by just omitting the file path. 

```sh
$ ./clox [filepath]
```

# License

This project is licensed under the [MIT License](LICENSE).