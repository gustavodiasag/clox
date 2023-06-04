#pragma once

#include "common.h"

#define GROW_CAPACITY(capacity) \
    ((capacity) < 8 ? 8 : capacity * 2)

// Gets the size of the array's element type and casts the
// resulting void pointer back to the right type.
#define GROW_ARRAY(type, pointer, old_count, new_count) \
    (type *)reallocate(pointer, sizeof(type) * old_count, \
        sizeof(type) * new_count)

#define FREE_ARRAY(type, pointer, old_count) \
    reallocate(pointer, sizeof(type) * old_count, 0)

void *reallocate(void *pointer, size_t old_size, size_t new_size);