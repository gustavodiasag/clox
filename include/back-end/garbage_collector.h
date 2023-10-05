#ifndef GARBAGE_COLLECTOR_H
#define GARBAGE_COLLECTOR_H

#include "vm.h"

/// @brief Marks a heap-stored value from the language
/// @param obj object referenced by the value
void mark_object(Obj* obj);

/// @brief Marks stack-stored values for garbage collection.
/// @param value content stored in a certain stack slot
void mark_value(Value value);

/// @brief Frees the table's keys not marked for reachability.
/// @param table
void table_remove_white(Table* table);

/// @brief Marks all the positions from the given table.
/// @param table hash table
void mark_table(Table* table);

// TODO: Description
void collect_garbage();

#endif