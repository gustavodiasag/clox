#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "back-end/object.h"
#include "back-end/table.h"
#include "back-end/value.h"
#include "back-end/vm.h"

#define ALLOCATE_OBJ(type, obj_type) \
    (type *)allocate_obj(sizeof(type), obj_type)

#define ALLOCATE_STR(len) \
    (obj_str_t *)allocate_obj(sizeof(obj_str_t) + sizeof(char[len]), OBJ_STR)

/// @brief Allocates an object of a given size on the heap.
/// @param size specified number of bytes for varying extra fields
/// @param type type of the object being allocated
/// @return pointer to the object created
static obj_t *allocate_obj(size_t size, obj_type_t type)
{
    obj_t *obj = (obj_t *)reallocate(NULL, 0, size);
    obj->type = type;
    // Every new object begins unmarked because it hasn't been
    // determined if it is reachable or not.
    obj->is_marked = false;
    // New object is inserted at the head of the linked list.
    obj->next = vm.objects;
    vm.objects = obj;

#ifdef DEBUG_LOG_GC
    printf("%p allocate %zu for %d\n", (void *)obj, size, type);
#endif

    return obj;
}

/// @brief Creates a new class object
/// @param name class' name
/// @return pointer to the object created
obj_class_t *new_class(obj_str_t *name)
{
    obj_class_t *class = ALLOCATE_OBJ(obj_class_t, OBJ_CLASS);
    class->name = name;

    return class;
}

/// @brief Creates a new upvalue object
/// @param slot stack position of the captured variable
/// @return pointer to the object created
obj_upvalue_t *new_upvalue(value_t *slot) {
    obj_upvalue_t *upvalue = ALLOCATE_OBJ(obj_upvalue_t, OBJ_UPVALUE);
    upvalue->closed = NIL_VAL;
    upvalue->location = slot;
    upvalue->next = NULL;

    return upvalue;
}

/// @brief Creates a new closure object.
/// @param function 
/// @return pointer to the object created
obj_closure_t *new_closure(obj_func_t *function)
{
    obj_upvalue_t **upvalues = ALLOCATE(obj_upvalue_t*, function->upvalue_count);
    
    for (int i = 0; i < function->upvalue_count; i++)
        upvalues[i] = NULL;

    obj_closure_t *closure = ALLOCATE_OBJ(obj_closure_t, OBJ_CLOSURE);
    closure->function = function;
    closure->upvalues = upvalues;
    closure->upvalue_count = function->upvalue_count;

    return closure;
}

/// @brief Creates a new Lox function.
/// @return pointer to the object created
obj_func_t *new_func()
{
    obj_func_t *func = ALLOCATE_OBJ(obj_func_t, OBJ_FUNC);
    func->upvalue_count = 0;
    func->arity = 0;
    func->name = NULL;

    init_chunk(&func->chunk);

    return func;
}

/// @brief Creates an object representing a new instance
/// @param class instance's class
/// @return pointer to the object created
obj_instance_t *new_instance(obj_class_t *class)
{
    obj_instance_t *instance = ALLOCATE_OBJ(obj_instance_t, OBJ_INSTANCE);
    instance->class = class;

    init_table(&instance->fields);

    return instance;
}

/// @brief Creates an object representing a native function.
/// @param function given native
/// @return pointer to the object created
obj_native_t *new_native(native_fn_t function)
{
    obj_native_t *native = ALLOCATE_OBJ(obj_native_t, OBJ_NATIVE);
    native->function = function;

    return native;
}

/// @brief Creates a new string object and initializes its fields.
/// @param chars string content
/// @param len string length
/// @param hash string's hash code
/// @return pointer to the object created
static obj_str_t *allocate_str(char *chars, int len, uint32_t hash)
{
    obj_str_t *str = ALLOCATE_STR(len);
    str->hash = hash;
    str->length = len;
    memcpy(str->chars, chars, len);
    // Object is pushed onto the stack to guarantee it is reachable in case
    // garbage collection is triggered while resizing the interned strings
    // table.
    push(OBJ_VAL(str));
    // Whenever a new string is created, it's added to the virtual machine's table.
    table_set(&vm.strings, str, NIL_VAL);
    pop();
    
    return str;
}

/// @brief Short version of the FNV-1a hashing algorithm.
/// @param key string to be hashed
/// @param len string length
/// @return hash value for that specific key
static uint32_t hash_str(const char *key, int len)
{
    uint32_t hash = 2166136261u;

    for (int i = 0; i < len; i++) {
        hash ^= (uint8_t)key[i];
        hash *= 16777619;
    }

    return hash;
}

/// @brief Takes ownership of the specified string.
/// @param chars string content
/// @param len string length
/// @return pointer to the new objcet containing the string
obj_str_t *take_str(char *chars, int len)
{
    uint32_t hash = hash_str(chars, len);
    obj_str_t *interned = table_find(&vm.strings, chars, len, hash);

    if (interned) {
        // Duplicate string is no longer needed.
        FREE_ARRAY(char, chars, len + 1);

        return interned;
    }

    return allocate_str(chars, len, hash);
}

/// @brief Consumes the string literal, properly allocating it on the heap.
/// @param chars string literal
/// @param len string length
/// @return pointer to the object generated from that string
obj_str_t *copy_str(const char *chars, int len)
{
    uint32_t hash = hash_str(chars, len);

    obj_str_t *interned = table_find(&vm.strings, chars, len, hash);

    if (interned)
        return interned;

    char *heap_chars = ALLOCATE(char, len + 1);
    memcpy(heap_chars, chars, len);

    heap_chars[len] = '\0';

    return allocate_str(heap_chars, len, hash);
}

/// @brief Prints the name of the given function object.
/// @param func function object
static void print_func(obj_func_t *func)
{
    if (!func->name) {
        printf("<script>");
        return;
    }
    printf(" <fn %s>", func->name->chars);
}

/// @brief Prints a value representing an object.
/// @param value contains the object type
void print_obj(value_t value)
{
    switch (OBJ_TYPE(value)) {
        case OBJ_CLASS:
            printf("%s", AS_CLASS(value)->name->chars);
            break;
        case OBJ_CLOSURE:
            print_func(AS_CLOSURE(value)->function);
            break;
        case OBJ_FUNC:
            print_func(AS_FUNC(value));
            break;
        case OBJ_INSTANCE: 
            printf("%s instance", AS_INSTANCE(value)->class->name->chars);
        case OBJ_NATIVE:
            printf("<native fn>");
            break;
        case OBJ_STR:
            printf("%s", AS_CSTR(value));
            break;
        case OBJ_UPVALUE:
            printf("upvalue");
            break;
    }
}