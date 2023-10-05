#include <stdlib.h>
#include <string.h>

#include "back-end/object.h"
#include "back-end/table.h"
#include "back-end/value.h"
#include "memory.h"

void init_table(Table* table)
{
    table->count = 0;
    table->size = 0;
    table->entries = NULL;
}

void free_table(Table* table)
{
    FREE_ARRAY(Entry, table->entries, table->size);
    init_table(table);
}

/// @brief Determines which entry the key provided should go in.
/// @param entries table's buckets
/// @param size table capacity
/// @param key variable to be looked up
/// @return table entry corresponding to the object key
static Entry* find_entry(Entry* entries, int size, ObjStr* key)
{
    uint32_t index = key->hash % size;
    Entry* tombstone = NULL;

    while (true) {
        Entry* entry = &entries[index];

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

bool table_get(Table* table, ObjStr* key, Value* value)
{
    if (table->count == 0)
        return false;

    Entry* entry = find_entry(table->entries, table->size, key);

    if (!entry)
        return false;

    *value = entry->value;

    return true;
}

/// @brief Resizes and reorganizes the table's entry array.
/// @param table hash table
/// @param size updated capacity
static void adjust_size(Table* table, int size)
{
    Entry* entries = ALLOCATE(Entry, size);

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
        Entry* entry = &table->entries[i];

        if (!entry->key)
            // Ignores both empty entries and tombstones.
            continue;

        Entry* dest = find_entry(entries, size, entry->key);
        dest->key = entry->key;
        dest->value = entry->value;
        table->count++;
    }

    FREE_ARRAY(Entry, table->entries, table->size);
    table->entries = entries;
    table->size = size;
}

bool table_set(Table* table, ObjStr* key, Value value)
{
    if (table->count + 1 > table->size * MAX_LOAD_FACTOR) {
        int size = GROW_CAPACITY(table->size);

        adjust_size(table, size);
    }

    Entry* entry = find_entry(table->entries, table->size, key);
    bool new_key = !entry->key;

    if (new_key && IS_NIL(entry->value))
        table->count++;

    entry->key = key;
    entry->value = value;

    return new_key;
}

bool table_delete(Table* table, ObjStr* key)
{
    if (table->count == 0)
        return false;

    Entry* entry = find_entry(table->entries, table->size, key);

    if (!entry->key)
        return false;

    // A tombstone is placed in the entry so that the probe sequence
    // used when searching a key is not broken.
    entry->key = NULL;
    entry->value = BOOL_VAL(true);

    return true;
}

void add_all(Table* src, Table* dest)
{
    for (int i = 0; i < src->size; i++) {
        Entry* entry = &src->entries[i];

        if (entry->key)
            table_set(dest, entry->key, entry->value);
    }
}

ObjStr* table_find(Table* table, const char* chars, int len, uint32_t hash)
{
    if (table->count == 0)
        return NULL;

    uint32_t index = hash % table->size;

    while (true) {
        Entry* entry = &table->entries[index];

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