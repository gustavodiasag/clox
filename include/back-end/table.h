#ifndef TABLE_H
#define TABLE_H

#include "common.h"
#include "value.h"

/**
 * Represents a hash table slot.
 * 
 * `key` is a string object with the hash key for the entry.
 * `value` is the value associated with the key.
 */
typedef struct
{
    ObjStr* key;
    Value   value;
} Entry;

/**
 * Structure representing a hash table, which is defined as a dynamic array of
 * entries.
 * 
 * `count` is the current number of entries in the table.
 * `size` is the capacity of the table.
 * `entries` is the array of table entries.
 */
typedef struct
{
    int     count;
    int     size;
    Entry*  entries;
} Table;

/**
 * Initializes a hash table specified by `table`, not performing any
 * allocation.
 */
void init_table(Table* table);

/**
 * Frees the entry array from a table specified by `table` and resets its
 * content.
 */
void free_table(Table* table);

/**
 * Searches for an entry with a key specified by `key` on a hash table
 * specified by `table`. If an entry is found, `value` points to its value, or
 * `NULL` otherwise.
 * 
 * Returns whether the entry was found or not.
 */
bool table_get(Table* table, ObjStr* key, Value* value);

/**
 * Inserts an entry composed of `key and `value` to a hash table specified by
 * `table`.
 * 
 * Returns whether the entry was successfully inserted or not. 
 */
bool table_set(Table* table, ObjStr* key, Value value);

/**
 * Removes an entry based on its key, specified by `key`, from a hash table
 * specified by `table`.
 * 
 * Returns whether the entry deletion was successfull or not.
 * 
*/
bool table_delete(Table* table, ObjStr* key);

/**
 * Copies all entries from a hash table specified by `src` into a hash table
 * `specified by `dest`.
 */
void table_add_all(Table* src, Table* dest);

/**
 * Searches for an interned string in a hash table specified by `table` that
 * matches the length, hash code and content of a stream of characters,
 * respectively specified by `len`, `hash` and `chars`.
 * 
 * Returns a pointer to the key if it was found, or `NULL` otherwise.
 */
ObjStr* table_find(Table* table, const char* chars, int len, uint32_t hash);

#endif