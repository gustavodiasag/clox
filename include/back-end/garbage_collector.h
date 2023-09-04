#pragma once

#include "vm.h"

void mark_object(obj_t* obj);
void mark_value(value_t value);
void table_remove_white(table_t* table);
void mark_table(table_t* table);
void collect_garbage();