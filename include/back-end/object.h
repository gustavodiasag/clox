#pragma once

#include "common.h"
#include "value.h"

#define OBJ_TYPE(val)   (AS_OBJ(val)->type)

#define IS_STR(val)     is_obj_type(val, OBJ_STR)

#define AS_STR(val)     ((obj_str_t *)AS_OBJ(val))
#define AS_CSTR(val)    (((obj_str_t *)AS_OBJ(val))->chars)

typedef enum {
    OBJ_STR
} obj_type_t;

struct obj_t{
    // Identifies what kind of object it is.
    obj_type_t type;
};

struct obj_str_t {
    obj_t obj;
    // Used to avoid traversing the string.
    int length;
    char *chars;
};

obj_str_t *copy_str(const char * chars, int length);
void print_obj(value_t value);

/// @brief Tells when is safe to cast a value to a specific object type.
/// @param value 
/// @param type object expected
/// @return whether the value is an object of the specified type
static inline bool is_obj_type(value_t value, obj_type_t type)
{
    // An inline function is used instead of a macro because the value gets
    // accessed twice. If that expression has side effects, once the macro
    // is expanded, the evaluation can lead to incorrect behaviour.
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}