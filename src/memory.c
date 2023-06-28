#include <stdlib.h>

#include "memory.h"
#include "back-end/vm.h"

/// @brief Allocates, frees, shrinks and expands the size of a dynamic allocation.
/// @param ptr pointer to the block of memory allocated
/// @param old_size previous size of allocation
/// @param new_size size to be allocated
/// @return pointer to the newly allocated memory
void *reallocate(void *ptr, size_t old_size, size_t new_size)
{
    if (new_size == 0) {
        free(ptr);
        return NULL;
    }
    // When old_size is zero, realloc is equivalent to calling malloc().
    void *result = realloc(ptr, new_size);

    if (!result)
        exit(1);

    return result;
}