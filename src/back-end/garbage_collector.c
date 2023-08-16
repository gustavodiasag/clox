#include "back-end/garbage_collector.h"
#include "common.h"
#include "front-end/compiler.h"

#ifdef DEBUG_LOG_GC

#include <stdio.h>
#include "debug.h"

#endif

/// @brief Marks a heap-stored value from the language
/// @param obj object referenced by the value 
void mark_object(obj_t *obj)
{
    if (!obj)
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

void collect_garbage()
{
#ifdef DEBUG_LOG_GC
    printf("--gc begin\n");
#endif

    mark_roots();

#ifdef DEBUG_LOG_GC
    printf("-- gc end\n");
#endif
}