#ifndef VALUE_H
#define VALUE_H

#include <string.h>

#include "common.h"

#ifdef NAN_BOXING

#define SIGN_BIT        ((uint64_t)0x8000000000000000)
#define QNAN            ((uint64_t)0x7ffc000000000000)

#define TAG_NIL         1 /* 01 */
#define TAG_FALSE       2 /* 10 */
#define TAG_TRUE        3 /* 11 */
#define FALSE_VAL       ((Value)(uint64_t)(QNAN | TAG_FALSE))
#define TRUE_VAL        ((Value)(uint64_t)(QNAN | TAG_TRUE))
#define NIL_VAL         ((Value)(uint64_t)(QNAN | TAG_NIL))

#define BOOL_VAL(val)   ((val) ? TRUE_VAL : FALSE_VAL)
#define NUM_VAL(val)    num_from_val(val)
#define OBJ_VAL(val)    (Value)(SIGN_BIT | QNAN | (uintptr_t)(val))

#define IS_BOOL(val)    (((val) | 1) == TRUE_VAL)
#define IS_NIL(val)     ((val) == NIL_VAL)
#define IS_NUM(val)     (((val) & QNAN) != QNAN)
#define IS_OBJ(val)     (((val) & (QNAN | SIGN_BIT)) == (QNAN | SIGN_BIT))

#define AS_BOOL(val)    ((val) == TRUE_VAL)
#define AS_NUM(val)     val_from_num(val)
#define AS_OBJ(val)     ((Obj*)(uintptr_t)((val) & ~(SIGN_BIT | QNAN)))

#else

#define BOOL_VAL(val)   ((Value){VAL_BOOL, {.boolean = val}})
#define NIL_VAL         ((Value){VAL_NIL, {.number = 0}})
#define NUM_VAL(val)    ((Value){VAL_NUM, {.number = val}})
#define OBJ_VAL(val)    ((Value){VAL_OBJ, {.obj = (Obj*)val}})

#define IS_BOOL(val)    ((val).type == VAL_BOOL)
#define IS_NIL(val)     ((val).type == VAL_NIL)
#define IS_NUM(val)     ((val).type == VAL_NUM)
#define IS_OBJ(val)     ((val).type == VAL_OBJ)

#define AS_BOOL(val)    ((val).as.boolean)
#define AS_NUM(val)     ((val).as.number)
#define AS_OBJ(val)     ((val).as.obj)

#endif

// Contains the state shared across all object types.
typedef struct Obj Obj;

// Each object type will have an `obj_t` as its first field. This is useful
// considering struct pointers can be converted to point to its first field,
// so any piece of code that intends to manipulate all objects can treat
// them as `obj_t *` and ignore other following fields.
typedef struct ObjStr ObjStr;

#ifdef NAN_BOXING

typedef uint64_t Value;

#else

// All types supported in the language.
typedef enum
{
    VAL_BOOL,
    VAL_NIL,
    VAL_NUM,
    VAL_OBJ
} ValueType;

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

#endif

//  List of values that appear as literals in the program.
typedef struct
{
    int     capacity;
    int     count;
    Value*  values;
} ValueArray;

#ifdef NAN_BOXING

static inline Value num_from_val(double val)
{
    Value value;
    memcpy(&value, &val, sizeof(double));
    return value;
}

static inline double val_from_num(Value val)
{
    double num;
    memcpy(&num, &val, sizeof(Value));
    return num;
}

#endif

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