#include <stdlib.h>

#include "back-end/garbage_collector.h"
#include "front-end/compiler.h"
#include "memory.h"

#ifdef DEBUG_LOG_GC

#include <stdio.h>
#include "debug.h"

#endif

#define GC_HEAP_GROW_FACTOR 2

/// @brief Marks a heap-stored value from the language
/// @param obj object referenced by the value 
void mark_object(obj_t *obj)
{
    if (!obj)
        return;

    if (obj->is_marked)
        return;

#ifdef DEBUG_LOG_GC
    printf("%p mark ", (void *)obj);
    print_value(OBJ_VAL(obj));
    printf("\n");
#endif

    obj->is_marked = true;

    if (vm.gray_capacity < vm.gray_count + 1) {
        vm.gray_capacity = GROW_CAPACITY(vm.gray_capacity);
        vm.gray_stack = (obj_t **)realloc(vm.gray_stack, sizeof(obj_t *) * vm.gray_capacity);

        if (!vm.gray_stack)
            exit(1);
    }

    vm.gray_stack[vm.gray_count++] = obj;
}

/// @brief Marks stack-stored values for garbage collection.
/// @param value content stored in a certain stack slot
void mark_value(value_t value)
{
    // Some values are stored directly inline in `value_t` and require
    // no heap allocation.
    if (IS_OBJ(value))
        mark_object(AS_OBJ(value));
}

/// @brief Marks all the values stored in an array.
/// @param array value array
static void mark_array(value_array_t *array)
{
    for (int i = 0; i < array->count; i++)
        mark_value(array->values[i]);
}

/// @brief Marks all the given object's heap references.
/// @param obj root object
static void blacken_object(obj_t *obj)
{
#ifdef DEBUG_LOG_GC
    printf("%p blacken ", (void *)obj);
    print_value(OBJ_VAL(obj));
    printf("\n");
#endif
    // Marking an object as "black" is not a direct state change.
    // A black object is any object whose `is_marked` field is
    // set and that is no longer in the gray stack.
    switch (obj->type) {
        case OBJ_CLASS: {
            obj_class_t *class = (obj_class_t *)obj;
            mark_object((obj_t *)class->name);
            break;
        }
        // Each closure has a reference to the bare function it wraps,
        // as well as an array of pointers to the upvalues it captures.
        case OBJ_CLOSURE: {
            obj_closure_t *closure = (obj_closure_t *)obj;
            mark_object((obj_t *)closure->function);

            for (int i = 0; i < closure->upvalue_count; i++)
                mark_object((obj_t *)closure->upvalues[i]);
            
            break;
        }
        case OBJ_FUNC: {
            obj_func_t *func = (obj_func_t *)obj;
            // Deals with the string object containing the function's name.
            mark_object((obj_t *)func->name);
            // Marks the function's constant table values.
            mark_array(&func->chunk.constants);
            break;
        }
        // If an instance is alive, both its class and its instance
        // fields should be marked.
        case OBJ_INSTANCE: {
            obj_instance_t *instance = (obj_instance_t *)obj;
            mark_object((obj_t *)instance->class);
            mark_table(&instance->fields);
            break;
        }
        // Strings and native function objects contain no
        // outgoing references.
        case OBJ_NATIVE:
        case OBJ_STR:
            break;
        case OBJ_UPVALUE:
            // When an upvalue is closed, it contains a reference
            // to the closed-over value.
            mark_value(((obj_upvalue_t *)obj)->closed);
            break;
    }
}

/// @brief Frees the table's keys not marked for reachability.
/// @param table 
void table_remove_white(table_t *table)
{
    for (int i = 0; i < table->size; i++) {
        entry_t *entry = &table->entries[i];

        if (entry->key && !entry->key->obj.is_marked)
            table_delete(table, entry->key);
    }
}

/// @brief Marks all the positions from the given table.
/// @param table hash table
void mark_table(table_t *table)
{
    for (int i = 0; i < table->size; i++) {
        entry_t *entry = &table->entries[i];

        mark_object((obj_t *)entry->key);
        mark_value(entry->value);
    }
}

/// @brief Marks all allocations directly reachable by the program.
static void mark_roots()
{
    for (value_t *slot = vm.stack; slot < vm.stack_top; slot++)
        mark_value(*slot);
    // Each call frame contains a pointer to the cosure being
    // called. The vm uses those pointers to access constants
    // and upvalues, so closures should also be marked.
    for (int i = 0; i < vm.frame_count; i++)
        mark_object((obj_t *)vm.frames[i].closure);

    for (obj_upvalue_t *upvalue = vm.open_upvalues; upvalue; upvalue = upvalue->next)
        mark_object((obj_t *)upvalue);
        
    // Global variables are also a source of roots.
    mark_table(&vm.globals);

    mark_compiler_roots();
}

/// @brief Traverses the stack of grey objects and their references.
static void trace_references()
{
    while (vm.gray_count > 0) {
        obj_t *obj = vm.gray_stack[--vm.gray_count];
        blacken_object(obj);
    }
}

static void sweep()
{
    obj_t *prev = NULL;
    obj_t *obj = vm.objects;
    // Walks the linked list of every object in the heap,
    // stored in the virtual machine.
    while (obj) {
        // Marked objects are simply ignored.
        if (obj->is_marked) {
            // Every marked object must have their mark flag
            // unset for the next collection cycle.
            obj->is_marked = false;
            prev = obj;
            obj = obj->next;
        } else {
            // If an object is unmarked, it is unlinked
            // from the list.
            obj_t *white = obj;
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

void collect_garbage()
{
#ifdef DEBUG_LOG_GC
    printf("-- gc begin\n");
    // Captures the heap size before the collection is triggered.
    size_t before = vm.bytes_allocated;
#endif

    mark_roots();
    trace_references();
    table_remove_white(&vm.strings);
    sweep();

    vm.next_gc = vm.bytes_allocated * GC_HEAP_GROW_FACTOR;

#ifdef DEBUG_LOG_GC
    printf("-- gc end\n");
    // Logs how much memory the garbage collector reclaimed during
    // its execution. 
    printf("   collected %zu bytes (from %zu to %zu) next at %zu\n",
        before - vm.bytes_allocated, before, vm.bytes_allocated, vm.next_gc);
#endif
}
