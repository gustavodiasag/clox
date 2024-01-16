#ifndef GARBAGE_COLLECTOR_H
#define GARBAGE_COLLECTOR_H

#include "vm.h"

/** Marks a heap-stored value specified by `obj` for collection. */
void mark_object(Obj* obj);

/** Marks a stack-stored value specified by `value` for collection. */
void mark_value(Value value);

/**
 * Frees the keys not marked as reachable from a table specified by `table`.
 */
void table_remove_white(Table* table);

/** Marks all the positions from a table specified by `table`. */
void mark_table(Table* table);

/** Triggers garbage collection for all ends of the interpreter. */
void collect_garbage();

#endif