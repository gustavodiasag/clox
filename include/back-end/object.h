#ifndef OBJECT_H
#define OBJECT_H

#include "chunk.h"
#include "common.h"
#include "table.h"
#include "value.h"

/** Heap-allocated objects supported by the language. */
typedef enum {
    OBJ_BOUND_METHOD,
    OBJ_CLASS,
    OBJ_CLOSURE,
    OBJ_FUNC,
    OBJ_INSTANCE,
    OBJ_NATIVE,
    OBJ_STR,
    OBJ_UPVALUE
} ObjType;

struct Obj {
    // Identifies what kind of object it is.
    ObjType     type;
    // Flag determining whether the object is reachable in memory or not.
    bool        is_marked;
    struct Obj* next;
};

// Since functions are first class in Lox, they should be
// represented as objects.
typedef struct _ObjFun {
    Obj     obj;
    // Stores the number of parameters the function expects.
    int     arity;
    // Number of upvalues accessed by the function.
    int     upvalue_count;
    // Each function contains its own chunk of bytecode.
    Chunk   chunk;
    // Function's name.
    ObjStr* name;
} ObjFun;

typedef Value (*NativeFun)(int argc, Value* argv);

// Given that native functions behave in a different way when
// compared to the language's functions, they are defined as
// an entirely different object type.
typedef struct _ObjNative {
    Obj         obj;
    // Pointer to the C function that implements the native behavior.
    NativeFun   function;
} ObjNative;

struct ObjStr {
    Obj         obj;
    // Used to avoid traversing the string.
    int         length;
    // Each string object stores a hash code for its content.
    uint32_t    hash;
    // Flexible array member. Stores the object and its
    // character array in a single contiguous allocation.
    char        chars[];
};

// Runtime representation of upvalues.
typedef struct _ObjUpvalue {
    Obj                 obj;
    // Pointer to a closed-over variable.
    Value*              location;
    // Because an object is already stored in the heap, when an
    // upvalue is closed, the variable representation becomes
    // part of the upvalue object.
    Value               closed;
    // The linked-list semantic is used so that the virtual machine
    // is able to store its own list of upvalues that point to
    // variables still on the stack.
    struct _ObjUpvalue* next;
} ObjUpvalue;

// Wrapper around a function object that represents its declaration
// at runtime. Functions can have references to variables declared
// in their bodies and also capture outer-scoped variables, so
// their behavior is similar to that of a closure.
typedef struct _ObjClosure{
    Obj             obj;
    ObjFun*         function;
    // Pointer to a dynamically allocated array of pointers to
    // upvalues.
    ObjUpvalue**    upvalues;
    // Number of elements in the upvalue array.
    int             upvalue_count;
} ObjClosure;

// Runtime representation of classes.
typedef struct _ObjClass {
    Obj     obj;
    // Class' name.
    ObjStr* name;
    // Each class stores a hash table of methods. Keys are
    // method names and each value is a closure object.
    Table   methods;
} ObjClass;

// Represents an instance of a class.
typedef struct _ObjInst {
    Obj         obj;
    // Pointer to the class that the instance represents.
    ObjClass*   class;
    // Instances' state storage.
    Table       fields;
} ObjInst;

// Wraps some receiver and the method closure together.
typedef struct _ObjBoundMethod {
    Obj             obj;
    Value           receiver;
    ObjClosure*     method;
} ObjBoundMethod;

#define OBJ_TYPE(val) (AS_OBJ(val)->type)

#define IS_BOUND_METHOD(val) is_obj_type(val, OBJ_BOUND_METHOD)

#define IS_CLASS(val) is_obj_type(val, OBJ_CLASS)

#define IS_CLOSURE(val) is_obj_type(val, OBJ_CLOSURE)

#define IS_FUNC(val) is_obj_type(val, OBJ_FUNC)

#define IS_INSTANCE(val) is_obj_type(val, OBJ_INSTANCE)

#define IS_NATIVE(val) is_obj_type(val, OBJ_NATIVE)

#define IS_STR(val) is_obj_type(val, OBJ_STR)

#define AS_BOUND_METHOD(val) ((ObjBoundMethod*)AS_OBJ(val))

#define AS_CLASS(val) ((ObjClass*)AS_OBJ(val))

#define AS_CLOSURE(val) ((ObjClosure*)AS_OBJ(val))

#define AS_FUNC(val) ((ObjFun*)AS_OBJ(val))

#define AS_INSTANCE(val) ((ObjInst*)AS_OBJ(val))

#define AS_NATIVE(val) (((ObjNative*)AS_OBJ(val))->function)

#define AS_STR(val) ((ObjStr*)AS_OBJ(val))

#define AS_CSTR(val) (((ObjStr*)AS_OBJ(val))->chars)

/**
 * ALlocates and initializes a bound from a method specified by a closure
 * `method` to an object specified by `receiver`.
 * 
 * Returns a pointer to the new bound method object created.
 */
ObjBoundMethod* new_bound_method(Value receiver, ObjClosure* method);

/**
 * ALlocates and initializes a class with the name specified by `name`.
 * 
 * Returns a pointer to the new class object created.
 */
ObjClass* new_class(ObjStr* name);

/**
 * ALlocates and initializes an upvalue from a variable given its stack
 * position speficied by `slot`.
 * 
 * Returns a pointer to the new upvalue object created.
 */
ObjUpvalue* new_upvalue(Value* slot);

/**
 * ALlocates and initializes a closure from a function specified by `fun`.
 * 
 * Returns a pointer to the new closure object created.
 */
ObjClosure* new_closure(ObjFun* fun);

/**
 * Allocates and initializes a function.
 * 
 * Returns a pointer to the new function created.
 */
ObjFun* new_func();

/**
 * Allocates and initializes an object representing a new instance of a class
 * specified by `class`.
 * 
 * Returns a pointer to the new instance created.
 */
ObjInst* new_instance(ObjClass* class);

/**
 * Allocates and initializes a native function with a signature specified by
 * `fun`.
 * 
 * Returns a pointer to the new native function created.
 */
ObjNative* new_native(NativeFun fun);

/**
 * Takes ownership of a stream of characters specified by `data` with a length
 * specified by `len` by converting it to an object directly manipulated by the
 * language's virtual machine.
 * 
 * Returns a pointer to the new string object created.
 */
ObjStr* take_str(char* data, int len);

/// @brief Consumes the string literal, properly allocating it on the heap.
/// @param chars string literal
/// @param len string length
/// @return pointer to the object generated from that string
ObjStr* copy_str(const char* chars, int len);

/// @brief Prints a value representing an object.
/// @param value contains the object type
void print_obj(Value value);

/// @brief Tells when is safe to cast a value to a specific object type.
/// @param value
/// @param type object expected
/// @return whether the value is an object of the specified type
static inline bool is_obj_type(Value value, ObjType type)
{
    // An inline function is used instead of a macro because the value gets
    // accessed twice. If that expression has side effects, once the macro
    // is expanded, the evaluation can lead to incorrect behaviour.
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#endif