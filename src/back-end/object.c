#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "back-end/object.h"
#include "back-end/value.h"
#include "back-end/vm.h"

#define ALLOCATE_OBJ(type, obj_type) \
    (type *)allocate_obj(sizeof(type), obj_type)

/// @brief Consumes the string literal, properly allocating it on the heap.
/// @param chars string constant
/// @param len string length
/// @return pointer to the object generated from that string
obj_str_t *copy_str(const char *chars, int len)
{
    char *heap_chars = ALLOCATE(char, len + 1);
    memcpy(heap_chars, chars, len);

    heap_chars[len] = '\0';

    return allocate_str(heap_chars, len);
}

/// @brief Creates a new string object and initializes its fields.
/// @param chars string content
/// @param len string length
/// @return pointer to the resulting string object
static obj_str_t *allocate_str(char *chars, int len)
{
    obj_str_t *str = ALLOCATE_OBJ(obj_str_t, OBJ_STR);
    str->length = len;
    str->chars = chars;
    
    return str;
}

/// @brief Allocates an object of a given size on the heap.
/// @param size specified number of bytes for varying extra fields
/// @param type type of the object being allocated
/// @return pointer to the allocated object memory
static obj_t *allocate_obj(size_t size, obj_type_t type)
{
    obj_t *obj = (obj_t *)reallocate(NULL, 0, size);
    obj->type = type;

    return obj;
}