#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "back-end/object.h"
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

/// @brief Creates a new string object and initializes its fields.
/// @param chars string content
/// @param len string length
/// @return pointer to the resulting string object
static obj_str_t *allocate_str(char *chars, int len)
{
    obj_str_t *str = ALLOCATE_STR(len);
    str->length = len;
    memcpy(str->chars, chars, len);
    
    return str;
}

/// @brief 
/// @param chars 
/// @param len 
/// @return 
obj_str_t *take_str(char *chars, int len)
{
    return allocate_str(chars, len);
}

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

/// @brief Prints a value representing an object.
/// @param value 
void print_obj(value_t value)
{
    switch (OBJ_TYPE(value)) {
        case OBJ_STR:
            printf("%s", AS_CSTR(value));
            break;
    }
}