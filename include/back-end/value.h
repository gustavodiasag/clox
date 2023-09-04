#pragma once

#include "common.h"

#define BOOL_VAL(val) ((value_t){VAL_BOOL, {.boolean = val}})
#define NIL_VAL ((value_t){VAL_NIL, {.number = 0}})
#define NUM_VAL(val) ((value_t){VAL_NUM, {.number = val}})
#define OBJ_VAL(object) ((value_t){VAL_OBJ, {.obj = (obj_t*)object}})

#define IS_BOOL(val) ((val).type == VAL_BOOL)
#define IS_NIL(val) ((val).type == VAL_NIL)
#define IS_NUM(val) ((val).type == VAL_NUM)
#define IS_OBJ(val) ((val).type == VAL_OBJ)

#define AS_BOOL(val) ((val).as.boolean)
#define AS_NUM(val) ((val).as.number)
#define AS_OBJ(val) ((val).as.obj)

// All types supported in the language.
typedef enum {
    VAL_BOOL,
    VAL_NIL,
    VAL_NUM,
    VAL_OBJ
} value_type_t;

// Contains the state shared across all object types.
typedef struct obj_t obj_t;

// Each object type will have an `obj_t` as its first field. This is useful
// considering struct pointers can be converted to point to its first field,
// so any piece of code that intends to manipulate all objects can treat
// them as `obj_t *` and ignore other following fields.
typedef struct obj_str_t obj_str_t;

// Tagged union representing a type and its correspondent value.
typedef struct {
    value_type_t type;
    union {
        bool boolean;
        double number;
        // A value whose state lives on the heap memory.
        obj_t* obj;
    } as;
} value_t;

//  List of values that appear as literals in the program.
typedef struct {
    int capacity;
    int count;
    value_t* values;
} value_array_t;

void init_value_array(value_array_t* array);
void write_value_array(value_array_t* array, value_t value);
void free_value_array(value_array_t* array);
void print_value(value_t value);
bool values_equal(value_t a, value_t b);