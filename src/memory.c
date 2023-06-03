#include <stdlib.h>

#include "memory.h"

/*
 * Responsible for allocating, freeing, shrinking and expanding
 * the size of an existing dynamic allocation.
 */
void *reallocate(void *pointer, size_t old_size, size_t new_size)
{
    if (new_size == 0) {
        free(pointer);
        return NULL;
    }
    // When old_size is zero, realloc is equivalent to calling malloc().
    void *result = realloc(pointer, new_size);

    if (!result)
        exit(1);

    return result;
}