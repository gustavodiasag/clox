#ifndef MEMORY_H
#define MEMORY_H

#include "back-end/object.h"
#include "common.h"

#define ALLOCATE(type, count) \
    (type*)reallocate(NULL, 0, sizeof(type) * (count))

#define GROW_CAPACITY(capacity) \
    ((capacity) < 8 ? 8 : capacity * 2)

// Gets the size of the array's element type and casts the
// resulting void pointer back to the right type.
#define GROW_ARRAY(type, ptr, old_count, new_count)  \
    (type*)reallocate(ptr, sizeof(type) * old_count, \
        sizeof(type) * new_count)

// Wrapper around `reallocate` that resizes an allocation
// down to zero bytes.
#define FREE(type, ptr) \
    reallocate(ptr, sizeof(type), 0)

#define FREE_ARRAY(type, ptr, old_count) \
    reallocate(ptr, sizeof(type) * old_count, 0)

/// @brief Allocates, frees, shrinks and expands the size of a dynamic allocation.
/// @param ptr pointer to the block of memory allocated
/// @param old_size previous size of allocation
/// @param new_size size to be allocated
/// @return pointer to the newly allocated memory
void* reallocate(void* pointer, size_t old_size, size_t new_size);

/// @brief Deallocates the specified object considering its type.
/// @param obj object to be freed from memory
void free_obj(obj_t* obj);

/// @brief Frees the object linked list from the virtual machine.
void free_objs();

#endif