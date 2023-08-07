#pragma once

#include "common.h"
#include "chunk.h"
#include "value.h"

#define OBJ_TYPE(val)   (AS_OBJ(val)->type)

#define IS_FUNC(val)    is_obj_type(val, OBJ_FUNC)
#define IS_NATIVE(val)  is_obj_type(val, OBJ_NATIVE)
#define IS_STR(val)     is_obj_type(val, OBJ_STR)

#define AS_FUNC(val)    ((obj_func_t *)AS_OBJ(val))
#define AS_NATIVE(val)  (((obj_native_t *)AS_OBJ(val))->function)
#define AS_STR(val)     ((obj_str_t *)AS_OBJ(val))
#define AS_CSTR(val)    (((obj_str_t *)AS_OBJ(val))->chars)

// All heap-allocated components supported in the language.
typedef enum {
    OBJ_FUNC,
    OBJ_NATIVE,
    OBJ_STR
} obj_type_t;

struct obj_t{
    // Identifies what kind of object it is.
    obj_type_t type;
    struct obj_t *next;
};

// Since functions are first class in Lox, they should be
// represented as objects.
typedef struct {
    obj_t obj;
    // Stores the number of parameters the function expects.
    int arity;
    // Each function contains its own chunk of bytecode.
    chunk_t chunk;
    // Function name.   
    obj_str_t *name;
} obj_func_t;

typedef value_t (*native_fn_t)(int argc, value_t *argv);

// Given that native functions behave in a different way when
// compared to the language's functions, they are defined as
// an entirely different object type.
typedef struct {
    obj_t obj;
    // Pointer to the C function that implements the native behavior.
    native_fn_t function;
} obj_native_t;

struct obj_str_t {
    obj_t obj;
    // Used to avoid traversing the string.
    int length;
    // Each string object stores a hash code for its content.
    uint32_t hash;
    // Flexible array member, stores the object and its
    // character array in a single contiguous allocation.
    char chars[];
};

obj_func_t *new_func();
obj_native_t *new_native(native_fn_t function);
obj_str_t *take_str(char *chars, int len);
obj_str_t *copy_str(const char * chars, int len);
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