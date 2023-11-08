#ifndef VALUE_H
#define VALUE_H

#include "common.h"

#define BOOL_VAL(val)   ((Value){VAL_BOOL, {.boolean = val}})

#define NIL_VAL         ((Value){VAL_NIL, {.number = 0}})

#define NUM_VAL(val)    ((Value){VAL_NUM, {.number = val}})

#define OBJ_VAL(object) ((Value){VAL_OBJ, {.obj = (Obj*)object}})

#define IS_BOOL(val)    ((val).type == VAL_BOOL)

#define IS_NIL(val)     ((val).type == VAL_NIL)

#define IS_NUM(val)     ((val).type == VAL_NUM)

#define IS_OBJ(val)     ((val).type == VAL_OBJ)

#define AS_BOOL(val)    ((val).as.boolean)

#define AS_NUM(val)     ((val).as.number)

#define AS_OBJ(val)     ((val).as.obj)

// All types supported in the language.
typedef enum {
    VAL_BOOL,
    VAL_NIL,
    VAL_NUM,
    VAL_OBJ
} ValueType;

// Contains the state shared across all object types.
typedef struct Obj Obj;

// Each object type will have an `obj_t` as its first field. This is useful
// considering struct pointers can be converted to point to its first field,
// so any piece of code that intends to manipulate all objects can treat
// them as `obj_t *` and ignore other following fields.
typedef struct ObjStr ObjStr;

// Tagged union representing a type and its correspondent value.
typedef struct
{
    ValueType   type;
    union
    {
        bool    boolean;
        double  number;
        // A value whose state lives on the heap memory.
        Obj*    obj;
    } as;
} Value;

//  List of values that appear as literals in the program.
typedef struct
{
    int     capacity;
    int     count;
    Value*  values;
} ValueArray;

// TODO: Description.
void init_value_array(ValueArray* array);

// TODO: Description.
void write_value_array(ValueArray* array, Value value);

// TODO: Description.
void free_value_array(ValueArray* array);

// TODO: Description.
void print_value(Value value);

/// @brief Compares the equality between two values, allowing multiple types.
/// @param a first value
/// @param b second value
/// @return whether the values are different or not
bool values_equal(Value a, Value b);

#endif