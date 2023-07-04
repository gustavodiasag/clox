#pragma once

#include "common.h"
#include "value.h"

#define MAX_LOAD_FACTOR 0.75

typedef struct {
    obj_str_t *key;
    value_t value;
} entry_t;

// Hash table that considers variable names as keys
// and their correspondent value as mappings.
typedef struct {
    // Number of key/value pairs stored.
    int count;
    int size;
    entry_t *entries;
} table_t;

void init_table(table_t *table);
void free_table(table_t *table);
bool table_get(table_t *table, obj_str_t *key, value_t *value);
bool table_set(table_t *table, obj_str_t *key, value_t value);
bool table_delete(table_t *table, obj_str_t *key);
void add_all(table_t *src, table_t *dest);
obj_str_t *table_find(table_t *table, const char *chars, int len, uint32_t hash);