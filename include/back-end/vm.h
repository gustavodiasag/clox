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
    obj_closure_t* closure;
    // Instead of storing the return address in the callee's
    // frame, the caller stores its own instruction pointer.
    uint8_t* ip;
    // Points into the virtual machine's value stack
    // at the first slot that the function can use.
    value_t* slots;
} call_frame_t;

typedef struct {
    // Dealing with function calls with stack semantics helps
    // optimizing memory usage by avoiding heap allocations.
    call_frame_t frames[FRAMES_MAX];
    // Stores the current height of the call frame stack.
    int frame_count;
    // Runtime stack of values.
    value_t stack[STACK_MAX];
    // Pointer to the top of the stack.
    value_t* stack_top;
    // Stores all strings created in the program.
    table_t strings;
    // Stores all global variables created in the program.
    table_t globals;
    // List of open upvalues that point to variables on the stack.
    obj_upvalue_t* open_upvalues;
    // Total number of bytes the vm has allocated.
    size_t bytes_allocated;
    // Threshold that triggers the next collection.
    size_t next_gc;
    // Linked list keeping track of every heap-allocated object.
    obj_t* objects;
    // Stores all the grey objects marked by the gc.
    obj_t** gray_stack;
    int gray_capacity;
    // Number of grey objects in total.
    int gray_count;
} vm_t;

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
} interpret_result_t;

// Exposes the global variable so that it can be used
// on other modules.
extern vm_t vm;

void init_vm();
void free_vm();
interpret_result_t interpret(const char* source);
void push(value_t value);
value_t pop();

#endif