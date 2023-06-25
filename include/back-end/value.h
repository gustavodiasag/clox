#pragma once

#include "common.h"

#define BOOL_VAL(value) ((value_t){VAL_BOOL, {.boolean = value}})
#define NIL_VAL         ((value_t){VAL_NIL, {.number = 0}})
#define NUM_VAL(value)  ((value_t){VAL_NUM, {.number = value}})

#define IS_BOOL(value)  ((value).type == VAL_BOOL)
#define IS_NIL(value)   ((value).type == VAL_NIL)
#define IS_NUM(value)   ((value).type == VAL_NUM)

#define AS_BOOL(value)  ((value).as.boolean)
#define AS_NUM(value)   ((value).as.number)

typedef enum {
    VAL_BOOL,
    VAL_NIL,
    VAL_NUM
} value_type_t;

typedef struct {
    value_type_t type;
    union {
        bool boolean;
        double number;
    } as;
} value_t; // FIXME: Currently, each instance takes 16 bytes of memory.

//  List of values that appear as literals in the program.
typedef struct {
    int capacity;
    int count;
    value_t *values;
} value_array_t;


void init_value_array(value_array_t *array);
void write_value_array(value_array_t *array, value_t value);
void free_value_array(value_array_t *array);
void print_value(value_t value);