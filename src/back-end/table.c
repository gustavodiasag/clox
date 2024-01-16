#include <stdlib.h>
#include <string.h>

#include "back-end/object.h"
#include "back-end/table.h"
#include "back-end/value.h"
#include "memory.h"

static Entry* find_entry(Entry* entries, int size, ObjStr* key)
{
    uint32_t index = key->hash & (size - 1);
    Entry* tombstone = NULL;

    while (true) {
        Entry* entry = &entries[index];

        if (!entry->key) {
            if (IS_NIL(entry->value)) {
                /* Empty entry. */
                return (tombstone) ? tombstone : entry;
            } else {
                if (!tombstone)
                    tombstone = entry;
            }
        } else if (entry->key == key) {
            return entry;
        }
        /* Collision handling (linear probing). */
        index = (index + 1) & (size - 1);
    }
}

static void adjust_size(Table* table, int size)
{
    Entry* entries = ALLOCATE(Entry, size);

    for (int i = 0; i < size; i++) {
        entries[i].key = NULL;
        entries[i].value = NIL_VAL;
    }
    /* Tombstones don't transfer, so the entry amount must be recalculated. */
    table->count = 0;
    /* Entries must be remapped. */
    for (int i = 0; i < table->size; i++) {
        Entry* entry = &table->entries[i];

        if (!entry->key) {
            /* Ignore both empty entries and tombstones. */
            continue;
        }
        Entry* dest = find_entry(entries, size, entry->key);
        dest->key = entry->key;
        dest->value = entry->value;
        table->count++;
    }

    FREE_ARRAY(Entry, table->entries, table->size);
    table->entries = entries;
    table->size = size;
}

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

bool table_get(Table* table, ObjStr* key, Value* value)
{
    if (!table->count) {
        return false;
    }
    Entry* entry = find_entry(table->entries, table->size, key);

    if (!entry->key) {
        return false;
    }
    *value = entry->value;
    return true;
}

bool table_set(Table* table, ObjStr* key, Value value)
{
    if (table->count + 1 > table->size * MAX_LOAD_FACTOR) {
        int size = GROW_CAPACITY(table->size);
        adjust_size(table, size);
    }
    Entry* entry = find_entry(table->entries, table->size, key);
    bool new_key = !entry->key;

    if (new_key && IS_NIL(entry->value)) {
        table->count++;
    }
    entry->key = key;
    entry->value = value;

    return new_key;
}

bool table_delete(Table* table, ObjStr* key)
{
    if (!table->count) {
        return false;
    }
    Entry* entry = find_entry(table->entries, table->size, key);

    if (!entry->key) {
        return false;
    }
    /* A tombstone is placed so that a future probe sequence doesn't break. */
    entry->key = NULL;
    entry->value = BOOL_VAL(true);

    return true;
}

void table_add_all(Table* src, Table* dest)
{
    for (int i = 0; i < src->size; i++) {
        Entry* entry = &src->entries[i];

        if (entry->key) {
            table_set(dest, entry->key, entry->value);
        }
    }
}

ObjStr* table_find(Table* table, const char* chars, int len, uint32_t hash)
{
    if (!table->count) {
        return NULL;
    }
    uint32_t index = hash & (table->size - 1);

    while (true) {
        Entry* entry = &table->entries[index];

        if (!entry->key) {
            /* Stop if an empty non-tombstone entry is found. */
            if (IS_NIL(entry->value)) {
                return NULL;
            }
        } else if (entry->key->length == len &&
                   entry->key->hash == hash &&
                   memcmp(entry->key->chars, chars, len) == 0) {
            return entry->key;
        }
        index = (index + 1) & (table->size - 1);
    }
}