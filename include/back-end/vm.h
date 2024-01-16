#ifndef VM_H
#define VM_H

#include "chunk.h"
#include "object.h"
#include "table.h"
#include "value.h"

/** Threshold for ongoing function calls. */
#define FRAMES_MAX      64
/** Threshold for stack slots. */
#define STACK_MAX       (FRAMES_MAX * UINT8_COUNT)

/**
 * Represents an ongoing function call.
 * 
 * `closure` is a wrapper around the function being called.
 * `ip` is the function's instruction pointer.
 * `slots` is a pointer to the vm's runtime stack, referencing the initial
 *         available slot for the function.
 */
typedef struct
{
    ObjClosure* closure;
    uint8_t*    ip;
    Value*      slots;
} CallFrame;

/**
 * Structure representing the language's virtual machine.
 * 
 * `frames` is a stack of active function calls.
 * `frame_count` is the current height of the call frame stack.
 * `stack` is the runtime stack.
 * `stack_top` is a pointer to the top of the runtime stack.
 * `strings` is a table of all the strings created in the program.
 * `init_string` is the name of a class' initializer method.
 * `globals` is a table of all the global variables created in a program.
 * `open_upvalues` is a list of upvalues that point to variables in the runtime
 *                 stack.
 * `bytes_allocated` is the total number of bytes the vm allocated.
 * `next_gc` is a threshold for triggering the next collection.
 * `objects` is a linked-list of heap-allocated values.
 * `gray_stack` is a list of objects marked by the garbage collector.
 * `gray_capacity` is the length of `grey_stack`.
 * `grey_count` is the current number of grey objects. 
 */
typedef struct
{
    CallFrame   frames[FRAMES_MAX];
    int         frame_count;
    Value       stack[STACK_MAX];
    Value*      stack_top;
    Table       strings;
    ObjStr*     init_string;
    Table       globals;
    ObjUpvalue* open_upvalues;
    size_t      bytes_allocated;
    size_t      next_gc;
    Obj*        objects;
    Obj**       gray_stack;
    int         gray_capacity;
    int         gray_count;
} Vm;

/** Possible return statuses during interpretation. */
typedef enum
{
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
} InterpretResult;

extern Vm vm;

/** Initializes the virtual machine. */
void init_vm();

/** Frees all dynamic allocations made by the virtual machine. */
void free_vm();

/**
 * Interprets a program whose content is specified by `source`.
 * 
 * Returns the interpretation status.
 */
InterpretResult interpret(const char* source);

/** Pushes a value specified by `value` onto the runtime stack. */
void push(Value value);

/**
 * Pops a value from the top of the runtime stack. 
 * 
 * Returns the value popped. 
 */
Value pop();

#endif