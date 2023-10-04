#ifndef TABLE_H
#define TABLE_H

#include "common.h"
#include "value.h"

#define MAX_LOAD_FACTOR 0.75

typedef struct {
    obj_str_t* key;
    value_t value;
} entry_t;

// Hash table that considers variable names as keys
// and their correspondent value as mappings.
typedef struct {
    // Number of key/value pairs stored.
    int count;
    int size;
    entry_t* entries;
} table_t;

/// @brief Initializes the specified table, nothing is allocated until needed.
/// @param table hash table
void init_table(table_t* table);

/// @brief Frees the entries allocated for the table.
/// @param table hash table
void free_table(table_t* table);

/// @brief Searches for an entry with the given key.
/// @param table hash table
/// @param key variable name
/// @param value pointer to the resulting value
/// @return whether the key was found or not
bool table_get(table_t* table, obj_str_t* key, value_t* value);

/// @brief Adds the given key/value pair to the specified hash table.
/// @param table hash table
/// @param key variable name
/// @param value variable's content
/// @return whether the entry added is a new one or not
bool table_set(table_t* table, obj_str_t* key, value_t value);

/// @brief Deletes the entry with the given key from the table.
/// @param table hash table
/// @param key variable name
/// @return whether the value was successfully deleted
bool table_delete(table_t* table, obj_str_t* key);

/// @brief Copies all entries from one table to another.
/// @param src table containing the entries being transferred
/// @param dest table receiving the new entries
void add_all(table_t* src, table_t* dest);

/// @brief Checks for an interned string with the given content.
/// @param table virtual machine's string table
/// @param chars key to be looked up
/// @param len key length
/// @param hash key's hash value
/// @return pointer to the entry containing that key
obj_str_t* table_find(table_t* table, const char* chars, int len, uint32_t hash);

#endif