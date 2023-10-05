#include <stdio.h>
#include <string.h>

#include "back-end/object.h"
#include "back-end/value.h"
#include "memory.h"

void init_value_array(ValueArray* array)
{
    array->values = NULL;
    array->capacity = 0;
    array->count = 0;
}

void write_value_array(ValueArray* array, Value value)
{
    if (array->capacity < array->count + 1) {
        int old_capacity = array->capacity;
        array->capacity = GROW_CAPACITY(old_capacity);
        array->values = GROW_ARRAY(Value, array->values, old_capacity, array->capacity);
    }

    array->values[array->count] = value;
    array->count++;
}

void free_value_array(ValueArray* array)
{
    FREE_ARRAY(Value, array->values, array->capacity);
    init_value_array(array);
}

void print_value(Value value)
{
    switch (value.type) {
    case VAL_BOOL:
        printf(AS_BOOL(value) ? "true" : "false");
        break;
    case VAL_NIL:
        printf("nil");
        break;
    case VAL_NUM:
        printf("%g", AS_NUM(value));
        break;
    case VAL_OBJ:
        print_obj(value);
        break;
    }
}

bool values_equal(Value a, Value b)
{
    if (a.type != b.type)
        return false;

    switch (a.type) {
    case VAL_BOOL:
        return AS_BOOL(a) == AS_BOOL(b);
    case VAL_NIL:
        return true;
    case VAL_NUM:
        return AS_NUM(a) == AS_NUM(b);
    case VAL_OBJ:
        return AS_OBJ(a) == AS_OBJ(b);
    default:
        return false;
    }
}