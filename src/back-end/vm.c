#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "back-end/object.h"
#include "back-end/vm.h"
#include "common.h"
#include "front-end/compiler.h"
#include "debug.h"
#include "memory.h"

vm_t vm; // FIXME: Should be manipulated through pointers.

static void reset_stack()
{
    vm.stack_top = vm.stack;
}

/// @brief Variadic function for runtime error reporting.
/// @param format specifies the output string format
/// @param params arguments to be printed according to the format
static void runtime_err(const char *format, ...)
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
    fprintf(stderr, "[line %d] in script\n", line);

    reset_stack();
}

/// @brief Looks at the stack without popping any value stored.
/// @param offset how far from the top of the stack to look
/// @return the value at the position specified
static value_t peek(int offset)
{
    return vm.stack_top[-1 - offset];
}

/// @brief Checks if the specified value has a false behaviour.
/// @param value 
/// @return whether the value is true or not
static bool is_falsey(value_t value)
{
    return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

/// @brief Concatenates two string objects.
static void concat()
{
    obj_str_t *b = AS_STR(pop());
    obj_str_t *a = AS_STR(pop());

    int len = a->length + b->length;
    char *chars = ALLOCATE(char, len + 1);

    memcpy(chars, a->chars, a->length);
    memcpy(chars + a->length, b->chars, b->length);
    chars[len] = '\0';

    obj_str_t *result = take_str(chars, len);
    push(OBJ_VAL(result));
}

static interpret_result_t run()
{
// Reads the byte currently pointed at and advances the instruction pointer.
#define READ_BYTE() (*vm.ip++)
//Reads the next two bytes from the chunk, building a 16-bit unsigned integer.
#define READ_SHORT() \
    (vm.ip += 2, (uint16_t)((vm.ip[-2] << 8) | vm.ip[-1]))
// Reads the next byte from the bytecode, treats the resulting number as an
// index and looks up the corresponding value in the chunk's constant table.
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
// Returns a string at a specific index in the constant table.
#define READ_STR() AS_STR(READ_CONSTANT())
// Executes numerical infix operations, pushing the result into the stack.
#define BINARY_OP(value_type, op)                       \
    do {                                                \
        if (!IS_NUM(peek(0)) || !IS_NUM(peek(1))) {     \
            runtime_err("Operands must be numbers");    \
            return INTERPRET_RUNTIME_ERROR;             \
        }                                               \
        double b = AS_NUM(pop());                       \
        double a = AS_NUM(pop());                       \
        push(value_type(a op b));                       \
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
            case OP_NIL:
                push(NIL_VAL);
                break;
            case OP_TRUE:
                push(BOOL_VAL(true));
                break;
            case OP_FALSE:
                push(BOOL_VAL(false));
                break;
            case OP_POP:
                pop();
                break;
            case OP_GET_LOCAL: {
                uint8_t slot = READ_BYTE();
                push(vm.stack[slot]);
                break;
            }
            case OP_SET_LOCAL: {
                uint8_t slot = READ_BYTE();
                vm.stack[slot] = peek(0);
                break;
            }
            case OP_GLOBAL: {
                obj_str_t *name = READ_STR();

                table_set(&vm.globals, name, peek(0));
                pop();
                break;
            }
            case OP_SET_GLOBAL: {
                obj_str_t *name = READ_STR();

                if (table_set(&vm.globals, name, peek(0))) {
                    table_delete(&vm.globals, name);
                    runtime_err("Undefined variable '%s'.", name->chars);

                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_GET_GLOBAL: {
                obj_str_t *name = READ_STR();
                value_t value;

                if (!table_get(&vm.globals, name, &value)) {
                    runtime_err("Undefined variable '%s'.", name->chars);

                    return INTERPRET_RUNTIME_ERROR;
                }
                push(value);
                break;
            }
            case OP_EQUAL: {
                value_t b = pop();
                value_t a = pop();

                push(BOOL_VAL(values_equal(a, b)));
            }
            break;
            case OP_GREATER:
                BINARY_OP(BOOL_VAL, >);
                break;
            case OP_LESS:
                BINARY_OP(BOOL_VAL, <);
                break;
            case OP_ADD: {
                if (!IS_STR(peek(0)) && !IS_STR(peek(1))) {
                    BINARY_OP(NUM_VAL, +);
                } else {
                    concat();
                }
                break;
            }
            case OP_SUBTRACT:
                BINARY_OP(NUM_VAL, -);
                break;
            case OP_MULTIPLY:
                BINARY_OP(NUM_VAL, *);
                break;
            case OP_DIVIDE:
                BINARY_OP(NUM_VAL, /);
                break;
            case OP_NOT:
                push(BOOL_VAL(is_falsey(pop())));
                break;
            case OP_NEGATE: {
                if (!IS_NUM(peek(0))) {
                    runtime_err("Operand must be a number.");

                    return INTERPRET_RUNTIME_ERROR;
                }
                push(NUM_VAL(-AS_NUM(pop())));
                break;
            }
            case OP_PRINT: {
                print_value(pop());
                printf("\n");
                break;
            }
            case OP_JUMP: {
                uint16_t offset = READ_SHORT();
                vm.ip += offset;
                break;
            }
            case OP_JUMP_FALSE: {
                uint16_t offset = READ_SHORT();

                if (is_falsey(peek(0)))
                    vm.ip += offset;
                    
                break;
            }
            case OP_RETURN:
                // Exit interpreter.
                return INTERPRET_OK;
        }
    }
#undef READ_BYTE
#undef READ_SHORT
#undef READ_CONSTANT
#undef RED_STR
#undef BINARY_OP
}

void init_vm()
{
    reset_stack();
    vm.objects = NULL;

    init_table(&vm.globals);
    init_table(&vm.strings);
}

/// @brief Frees all objects once the program is done executing.
void free_vm()
{
    free_table(&vm.globals);
    free_table(&vm.strings);
    free_objs();
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