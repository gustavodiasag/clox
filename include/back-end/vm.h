#ifndef VM_H
#define VM_H

#include "chunk.h"
#include "object.h"
#include "table.h"
#include "value.h"

#define FRAMES_MAX 64
#define GC_THRESHOLD 0x100000
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)

// Represents a single ongoing function call.
typedef struct {
    // Function being called.
    ObjClosure* closure;
    // Instead of storing the return address in the callee's
    // frame, the caller stores its own instruction pointer.
    uint8_t* ip;
    // Points into the virtual machine's value stack
    // at the first slot that the function can use.
    Value* slots;
} CallFrame;

typedef struct {
    // Dealing with function calls with stack semantics helps
    // optimizing memory usage by avoiding heap allocations.
    CallFrame frames[FRAMES_MAX];
    // Stores the current height of the call frame stack.
    int frame_count;
    // Runtime stack of values.
    Value stack[STACK_MAX];
    // Pointer to the top of the stack.
    Value* stack_top;
    // Stores all strings created in the program.
    Table strings;
    // Stores all global variables created in the program.
    Table globals;
    // List of open upvalues that point to variables on the stack.
    ObjUpvalue* open_upvalues;
    // Total number of bytes the vm has allocated.
    size_t bytes_allocated;
    // Threshold that triggers the next collection.
    size_t next_gc;
    // Linked list keeping track of every heap-allocated object.
    Obj* objects;
    // Stores all the grey objects marked by the gc.
    Obj** gray_stack;
    int gray_capacity;
    // Number of grey objects in total.
    int gray_count;
} Vm;

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
} InterpretResult;

// Exposes the global variable so that it can be used
// on other modules.
extern Vm vm;

// TODO: Description.
void init_vm();

/// @brief Frees all objects once the program is done executing.
void free_vm();

// TODO: Description.
InterpretResult interpret(const char* source);

// TODO: Description.
void push(Value value);

// TODO: Description.
Value pop();

#endif