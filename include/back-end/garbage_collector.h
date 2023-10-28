#ifndef GARBAGE_COLLECTOR_H
#define GARBAGE_COLLECTOR_H

#include "vm.h"

/** Marks a heap-stored value specified by `o`. */
void mark_object(Obj* o);

/** Marks a stack-stored value specified by `v`. */
void mark_value(Value value);

/** Frees the keys not marked as reachable from a table specified by `t`. */
void table_remove_white(Table* t);

/** Marks all the positions from a table specified by `t`. */
void mark_table(Table* table);

// FIXME: Description
void collect_garbage();

#endif