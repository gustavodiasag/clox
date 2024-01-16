#ifndef OBJECT_H
#define OBJECT_H

#include "chunk.h"
#include "common.h"
#include "table.h"
#include "value.h"

#define OBJ_TYPE(val)           (AS_OBJ(val)->type)

#define IS_BOUND_METHOD(val)    is_obj_type(val, OBJ_BOUND_METHOD)
#define IS_CLASS(val)           is_obj_type(val, OBJ_CLASS)
#define IS_CLOSURE(val)         is_obj_type(val, OBJ_CLOSURE)
#define IS_FUNC(val)            is_obj_type(val, OBJ_FUNC)
#define IS_INSTANCE(val)        is_obj_type(val, OBJ_INSTANCE)
#define IS_NATIVE(val)          is_obj_type(val, OBJ_NATIVE)
#define IS_STR(val)             is_obj_type(val, OBJ_STR)

#define AS_BOUND_METHOD(val)    ((ObjBoundMethod*)AS_OBJ(val))
#define AS_CLASS(val)           ((ObjClass*)AS_OBJ(val))
#define AS_CLOSURE(val)         ((ObjClosure*)AS_OBJ(val))
#define AS_FUNC(val)            ((ObjFun*)AS_OBJ(val))
#define AS_INSTANCE(val)        ((ObjInst*)AS_OBJ(val))
#define AS_NATIVE(val)          (((ObjNative*)AS_OBJ(val))->function)
#define AS_STR(val)             ((ObjStr*)AS_OBJ(val))
#define AS_CSTR(val)            (((ObjStr*)AS_OBJ(val))->chars)

/** Heap-allocated objects supported by the language. */
typedef enum
{
    OBJ_BOUND_METHOD,
    OBJ_CLASS,
    OBJ_CLOSURE,
    OBJ_FUNC,
    OBJ_INSTANCE,
    OBJ_NATIVE,
    OBJ_STR,
    OBJ_UPVALUE
} ObjType;

/** 
 * Structure representing common components of dynamically allocated values.
 * 
 * Every object from the language has this definition as its first field, which
 * provides a way to toggle between the complete and the geral definitions
 * through struct inheritance. 
 * 
 * `type` is the type of the object.
 * `is_marked` is a garbage collection flag determning whether the object is
 *             reachable in memory or not.
 * `next` is a way to construct a linked-list of objects, used by the vm to
 *        keep track of allocations.
 */
struct Obj
{
    ObjType     type;
    bool        is_marked;
    struct Obj* next;
};

/**
 * Function object.
 * 
 * `arity` is the number of parameters the function expects.
 * `upvalue_count` is the number of upvalues accessed by the function.
 * `chunk` is the function's bytecode.
 * `name` is the name of the function.
 */
typedef struct
{
    Obj     obj;
    int     arity;
    int     upvalue_count;
    Chunk   chunk;
    ObjStr* name;
} ObjFun;

/** Represents a function that implements a native behaviour. */
typedef Value (*NativeFun)(int argc, Value* argv);

/**
 * Native function object.
 * 
 * `function` is a pointer to a function that implements the native behaviour.
 */
typedef struct
{
    Obj         obj;
    NativeFun   function;
} ObjNative;

/**
 * Structure representing a string object in the language.
 * 
 * `length` is the size of the string.
 * `hash` is the hash code calculated for the string upon definition.
 * `chars` is the string content, behaves as a flexible array member.
 */
struct ObjStr
{
    Obj         obj;
    int         length;
    uint32_t    hash;
    char        chars[];
};

/**
 * Upvalue object.
 * 
 * `location` is a pointer to the closed-over variable.
 * `closed` is the variable's value when it is captured.
 * `next` is used for garbage collection, defining a linked-list of upvalues
 *        pointing to variables on the stack.
 */
typedef struct _ObjUpvalue
{
    Obj                 obj;
    Value*              location;
    Value               closed;
    struct _ObjUpvalue* next;
} ObjUpvalue;

/**
 * Closure object.
 * 
 * Functions in the language are all defined as closures instead of `ObjFun`,
 * given the similar behaviour between them. Functions can have references to
 * variables declared in their bodies and also capture outer-scoped ones.
 * 
 * `function` is the function wrapped by the closure.
 * `upvalues` is a pointer to the closure's upvalue vector.
 * `upvalue_count` is the number of current upvalues captured.
 */
typedef struct
{
    Obj             obj;
    ObjFun*         function;
    ObjUpvalue**    upvalues;
    int             upvalue_count;
} ObjClosure;

/**
 * Class object.
 * 
 * `name` is the name of the class.
 * `methods` is a method hash table whose keys are the methods names and the
 *           values are closure objects. 
 */
typedef struct
{
    Obj     obj;
    ObjStr* name;
    Table   methods;
} ObjClass;

/**
 * Instance object.
 * 
 * `class` is a pointer to the instance's class.
 * `fields` is a hash table of the instance's state.
 */
typedef struct 
{
    Obj         obj;
    ObjClass*   class;
    Table       fields;
} ObjInst;

/**
 * Bound method object.
 */
typedef struct
{
    Obj             obj;
    Value           receiver;
    ObjClosure*     method;
} ObjBoundMethod;

/**
 * Allocates and initializes a binding between an object specified by
 * `receiver` and a closure specified by `method`.
 * 
 * Returns a pointer to the new binding.
 */
ObjBoundMethod* new_bound_method(Value receiver, ObjClosure* method);

/**
 * Allocates and initializes a class with a name specified by `name`.
 * 
 * Returns a pointer to the new class.
 */
ObjClass* new_class(ObjStr* name);

/**
 * Allocates and initializes an upvalue for a variable at a stack position
 * specified by `slot`.
 * 
 * Returns a pointer to the new upvalue.
 */
ObjUpvalue* new_upvalue(Value* slot);

/**
 * Allocates and initializes a closure from a function specified by `fun`.
 * 
 * Returns a pointer to the new closure.
 */
ObjClosure* new_closure(ObjFun* fun);

/**
 * Allocates and initializes a function.
 * 
 * Returns a pointer to the new function.
 */
ObjFun* new_func();

/**
 * Allocates and initializes an instance of a class specified by `class`.
 * 
 * Returns a pointer to the new instance created.
 */
ObjInst* new_instance(ObjClass* class);

/**
 * Allocates and initializes a native function with a signature specified by
 * `fun`.
 * 
 * Returns a pointer to the new native function.
 */
ObjNative* new_native(NativeFun fun);

/**
 * Takes ownership of a character stream specified by `data` with size `len`,
 * converting it to a string object.
 * 
 * Returns a pointer to the new string object.
 */
ObjStr* take_str(char* data, int len);

/**
 * Converts a static allocated string specified by `chars` woth size `len`
 * to a heap-allocated string.
 * 
 * Returns a pointer to the new string object.
*/
ObjStr* copy_str(const char* chars, int len);

/** Pretty prints an object value specified by `value`. */
void print_obj(Value value);

/**
 * Checks if a value specified by `value` is an object with type specified by
 * `type`.
 * 
 * Returns whether the value was the one expected or not.
 */
static inline bool is_obj_type(Value value, ObjType type)
{
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#endif