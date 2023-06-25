# clox

CLox is a C99 implementation of a bytecode virtual machine interpreter for the [Lox programming language](https://www.craftinginterpreters.com/the-lox-language.html) following the third part of Robert Nystrom's book [Crafting Interpreters](https://craftinginterpreters.com/).

The main reason for writing another Lox interpreter using a different approach is that walking through an Abstract Syntax Tree is not [memory-efficient](http://gameprogrammingpatterns.com/data-locality.html), since each piece of syntax in the language becomes a heap-stored AST node. To avoid that, this implementation focuses on exploring and taking advantage of CPU caching by ways of representing code in memory in a dense and ordered way.

# Notes

Besides the main purpose of the book, which is the actual implementation of the interpreters, a bunch of concepts and theorems regarding computer science as a whole is also presented throughout its content. Considering that some of this information, if not all of it, is crucial for one's path becoming a somewhat decent computer scientist, a whole [separate section](NOTES.md) is dedicated to it.

# Build

In order to build and compile clox, you need to install CMake. After setting it up, build the project as follows:

```bash
https://github.com/gustavodiasag/clox.git
cd clox/
mkdir build
cd build
cmake ..
cmake --build .
```

# License

This project is licensed under the [MIT License](LICENSE)