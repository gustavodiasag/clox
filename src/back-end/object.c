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
/// @return pointer to the allocated object memory
static obj_t *allocate_obj(size_t size, obj_type_t type)
{
    obj_t *obj = (obj_t *)reallocate(NULL, 0, size);
    obj->type = type;
    // New object is inserted at the head of the linked list.
    obj->next = vm.objects;
    vm.objects = obj;

    return obj;
}

/// @brief Creates a new Lox function.
/// @return pointer to the new function object
obj_func_t *new_func()
{
    obj_func_t *func = ALLOCATE_OBJ(obj_func_t, OBJ_FUNC);
    func->arity = 0;
    func->name = NULL;
    init_chunk(&func->chunk);

    return func;
}

/// @brief Creates an object representing a native function.
/// @param function given native
/// @return pointer to the allocated function
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
/// @return pointer to the resulting string object
static obj_str_t *allocate_str(char *chars, int len, uint32_t hash)
{
    obj_str_t *str = ALLOCATE_STR(len);
    str->hash = hash;
    str->length = len;
    memcpy(str->chars, chars, len);
    // Whenever a new string is created, it's added to the virtual machine's table.
    table_set(&vm.strings, str, NIL_VAL);
    
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
    printf("<fn %s>", func->name->chars);
}

/// @brief Prints a value representing an object.
/// @param value 
void print_obj(value_t value)
{
    switch (OBJ_TYPE(value)) {
        case OBJ_FUNC:
            print_func(AS_FUNC(value));
            break;
        case OBJ_NATIVE:
            printf("<native fn>");
            break;
        case OBJ_STR:
            printf("%s", AS_CSTR(value));
            break;
    }
}