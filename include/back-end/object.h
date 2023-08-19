#pragma once

#include "common.h"
#include "chunk.h"
#include "value.h"

#define OBJ_TYPE(val)   (AS_OBJ(val)->type)

#define IS_CLASS(val)   is_obj_type(val, OBJ_CLASS)
#define IS_CLOSURE(val) is_obj_type(val, OBJ_CLOSURE)
#define IS_FUNC(val)    is_obj_type(val, OBJ_FUNC)
#define IS_NATIVE(val)  is_obj_type(val, OBJ_NATIVE)
#define IS_STR(val)     is_obj_type(val, OBJ_STR)

#define AS_CLASS(val)   ((obj_class_t *)AS_OBJ(val))
#define AS_CLOSURE(val) ((obj_closure_t *)AS_OBJ(val))
#define AS_FUNC(val)    ((obj_func_t *)AS_OBJ(val))
#define AS_NATIVE(val)  (((obj_native_t *)AS_OBJ(val))->function)
#define AS_STR(val)     ((obj_str_t *)AS_OBJ(val))
#define AS_CSTR(val)    (((obj_str_t *)AS_OBJ(val))->chars)

// All heap-allocated components supported in the language.
typedef enum {
    OBJ_CLASS,
    OBJ_CLOSURE,
    OBJ_FUNC,
    OBJ_NATIVE,
    OBJ_STR,
    OBJ_UPVALUE
} obj_type_t;

struct obj_t{
    // Identifies what kind of object it is.
    obj_type_t type;
    // Flag determining that the object is reachable in memory.
    bool is_marked;
    struct obj_t *next;
};

// Since functions are first class in Lox, they should be
// represented as objects.
typedef struct {
    obj_t obj;
    // Stores the number of parameters the function expects.
    int arity;
    // Number of upvalues accessed by the function.
    int upvalue_count;
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

// Runtime representation of upvalues.
typedef struct obj_upvalue_t {
    obj_t obj;
    // Pointer to a closed-over variable.
    value_t *location;
    // Because an object is already stored in the heap, when an
    // upvalue is closed, the variable representation becomes
    // part of the upvalue object.
    value_t closed;
    // The linked-list semantic is used so that the virtual machine
    // is able to store its own list of upvalues that point to
    // variables still on the stack.
    struct obj_upvalue_t *next;
} obj_upvalue_t;

// Wrapper around a function object that represents its declaration
// at runtime. Functions can have references to variables declared
// in their bodies and also capture outer-scoped variables, so
// their behavior is similar to that of a closure.
typedef struct {
    obj_t obj;
    obj_func_t *function;
    // Pointer to a dynamically allocated array of pointers to
    // upvalues.
    obj_upvalue_t **upvalues;
    // Number of elements in the upvalue array.
    int upvalue_count;
} obj_closure_t;

// Runtime representation of classes.
typedef struct {
    obj_t obj;
    // Class' name.
    obj_str_t *name;
} obj_class_t;

obj_class_t *new_class(obj_str_t *name);
obj_upvalue_t *new_upvalue(value_t *slot);
obj_closure_t *new_closure(obj_func_t *function);
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