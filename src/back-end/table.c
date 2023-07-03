#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "back-end/object.h"
#include "back-end/table.h"
#include "back-end/value.h"

/// @brief Initializes the specified table, nothing is allocated until needed.
/// @param table
void init_table(table_t *table)
{
    table->count = 0;
    table->size = 0;
    table->entries = NULL;
}

/// @brief Frees the entries allocated for the table, treating it as an array.
/// @param table 
void free_table(table_t *table)
{
    FREE_ARRAY(entry_t, table->entries, table->size);
    init_table(table);
}

static entry_t *find_entry(entry_t *entries, int size, obj_str_t *key)
{
    uint32_t index = key->hash % size;

    while (true) {
        entry_t *entry = &entries[index];

        if (entry->key == key || !entry->key)
            return entry;

        index = (index + 1) % size;
    }
}

/// @brief Adds the given key/value pair to the given hash table.
/// @param table 
/// @param key 
/// @param value 
/// @return whether the entry added is a new one or not
bool table_set(table_t *table, obj_str_t *key, value_t value)
{
    if (table->count + 1 > table->size * MAX_LOAD_FACTOR) {
        int size = GROW_CAPACITY(table->size);

        adjust_size(table, size);
    }

    entry_t *entry = find_entry(table->entries, table->size, key);
    bool new_key = !entry->key;

    if (new_key)
        table->count++;

    entry->key = key;
    entry->value = value;

    return new_key;
}