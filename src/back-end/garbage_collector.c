#include <stdlib.h>

#include "back-end/garbage_collector.h"
#include "front-end/compiler.h"
#include "memory.h"

#ifdef DEBUG_LOG_GC
#include <stdio.h>
#include "debug.h"
#endif

#define GC_HEAP_GROW_FACTOR 2

static void mark_array(ValueArray* array)
{
    for (int i = 0; i < array->count; i++) {
        mark_value(array->values[i]);
    }
}

static void blacken_object(Obj* obj)
{
#ifdef DEBUG_LOG_GC
    printf("%p blacken ", (void*)obj);
    print_value(OBJ_VAL(obj));
    printf("\n");
#endif
    switch (obj->type) {
    case OBJ_BOUND_METHOD: {
        ObjBoundMethod* bound = (ObjBoundMethod*)obj;
        mark_value(bound->receiver);
        mark_object((Obj*)bound->method);
        break;
    }
    case OBJ_CLASS: {
        ObjClass* class = (ObjClass*)obj;
        mark_object((Obj*)class->name);
        mark_table(&class->methods);
        break;
    }
    case OBJ_CLOSURE: {
        ObjClosure* closure = (ObjClosure*)obj;
        mark_object((Obj*)closure->function);

        for (int i = 0; i < closure->upvalue_count; i++) {
            mark_object((Obj*)closure->upvalues[i]);
        }
        break;
    }
    case OBJ_FUNC: {
        ObjFun* func = (ObjFun*)obj;
        mark_object((Obj*)func->name);
        mark_array(&func->chunk.constants);
        break;
    }
    case OBJ_INSTANCE: {
        ObjInst* instance = (ObjInst*)obj;
        mark_object((Obj*)instance->class);
        mark_table(&instance->fields);
        break;
    }
    /* Strings and native function objects contain no outgoing references. */
    case OBJ_NATIVE:
    case OBJ_STR:
        break;
    case OBJ_UPVALUE:
        mark_value(((ObjUpvalue*)obj)->closed);
        break;
    }
}

static void mark_roots()
{
    for (Value* slot = vm.stack; slot < vm.stack_top; slot++) {
        mark_value(*slot);
    }
    /*
     * Call frames have a pointer to the closure being called. The vm uses it
     * to access constants and upvalues, so closures must be marked.
     */
    for (int i = 0; i < vm.frame_count; i++) {
        mark_object((Obj*)vm.frames[i].closure);
    }
    for (ObjUpvalue* upvalue = vm.open_upvalues; upvalue; upvalue = upvalue->next) {
        mark_object((Obj*)upvalue);
    }
    mark_table(&vm.globals);
    /*
     * A compiler periodically gets heap memory for literals and its constant
     * table. If garbage collection is triggered while compilation, any values
     * accessible by the compiler need to be treated as roots.
     */
    mark_compiler_roots();
    mark_object((Obj*)vm.init_string);
}

static void trace_references()
{
    while (vm.gray_count > 0) {
        Obj* obj = vm.gray_stack[--vm.gray_count];
        blacken_object(obj);
    }
}

static void sweep()
{
    Obj* prev = NULL;
    Obj* obj = vm.objects;
    /* Traverses the linked list of heap-stored objects from the vm. */
    while (obj) {
        /* Marked objects are ignored.*/
        if (obj->is_marked) {
            /* Marked objects must be unmarked for the next collection. */
            obj->is_marked = false;
            prev = obj;
            obj = obj->next;
        } else {
            Obj* white = obj;
            obj = obj->next;

            if (prev) {
                prev->next = obj;
            } else {
                vm.objects = obj;
            }
            free_obj(white);
        }
    }
}

void mark_object(Obj* obj)
{
    if (!obj)
        return;

    if (obj->is_marked)
        return;

#ifdef DEBUG_LOG_GC
    printf("%p mark ", (void*)obj);
    print_value(OBJ_VAL(obj));
    printf("\n");
#endif
    obj->is_marked = true;

    if (vm.gray_capacity < vm.gray_count + 1) {
        vm.gray_capacity = GROW_CAPACITY(vm.gray_capacity);
        vm.gray_stack =
            (Obj**)realloc(vm.gray_stack, sizeof(Obj*) * vm.gray_capacity);

        if (!vm.gray_stack) {
            exit(1);
        }
    }
    vm.gray_stack[vm.gray_count++] = obj;
}

void mark_value(Value value)
{
    if (IS_OBJ(value)) {
        mark_object(AS_OBJ(value));
    }
}

void table_remove_white(Table* table)
{
    for (int i = 0; i < table->size; i++) {
        Entry* entry = &table->entries[i];

        if (entry->key && !entry->key->obj.is_marked) {
            table_delete(table, entry->key);
        }
    }
}

void mark_table(Table* table)
{
    for (int i = 0; i < table->size; i++) {
        Entry* entry = &table->entries[i];

        mark_object((Obj*)entry->key);
        mark_value(entry->value);
    }
}

void collect_garbage()
{
#ifdef DEBUG_LOG_GC
    printf("-- gc begin\n");
    /* Heap size before the collection is triggered. */
    size_t before = vm.bytes_allocated;
#endif
    mark_roots();
    trace_references();
    table_remove_white(&vm.strings);
    sweep();

    vm.next_gc = vm.bytes_allocated * GC_HEAP_GROW_FACTOR;
#ifdef DEBUG_LOG_GC
    printf("-- gc end\n");
    /* Total of memory collected. */
    printf("   collected %zu bytes (from %zu to %zu) next at %zu\n",
        before - vm.bytes_allocated, before, vm.bytes_allocated, vm.next_gc);
#endif
}
