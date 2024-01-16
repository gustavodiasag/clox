#ifndef VALUE_H
#define VALUE_H

#include <string.h>

#include "common.h"

/* Object forward declaration. */
typedef struct Obj Obj;
/* String object forward declaration. */
typedef struct ObjStr ObjStr;

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

#ifdef NAN_BOXING

/**
 * With NaN-boxing, the language's types are represented with a 64-bit double.
 * 
 * Numeric values, which are floating point numbers, directly use the type,
 * still having to be type checked at runtime. Object, boolean and nil values
 * use quiet NaNs by having the exponent bits and highest mantissa bit set.
 * 
 * Nil, false and true are defined with the sign bit unset and the byte values
 * of 1, 2 and 3, respectively. Objects have the sign bit set and their pointer
 * stored in the lower 48 mantissa bits.
 */
typedef uint64_t Value;

#else

/** Represents the types supported by the language */
typedef enum
{
    VAL_BOOL,
    VAL_NIL,
    VAL_NUM,
    VAL_OBJ
} ValueType;

/**
 * Structure representing the language's dynamic type information.
 * 
 * `type` is the type of the value.
 * `as` is the possible values that can be stored. 
 */
typedef struct
{
    ValueType   type;
    union
    {
        bool    boolean;
        double  number;
        Obj*    obj;
    } as;
} Value;

#endif

#ifdef NAN_BOXING

/**
 * Converts a double-precision, floating point number specified by `val` to a
 * NaN-boxed value of the language.
 */
static inline Value num_from_val(double val)
{
    Value value;
    memcpy(&value, &val, sizeof(double));
    return value;
}

/**
 * Converts a NaN-boxed value of the language specified by `val` to a 
 * double-precision, floating point number.
 */
static inline double val_from_num(Value val)
{
    double num;
    memcpy(&num, &val, sizeof(Value));
    return num;
}

#endif

/** 
 * Represents a dynamic array of values.
 * 
 * `count` is the current amount of values in the vector.
 * `capacity` is the size of the vector.
 * `values` is the value array.
 */
typedef struct
{
    int     count;
    int     capacity;
    Value*  values;
} ValueArray;

/**
 * Initializes a value vector specified by `array`, not performing any
 * allocation.
 */
void init_value_array(ValueArray* array);

/** Inserts a value specified by `value` to a vector specified by `array`. */
void write_value_array(ValueArray* array, Value value);

/** Frees and resets the memory of a value vector specified by `array`. */
void free_value_array(ValueArray* array);

/** Pretty prints a value specified by `value`. */
void print_value(Value value);

/** Checks for equality between two values, specified by `a` and `b`. */
bool values_equal(Value a, Value b);

#endif