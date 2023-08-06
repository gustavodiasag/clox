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
    // When `old_size` is zero, it is equivalent to calling `malloc()`.
    void *result = realloc(ptr, new_size);

    if (!result)
        exit(1);

    return result;
}

/// @brief Deallocates the specified object considering its type.
/// @param obj object to be freed from memory
static void free_obj(obj_t *obj)
{
    switch(obj->type) {
        case OBJ_FUNC: {
            obj_func_t *func = (obj_func_t *)obj;
            free_chunk(&func->chunk);

            FREE(obj_func_t, obj);
            break;
        }
        case OBJ_STR: {
            obj_str_t *str = (obj_str_t *)obj;
            
            FREE(obj_str_t, obj);
            break;
        }
    }
}

/// @brief Frees the whole linked list of objects from the virtual machine.
void free_objs()
{
    obj_t *obj = vm.objects;

    while (obj) {
        obj_t *next = obj->next;
        free_obj(obj);
        obj = next;
    }
}