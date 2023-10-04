#include <stdlib.h>

#include "back-end/garbage_collector.h"
#include "back-end/vm.h"
#include "memory.h"

#ifdef DEBUG_LOG_GC

#include "debug.h"
#include <stdio.h>

#endif

void* reallocate(void* ptr, size_t old_size, size_t new_size)
{
    vm.bytes_allocated += (new_size - old_size);

    if (new_size > old_size)
#ifdef DEBUG_STRESS_GC
        collect_garbage();
#endif

    if (vm.bytes_allocated > vm.next_gc)
        collect_garbage();

    if (new_size == 0) {
        free(ptr);
        return NULL;
    }
    // When `old_size` is zero, it is equivalent to calling `malloc()`.
    void* result = realloc(ptr, new_size);

    if (!result)
        exit(1);

    return result;
}

void free_obj(obj_t* obj)
{
#ifdef DEBUG_LOG_GC
    printf("%p free type %d\n", (void*)obj, obj->type);
#endif

    switch (obj->type) {
    case OBJ_BOUND_METHOD: {
        FREE(obj_bound_method_t, obj);
        break;
    }
    case OBJ_CLASS: {
        obj_class_t* class = (obj_class_t*)obj;
        // The class object owns the memory for the method table,
        // so when deallocating a class, the table must be freed.
        free_table(&class->methods);
        FREE(obj_class_t, obj);
        break;
    }
    // Only the closure object is freed and not the function it
    // encloses, since the closure doesn't own the function. There
    // may be multiple closures that reference the same function.
    case OBJ_CLOSURE: {
        obj_closure_t* closure = (obj_closure_t*)obj;
        FREE_ARRAY(obj_upvalue_t*, closure->upvalues, closure->upvalue_count);
        FREE(obj_closure_t, obj);
        break;
    }
    case OBJ_FUNC: {
        obj_func_t* func = (obj_func_t*)obj;
        free_chunk(&func->chunk);

        FREE(obj_func_t, obj);
        break;
    }
    case OBJ_INSTANCE: {
        obj_instance_t* instance = (obj_instance_t*)obj;
        free_table(&instance->fields);

        FREE(obj_instance_t, obj);
        break;
    }
    case OBJ_NATIVE: {
        FREE(obj_native_t, obj);
        break;
    }
    case OBJ_STR: {
        obj_str_t* str = (obj_str_t*)obj;

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

void free_objs()
{
    obj_t* obj = vm.objects;

    while (obj) {
        obj_t* next = obj->next;
        free_obj(obj);
        obj = next;
    }

    free(vm.gray_stack);
}