#include <stdio.h>
#include <string.h>

#include "back-end/object.h"
#include "back-end/table.h"
#include "back-end/value.h"
#include "back-end/vm.h"
#include "memory.h"

#define ALLOCATE_OBJ(type, obj_type) \
    (type*)allocate_obj(sizeof(type), obj_type)

#define ALLOCATE_STR(len) \
    (ObjStr*)allocate_obj(sizeof(ObjStr) + sizeof(char[len]), OBJ_STR)

/// @brief Allocates an object of a given size on the heap.
/// @param size specified number of bytes for varying extra fields
/// @param type type of the object being allocated
/// @return pointer to the object created
static Obj* allocate_obj(size_t size, ObjType type)
{
    Obj* obj = (Obj*)reallocate(NULL, 0, size);
    obj->type = type;
    // Every new object begins unmarked because it hasn't been
    // determined if it is reachable or not.
    obj->is_marked = false;
    // New object is inserted at the head of the linked list.
    obj->next = vm.objects;
    vm.objects = obj;

#ifdef DEBUG_LOG_GC
    printf("%p allocate %zu for %d\n", (void*)obj, size, type);
#endif

    return obj;
}

ObjBoundMethod* new_bound_method(Value receiver, ObjClosure* method)
{
    ObjBoundMethod* bound = ALLOCATE_OBJ(ObjBoundMethod, OBJ_BOUND_METHOD);
    bound->receiver = receiver;
    bound->method = method;

    return bound;
}

ObjClass* new_class(ObjStr* name)
{
    ObjClass* class = ALLOCATE_OBJ(ObjClass, OBJ_CLASS);
    class->name = name;
    init_table(&class->methods);

    return class;
}

ObjUpvalue* new_upvalue(Value* slot)
{
    ObjUpvalue* upvalue = ALLOCATE_OBJ(ObjUpvalue, OBJ_UPVALUE);
    upvalue->closed = NIL_VAL;
    upvalue->location = slot;
    upvalue->next = NULL;

    return upvalue;
}

ObjClosure* new_closure(ObjFun* function)
{
    ObjUpvalue** upvalues = ALLOCATE(ObjUpvalue*, function->upvalue_count);

    for (int i = 0; i < function->upvalue_count; i++)
        upvalues[i] = NULL;

    ObjClosure* closure = ALLOCATE_OBJ(ObjClosure, OBJ_CLOSURE);
    closure->function = function;
    closure->upvalues = upvalues;
    closure->upvalue_count = function->upvalue_count;

    return closure;
}

ObjFun* new_func()
{
    ObjFun* func = ALLOCATE_OBJ(ObjFun, OBJ_FUNC);
    func->upvalue_count = 0;
    func->arity = 0;
    func->name = NULL;

    init_chunk(&func->chunk);

    return func;
}

ObjInst* new_instance(ObjClass* class)
{
    ObjInst* instance = ALLOCATE_OBJ(ObjInst, OBJ_INSTANCE);
    instance->class = class;

    init_table(&instance->fields);

    return instance;
}

ObjNative* new_native(NativeFun function)
{
    ObjNative* native = ALLOCATE_OBJ(ObjNative, OBJ_NATIVE);
    native->function = function;

    return native;
}

/// @brief Creates a new string object and initializes its fields.
/// @param chars string content
/// @param len string length
/// @param hash string's hash code
/// @return pointer to the object created
static ObjStr* allocate_str(char* chars, int len, uint32_t hash)
{
    ObjStr* str = ALLOCATE_STR(len);
    str->hash = hash;
    str->length = len;
    memcpy(str->chars, chars, len);
    // Object is pushed onto the stack to guarantee it is reachable in case
    // garbage collection is triggered while resizing the interned strings
    // table.
    push(OBJ_VAL(str));
    // Whenever a new string is created, it's added to the virtual machine's table.
    table_set(&vm.strings, str, NIL_VAL);
    pop();

    return str;
}

/// @brief Short version of the FNV-1a hashing algorithm.
/// @param key string to be hashed
/// @param len string length
/// @return hash value for that specific key
static uint32_t hash_str(const char* key, int len)
{
    uint32_t hash = 2166136261u;

    for (int i = 0; i < len; i++) {
        hash ^= (uint8_t)key[i];
        hash *= 16777619;
    }

    return hash;
}

ObjStr* take_str(char* chars, int len)
{
    uint32_t hash = hash_str(chars, len);
    ObjStr* interned = table_find(&vm.strings, chars, len, hash);

    if (interned) {
        // Duplicate string is no longer needed.
        FREE_ARRAY(char, chars, len + 1);

        return interned;
    }

    return allocate_str(chars, len, hash);
}

ObjStr* copy_str(const char* chars, int len)
{
    uint32_t hash = hash_str(chars, len);

    ObjStr* interned = table_find(&vm.strings, chars, len, hash);

    if (interned)
        return interned;

    char* heap_chars = ALLOCATE(char, len + 1);
    memcpy(heap_chars, chars, len);

    heap_chars[len] = '\0';

    return allocate_str(heap_chars, len, hash);
}

/// @brief Prints the name of the given function object.
/// @param func function object
static void print_func(ObjFun* func)
{
    if (!func->name) {
        printf("<script>");
        return;
    }
    printf(" <fn %s>", func->name->chars);
}

void print_obj(Value value)
{
    switch (OBJ_TYPE(value)) {
    case OBJ_BOUND_METHOD:
        print_func(AS_BOUND_METHOD(value)->method->function);
        break;
    case OBJ_CLASS:
        printf("%s", AS_CLASS(value)->name->chars);
        break;
    case OBJ_CLOSURE:
        print_func(AS_CLOSURE(value)->function);
        break;
    case OBJ_FUNC:
        print_func(AS_FUNC(value));
        break;
    case OBJ_INSTANCE:
        printf("%s instance", AS_INSTANCE(value)->class->name->chars);
        break;
    case OBJ_NATIVE:
        printf("<native fn>");
        break;
    case OBJ_STR:
        printf("%s", AS_CSTR(value));
        break;
    case OBJ_UPVALUE:
        printf("upvalue");
        break;
    }
}