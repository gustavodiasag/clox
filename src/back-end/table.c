#include <stdlib.h>
#include <string.h>

#include "back-end/object.h"
#include "back-end/table.h"
#include "back-end/value.h"
#include "memory.h"

/// @brief Initializes the specified table, nothing is allocated until needed.
/// @param table hash table
void init_table(table_t* table)
{
    table->count = 0;
    table->size = 0;
    table->entries = NULL;
}

/// @brief Frees the entries allocated for the table, treating it as an array.
/// @param table hash table
void free_table(table_t* table)
{
    FREE_ARRAY(entry_t, table->entries, table->size);
    init_table(table);
}

/// @brief Determines which entry the key provided should go in.
/// @param entries table's buckets
/// @param size table capacity
/// @param key variable to be looked up
/// @return table entry corresponding to the object key
static entry_t* find_entry(entry_t* entries, int size, obj_str_t* key)
{
    uint32_t index = key->hash % size;
    entry_t* tombstone = NULL;

    while (true) {
        entry_t* entry = &entries[index];

        if (!entry->key) {
            if (IS_NIL(entry->value)) {
                // Empty entry.
                return (tombstone) ? tombstone : entry;
            } else {
                if (!tombstone)
                    tombstone = entry;
            }
        } else if (entry->key == key) {
            return entry;
        }
        // Collision handling.
        index = (index + 1) % size;
    }
}

/// @brief Searches for an entry with the given key.
/// @param table hash table
/// @param key variable name
/// @param value pointer to the resulting value
/// @return whether the key was found or not
bool table_get(table_t* table, obj_str_t* key, value_t* value)
{
    if (table->count == 0)
        return false;

    entry_t* entry = find_entry(table->entries, table->size, key);

    if (!entry)
        return false;

    *value = entry->value;

    return true;
}

/// @brief Resizes and reorganizes the table's entry array.
/// @param table hash table
/// @param size updated capacity
static void adjust_size(table_t* table, int size)
{
    entry_t* entries = ALLOCATE(entry_t, size);

    for (int i = 0; i < size; i++) {
        entries[i].key = NULL;
        entries[i].value = NIL_VAL;
    }
    // Since tombstones are not transferred from the old table
    // to the new one, the number of entries is reinitialized.
    table->count = 0;

    // Once resized, the previous entries must be mapped again
    // considering that the table's size has been updated.
    for (int i = 0; i < table->size; i++) {
        entry_t* entry = &table->entries[i];

        if (!entry->key)
            // Ignores both empty entries and tombstones.
            continue;

        entry_t* dest = find_entry(entries, size, entry->key);
        dest->key = entry->key;
        dest->value = entry->value;
        table->count++;
    }

    FREE_ARRAY(entry_t, table->entries, table->size);
    table->entries = entries;
    table->size = size;
}

/// @brief Adds the given key/value pair to the specified hash table.
/// @param table hash table
/// @param key variable name
/// @param value variable's content
/// @return whether the entry added is a new one or not
bool table_set(table_t* table, obj_str_t* key, value_t value)
{
    if (table->count + 1 > table->size * MAX_LOAD_FACTOR) {
        int size = GROW_CAPACITY(table->size);

        adjust_size(table, size);
    }

    entry_t* entry = find_entry(table->entries, table->size, key);
    bool new_key = !entry->key;

    if (new_key && IS_NIL(entry->value))
        table->count++;

    entry->key = key;
    entry->value = value;

    return new_key;
}

/// @brief Deletes the entry with the given key from the table.
/// @param table hash table
/// @param key variable name
/// @return whether the value was successfully deleted
bool table_delete(table_t* table, obj_str_t* key)
{
    if (table->count == 0)
        return false;

    entry_t* entry = find_entry(table->entries, table->size, key);

    if (!entry->key)
        return false;

    // A tombstone is placed in the entry so that the probe sequence
    // used when searching a key is not broken.
    entry->key = NULL;
    entry->value = BOOL_VAL(true);

    return true;
}

/// @brief Copies all entries from one table to another.
/// @param src table containing the entries being transferred
/// @param dest table receiving the new entries
void add_all(table_t* src, table_t* dest)
{
    for (int i = 0; i < src->size; i++) {
        entry_t* entry = &src->entries[i];

        if (entry->key)
            table_set(dest, entry->key, entry->value);
    }
}

/// @brief Checks for an interned string with the given content.
/// @param table virtual machine's string table
/// @param chars key to be looked up
/// @param len key length
/// @param hash key's hash value
/// @return pointer to the entry containing that key
obj_str_t* table_find(table_t* table, const char* chars, int len, uint32_t hash)
{
    if (table->count == 0)
        return NULL;

    uint32_t index = hash % table->size;

    while (true) {
        entry_t* entry = &table->entries[index];

        if (!entry->key) {
            // Stop if an empty non-tombstone entry is found.
            if (IS_NIL(entry->value))
                return NULL;

        } else if (entry->key->length == len && entry->key->hash == hash && memcmp(entry->key->chars, chars, len) == 0) {

            return entry->key;
        }

        index = (index + 1) % table->size;
    }
}