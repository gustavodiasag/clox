#include <stdio.h>
#include <stdarg.h>

#include "back-end/vm.h"
#include "common.h"
#include "front-end/compiler.h"
#include "debug.h"

vm_t vm; // FIXME: Should be manipulated through pointers.

static void reset_stack()
{
    vm.stack_top = vm.stack;
}

/// @brief Variadic function for runtime error reporting.
/// @param format specifies the output string format
/// @param params arguments to be printed according to the format
static void runtime_error(const char *format, ...)
{
    // Enables passing an arbitrary number of arguments to the function.
    va_list args;
    va_start(args, format);
    // Variadic version of printf.
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    size_t instruction = vm.ip - vm.chunk->code - 1;
    int line = vm.chunk->lines[instruction];
    fprintf(stderr, "[line &d] in script\n", line);

    reset_stack();
}

/// @brief Looks at the stack without popping any value stored.
/// @param offset how far from the top of the stack to look
/// @return the value at the position specified
static value_t peek(int offset)
{
    return vm.stack_top[-1 - offset];
}

static interpret_result_t run()
{
// Reads the byte currently pointed at and advances the instruction pointer.
#define READ_BYTE() (*vm.ip++)

// Reads the next byte from the bytecode, treats the resulting number as an
// index and looks up the corresponding value in the chunk's constant table.
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
#define BINARY_OP(op)       \
    do {                    \
        double b = pop();   \
        double a = pop();   \
        push(a op b);       \
    } while (false); // Ensures that all statements are within the same scope.

    while (true) {
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
                push(READ_CONSTANT());
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
                if (!IS_NUM(peek(0))) {
                    runtime_err("Operand must be a number.");

                    return INTERPRET_RUNTIME_ERROR;
                }
                push(NUM_VAL(-AS_NUM(pop())));
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
