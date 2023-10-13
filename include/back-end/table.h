#ifndef TABLE_H
#define TABLE_H

#include "common.h"
#include "value.h"

#define MAX_LOAD_FACTOR 0.75

typedef struct {
    ObjStr* key;
    Value value;
} Entry;

// Hash table that considers variable names as keys
// and their correspondent value as mappings.
typedef struct {
    // Number of key/value pairs stored.
    int count;
    int size;
    Entry* entries;
} Table;

/// @brief Initializes the specified table, nothing is allocated until needed.
/// @param table hash table
void init_table(Table* table);

/// @brief Frees the entries allocated for the table.
/// @param table hash table
void free_table(Table* table);

/// @brief Searches for an entry with the given key.
/// @param table hash table
/// @param key variable name
/// @param value pointer to the resulting value
/// @return whether the key was found or not
bool table_get(Table* table, ObjStr* key, Value* value);

/// @brief Adds the given key/value pair to the specified hash table.
/// @param table hash table
/// @param key variable name
/// @param value variable's content
/// @return whether the entry added is a new one or not
bool table_set(Table* table, ObjStr* key, Value value);

/// @brief Deletes the entry with the given key from the table.
/// @param table hash table
/// @param key variable name
/// @return whether the value was successfully deleted
bool table_delete(Table* table, ObjStr* key);

/// @brief Copies all entries from one table to another.
/// @param src table containing the entries being transferred
/// @param dest table receiving the new entries
void table_add_all(Table* src, Table* dest);

/// @brief Checks for an interned string with the given content.
/// @param table virtual machine's string table
/// @param chars key to be looked up
/// @param len key length
/// @param hash key's hash value
/// @return pointer to the entry containing that key
ObjStr* table_find(Table* table, const char* chars, int len, uint32_t hash);

#endif