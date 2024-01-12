#include <stdlib.h>

#ifdef DEBUG_LOG_GC

#include "debug.h"
#include <stdio.h>

#endif

#include "back-end/garbage_collector.h"
#include "back-end/vm.h"
#include "memory.h"

void free_obj(Obj* obj)
{
#ifdef DEBUG_LOG_GC
    printf("%p free type %d\n", (void*)obj, obj->type);
#endif
    switch (obj->type) {
    case OBJ_BOUND_METHOD: {
        FREE(ObjBoundMethod, obj);
        break;
    }
    case OBJ_CLASS: {
        ObjClass* class = (ObjClass*)obj;
        free_table(&class->methods);
        FREE(ObjClass, obj);
        break;
    }
    /* Only the closure object is freed since it doesn't own its function. */
    case OBJ_CLOSURE: {
        ObjClosure* closure = (ObjClosure*)obj;
        FREE_ARRAY(ObjUpvalue*, closure->upvalues, closure->upvalue_count);
        FREE(ObjClosure, obj);
        break;
    }
    case OBJ_FUNC: {
        ObjFun* func = (ObjFun*)obj;
        free_chunk(&func->chunk);
        FREE(ObjFun, obj);
        break;
    }
    case OBJ_INSTANCE: {
        ObjInst* instance = (ObjInst*)obj;
        free_table(&instance->fields);
        FREE(ObjInst, obj);
        break;
    }
    case OBJ_NATIVE: {
        FREE(ObjNative, obj);
        break;
    }
    case OBJ_STR: {
        ObjStr* str = (ObjStr*)obj;
        FREE(ObjStr, obj);
        break;
    }
    case OBJ_UPVALUE: {
        FREE(ObjUpvalue, obj);
        break;
    }
    }
}

void free_objs()
{
    Obj* obj = vm.objects;

    while (obj) {
        Obj* next = obj->next;
        free_obj(obj);
        obj = next;
    }
    free(vm.gray_stack);
}

void* reallocate(void* ptr, size_t old_size, size_t new_size)
{
    vm.bytes_allocated += (new_size - old_size);

    if (new_size > old_size)
#ifdef DEBUG_STRESS_GC
        collect_garbage();
#endif
    if (vm.bytes_allocated > vm.next_gc) {
        collect_garbage();
    }
    if (new_size == 0) {
        free(ptr);
        return NULL;
    }
    void* result = realloc(ptr, new_size);

    if (!result) {
        exit(1);
    }
    return result;
}