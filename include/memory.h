#ifndef MEMORY_H
#define MEMORY_H

#include "back-end/object.h"
#include "common.h"

#define ALLOCATE(type, count) \
    (type*)reallocate(NULL, 0, sizeof(type) * (count))

#define GROW_CAPACITY(capacity) \
    ((capacity) < 8 ? 8 : capacity * 2)

#define GROW_ARRAY(type, ptr, old_count, new_count)  \
    (type*)reallocate(ptr, sizeof(type) * old_count, sizeof(type) * new_count)

#define FREE(type, ptr) \
    reallocate(ptr, sizeof(type), 0)

#define FREE_ARRAY(type, ptr, old_count) \
    reallocate(ptr, sizeof(type) * old_count, 0)


/** Deallocates an object specified by `obj` based on its tag. */
void free_obj(Obj* obj);

/** Frees the virtual machine's object references. */
void free_objs();

/**
 * Manages the amount of memory pointed by `ptr` based on the sizes specified
 * by `old_size` and `new_size`.
 * 
 * When `old_size` is 0 and `new_size` is a positive value, `reallocate`
 * functions as a call to `malloc`. When `old_size` is positive and `new_size`
 * is 0, `reallocate` functions as a call to `free`.
 * 
 * Returns a pointer to the newly allocated memory.
 */
void* reallocate(void* ptr, size_t old_size, size_t new_size);

#endif