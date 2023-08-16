#include <stdlib.h>

#include "memory.h"
#include "back-end/garbage_collector.h"
#include "back-end/vm.h"

#ifdef DEBUG_LOG_GC

#include <stdio.h>
#include "debug.h"

#endif

/// @brief Allocates, frees, shrinks and expands the size of a dynamic allocation.
/// @param ptr pointer to the block of memory allocated
/// @param old_size previous size of allocation
/// @param new_size size to be allocated
/// @return pointer to the newly allocated memory
void *reallocate(void *ptr, size_t old_size, size_t new_size)
{
    if (new_size > old_size)
#ifdef DEBUG_STRESS_GC
        collect_garbage();
#endif

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
#ifdef DEBUG_LOG_GC
    printf("%p free type %d\n", (void *)obj, obj->type);
#endif

    switch(obj->type) {
        // Only the closure object is freed and not the function it
        // encloses, since the closure doesn't own the function. There
        // may be multiple closures that reference the same function.
        case OBJ_CLOSURE: {
            obj_closure_t *closure = (obj_closure_t *)obj;
            FREE_ARRAY(obj_upvalue_t *, closure->upvalues, closure->upvalue_count);
            FREE(obj_closure_t, obj);
            break;
        }
        case OBJ_FUNC: {
            obj_func_t *func = (obj_func_t *)obj;
            free_chunk(&func->chunk);

            FREE(obj_func_t, obj);
            break;
        }
        case OBJ_NATIVE: {
            FREE(obj_native_t, obj);
            break;
        }
        case OBJ_STR: {
            obj_str_t *str = (obj_str_t *)obj;
            
            FREE(obj_str_t, obj);
            break;
        }
        // Multiple closures can close over the same variable, so
        // `obj_upvalue_t` doesn't own the variable it references.
        case OBJ_UPVALUE: {
            FREE(obj_upvalue_t, obj);
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

    free(vm.gray_stack);
}