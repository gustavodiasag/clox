#include <stdlib.h>

#include "back-end/chunk.h"
#include "back-end/vm.h"
#include "memory.h"

void init_chunk(chunk_t *chunk)
{
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;
    chunk->lines = NULL;
    init_value_array(&chunk->constants);
}

void free_chunk(chunk_t *chunk)
{
    FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
    FREE_ARRAY(int, chunk->lines, chunk->capacity);
    free_value_array(&chunk->constants);
    init_chunk(chunk);
}

void write_chunk(chunk_t *chunk, uint8_t byte, int line)
{
    if (chunk->capacity < chunk->count + 1) {
        int old_capacity = chunk->capacity;
        chunk->capacity = GROW_CAPACITY(old_capacity);
        chunk->code = GROW_ARRAY(uint8_t, chunk->code, old_capacity, chunk->capacity);
        chunk->lines = GROW_ARRAY(int, chunk->lines, old_capacity, chunk->capacity);
    }

    chunk->code[chunk->count] = byte;
    chunk->lines[chunk->count] = line;
    chunk->count++;
}

// Adds the constant and returns the index where the constant was appended.
int add_constant(chunk_t *chunk, value_t value)
{
    // When writing the value to the chunk's constant table, there may be
    // necessary for the dynamic array to grow it's size, which could
    // trigger collection, sweeping the value before it is added to the
    // table. To fix that, it is temporarily pushed onto the stack.
    push(value);
    write_value_array(&chunk->constants, value);
    // Once the constant table contains the object, it is popped off.
    pop();

    return chunk->constants.count - 1;
}