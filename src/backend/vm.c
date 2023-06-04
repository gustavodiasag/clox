#include <stdio.h>

#include "common.h"
#include "debug.h"
#include "backend/vm.h"

VM vm; // FIXME: Should be manipulated through pointers.

static void reset_stack()
{
    vm.stack_top = vm.stack;
}

static InterpretResult run()
{
// Reads the byte currently pointed at and advances the instruction pointer.
#define READ_BYTE() (*vm.ip++)

// Reads the next byte from the bytecode, treats the resulting number as an
// index and looks up the corresponding Value in the chunk's constant table
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])

    for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
            printf("          ");

            for (Value *v = vm.stack; v < vm.stack_top; v++) {
                printf("[ ");
                print_value(*v);
                printf(" ]");
            }
            printf("\n");

            disassemble_instruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
#endif

        uint8_t instruction;

        switch(instruction = READ_BYTE()) {
           case OP_CONSTANT:
                Value constant = READ_CONSTANT();
                push(constant);
                break;
            case OP_NEGATE:
                push(-pop());
                break;
            case OP_RETURN:
                print_value(pop());
                printf("\n");
                return INTERPRET_OK;
        }
    }
#undef READ_BYTE
#undef READ_CONSTANT
}

void init_vm()
{
    reset_stack();
}

void free_vm()
{

}

void push(Value value)
{
    *vm.stack_top = value;
    vm.stack_top++;
}

Value pop()
{
    vm.stack_top--;

    return *vm.stack_top;
}

InterpretResult interpret(Chunk *chunk)
{
    vm.chunk = chunk;
    vm.ip = vm.chunk->code;

    return run();
}
