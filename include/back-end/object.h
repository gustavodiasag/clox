#ifndef OBJECT_H
#define OBJECT_H

#include "chunk.h"
#include "common.h"
#include "table.h"
#include "value.h"

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

// All heap-allocated components in the language.
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
    ObjType type;
    // Flag determining that the object is reachable in memory.
    bool is_marked;
    struct Obj* next;
};

// Since functions are first class in Lox, they should be
// represented as objects.
typedef struct {
    Obj obj;
    // Stores the number of parameters the function expects.
    int arity;
    // Number of upvalues accessed by the function.
    int upvalue_count;
    // Each function contains its own chunk of bytecode.
    Chunk chunk;
    // Function name.
    ObjStr* name;
} ObjFun;

typedef Value (*NativeFun)(int argc, Value* argv);

// Given that native functions behave in a different way when
// compared to the language's functions, they are defined as
// an entirely different object type.
typedef struct {
    Obj obj;
    // Pointer to the C function that implements the native behavior.
    NativeFun function;
} ObjNative;

struct ObjStr {
    Obj obj;
    // Used to avoid traversing the string.
    int length;
    // Each string object stores a hash code for its content.
    uint32_t hash;
    // Flexible array member. Stores the object and its
    // character array in a single contiguous allocation.
    char chars[];
};

// Runtime representation of upvalues.
typedef struct ObjUpvalue {
    Obj obj;
    // Pointer to a closed-over variable.
    Value* location;
    // Because an object is already stored in the heap, when an
    // upvalue is closed, the variable representation becomes
    // part of the upvalue object.
    Value closed;
    // The linked-list semantic is used so that the virtual machine
    // is able to store its own list of upvalues that point to
    // variables still on the stack.
    struct ObjUpvalue* next;
} ObjUpvalue;

// Wrapper around a function object that represents its declaration
// at runtime. Functions can have references to variables declared
// in their bodies and also capture outer-scoped variables, so
// their behavior is similar to that of a closure.
typedef struct {
    Obj obj;
    ObjFun* function;
    // Pointer to a dynamically allocated array of pointers to
    // upvalues.
    ObjUpvalue** upvalues;
    // Number of elements in the upvalue array.
    int upvalue_count;
} ObjClosure;

// Runtime representation of classes.
typedef struct {
    Obj obj;
    // Class' name.
    ObjStr* name;
    // Each class stores a hash table of methods. Keys are
    // method names and each value is a closure object.
    Table methods;
} ObjClass;

// Represents an instance of some class.
typedef struct {
    Obj obj;
    // Pointer to the class that the instance represents.
    ObjClass* class;
    // Instances' state storage.
    Table fields;
} ObjInst;

// Wraps some receiver and the method closure together.
typedef struct {
    Obj obj;
    Value receiver;
    ObjClosure* method;
} ObjBoundMethod;

/// @brief Creates a new bounded method object.
/// @param receiver variable receiving the method
/// @param method method clojure
/// @return pointer to the object created
ObjBoundMethod* new_bound_method(Value receiver, ObjClosure* method);

/// @brief Creates a new class object
/// @param name class' name
/// @return pointer to the object created
ObjClass* new_class(ObjStr* name);

/// @brief Creates a new upvalue object
/// @param slot stack position of the captured variable
/// @return pointer to the object created
ObjUpvalue* new_upvalue(Value* slot);

/// @brief Creates a new closure object.
/// @param function
/// @return pointer to the object created
ObjClosure* new_closure(ObjFun* function);

/// @brief Creates a new Lox function.
/// @return pointer to the object created
ObjFun* new_func();

/// @brief Creates an object representing a new instance
/// @param class instance's class
/// @return pointer to the object created
ObjInst* new_instance(ObjClass* class);

/// @brief Creates an object representing a native function.
/// @param function given native
/// @return pointer to the object created
ObjNative* new_native(NativeFun function);

/// @brief Takes ownership of the specified string.
/// @param chars string content
/// @param len string length
/// @return pointer to the new objcet containing the string
ObjStr* take_str(char* chars, int len);

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