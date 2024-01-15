#ifndef COMPILER_H
#define COMPILER_H

#include "back-end/object.h"
#include "scanner.h"

/**
 * Compiles the program represented as a stream of characters specified by
 * `source`.
 * 
 * Returns a pointer to the top-level function containing the bytecode
 * generated during compilation.
 */
ObjFun* compile(const char* source);

/**
 * Marks all the objects currently allocated on the heap during compilation.
 */
void mark_compiler_roots();

#endif