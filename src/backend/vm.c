#include <stdio.h>

#include "backend/vm.h"
#include "common.h"
#include "frontend/compiler.h"
#include "debug.h"

vm_t vm; // FIXME: Should be manipulated with pointers.

static void reset_stack()
{
    vm.stack_top = vm.stack;
}

static interpret_result_t run()
{
// Reads the byte currently pointed at and advances the instruction pointer.
#define READ_BYTE() (*vm.ip++)

// Reads the next byte from the bytecode, treats the resulting number as an
// index and looks up the corresponding Value in the chunk's constant table
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
#define BINARY_OP(op) \
    do { \
        double b = pop(); \
        double a = pop(); \
        push(a op b); \
    } while (false); // Ensures that all statements are within the same scope.

    for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
        printf("          ");

        for (value_t *v = vm.stack; v < vm.stack_top; v++) {
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
                value_t constant = READ_CONSTANT();
                push(constant);
                break;
            case OP_ADD:
                BINARY_OP(+);
                break;
            case OP_SUBTRACT:
                BINARY_OP(-);
                break;
            case OP_MULTIPLY:
                BINARY_OP(*);
                break;
            case OP_DIVIDE:
                BINARY_OP(/);
                break;
            case OP_NEGATE:
                *(vm.stack_top - 1) = -*(vm.stack_top - 1);
                break;
            case OP_RETURN:
                print_value(pop());
                printf("\n");
                return INTERPRET_OK;
        }
    }
#undef READ_BYTE
#undef READ_CONSTANT
#undef BINARY_OP
}

void init_vm()
{
    reset_stack();
}

void free_vm()
{

}

void push(value_t value)
{
    *vm.stack_top = value;
    vm.stack_top++;
}

value_t pop()
{
    vm.stack_top--;

    return *vm.stack_top;
}

interpret_result_t interpret(const char *source)
{
    chunk_t chunk;

    init_chunk(&chunk);

    if (!compile(source, &chunk)) {
        free_chunk(&chunk);

        return INTERPRET_COMPILE_ERROR;
    }

    vm.chunk = &chunk;
    vm.ip = vm.chunk->code;

    interpret_result_t result = run();

    free_chunk(&chunk);

    return result;
}
