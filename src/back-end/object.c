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

static Obj* allocate_obj(size_t size, ObjType type)
{
    Obj* obj = (Obj*)reallocate(NULL, 0, size);
    obj->type = type;
    /* Every new object begins unmarked. */
    obj->is_marked = false;
    /* Every new object allocation is tracked by the vm. */
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

ObjClosure* new_closure(ObjFun* fun)
{
    ObjUpvalue** upvalues = ALLOCATE(ObjUpvalue*, fun->upvalue_count);

    for (int i = 0; i < fun->upvalue_count; i++) {
        upvalues[i] = NULL;
    }
    ObjClosure* closure = ALLOCATE_OBJ(ObjClosure, OBJ_CLOSURE);
    closure->function = fun;
    closure->upvalues = upvalues;
    closure->upvalue_count = fun->upvalue_count;

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

ObjNative* new_native(NativeFun fun)
{
    ObjNative* native = ALLOCATE_OBJ(ObjNative, OBJ_NATIVE);
    native->function = fun;

    return native;
}

static ObjStr* allocate_str(char* chars, int len, uint32_t hash)
{
    ObjStr* str = ALLOCATE_STR(len);
    str->hash = hash;
    str->length = len;
    memcpy(str->chars, chars, len);
    /*
     * The string is pushed onto the runtime stack to avoid collection if it is
     * triggered while resizing the interned strings table.
     */
    push(OBJ_VAL(str));
    /* String interning. */
    table_set(&vm.strings, str, NIL_VAL);
    pop();

    return str;
}

static uint32_t hash_str(const char* key, int len)
{
    /* FNV-1a hash. */
    uint32_t hash = 2166136261u;

    for (int i = 0; i < len; i++) {
        hash ^= (uint8_t)key[i];
        hash *= 16777619;
    }
    return hash;
}

ObjStr* take_str(char* data, int len)
{
    uint32_t hash = hash_str(data, len);
    ObjStr* interned = table_find(&vm.strings, data, len, hash);

    if (interned) {
        /* Duplicate string is no longer needed. */
        FREE_ARRAY(char, data, len + 1);
        return interned;
    }
    return allocate_str(data, len, hash);
}

ObjStr* copy_str(const char* chars, int len)
{
    uint32_t hash = hash_str(chars, len);
    ObjStr* interned = table_find(&vm.strings, chars, len, hash);

    if (interned) {
        return interned;
    }
    char* heap_chars = ALLOCATE(char, len + 1);
    memcpy(heap_chars, chars, len);
    heap_chars[len] = '\0';

    return allocate_str(heap_chars, len, hash);
}

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