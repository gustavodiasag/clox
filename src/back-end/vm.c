#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include "back-end/object.h"
#include "back-end/vm.h"
#include "common.h"
#include "front-end/compiler.h"
#include "debug.h"
#include "memory.h"

vm_t vm; // FIXME: Should be manipulated through pointers.

/// @brief Defines one of the native functions supported by the language.
/// @param argc number of arguments 
/// @param argv pointer to the first argument
/// @return elapsed time since the program started running. 
static value_t clock_native(int argc, value_t *argv)
{
    return NUM_VAL((double)clock() / CLOCKS_PER_SEC);
}

static void reset_stack()
{
    vm.stack_top = vm.stack;
    vm.frame_count = 0;
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

    // Stack trace of each function executing when the error occurred.
    for (int i = vm.frame_count - 1; i >= 0; i--) {
        call_frame_t *frame = &vm.frames[i];
        obj_func_t *func = frame->closure->function;
        size_t instruction = frame->ip - func->chunk.code - 1;

        fprintf(stderr, "[line %d] in ", func->chunk.lines[instruction]);

        if (func->name == NULL) {
            fprintf(stderr, "script\n");
        } else {
            fprintf(stderr, "%s()\n", func->name->chars);
        }
    }

    reset_stack();
}

/// @brief Defines a new native function exposed to the language.
/// @param name function name
/// @param function native function
static void define_native(const char *name, native_fn_t function)
{
    push(OBJ_VAL(copy_str(name, (int)strlen(name))));
    push(OBJ_VAL(new_native(function)));
    
    table_set(&vm.globals, AS_STR(vm.stack[0]), vm.stack[1]);
}

/// @brief Looks at the stack without popping any value stored.
/// @param offset how far from the top of the stack to look
/// @return the value at the position specified
static value_t peek(int offset)
{
    return vm.stack_top[-1 - offset];
}

/// @brief Initializes the next call frame on the stack.
/// @param func closure being called (all functions are closures)
/// @param args amount of arguments supported
/// @return status of successfully initialized frame
static bool init_frame(obj_closure_t *closure, int args)
{
    // The overlapping stack windows work based on the assumption
    // that a call passes exactly one argument for each of the
    // function's parameters, but considering that lox is dinamically
    // typed, a user could pass too many or too few arguments.
    if (args != closure->function->arity) {
        runtime_err("Expected %d arguments but got %d.",
            closure->function->arity, args);
        return false;
    }
    // Deals with a deep call chain.
    if (vm.frame_count == FRAMES_MAX) {
        runtime_err("Stack overflow.");
        return false;
    }

    call_frame_t *frame = &vm.frames[vm.frame_count++];
    
    frame->closure = closure;
    frame->ip = closure->function->chunk.code;
    // Provides the window into the stack for the new frame,
    // the arithmetic ensures that the arguments already on
    // the stack line up with the function's parameters.
    frame->slots = vm.stack_top - args - 1;

    return true;
}

/// @brief Executes a function call.
/// @param callee value containing the function object
/// @param args amount of arguments supported
/// @return whether the call was successfully executed or not
static bool call_value(value_t callee, int args)
{
    if (IS_OBJ(callee)) {
        switch (OBJ_TYPE(callee)) {
            case OBJ_CLOSURE:
                return call(AS_CLOSURE(callee), args);
            case OBJ_NATIVE: {
                native_fn_t native = AS_NATIVE(callee);
                value_t result = native(args, vm.stack_top - args);

                vm.stack_top -= args + 1;
                push(result);

                return true;
            }
            default:
                break; // Non-callable object type.
        }
    }
    runtime_err("Only function and classes can be called");

    return false;
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
    call_frame_t *frame = &vm.frames[vm.frame_count - 1];
// Reads the byte currently pointed at and advances the frame's ip.
#define READ_BYTE() (*frame->ip++)
// Reads the next two bytes from the chunk, building a 16-bit unsigned integer.
#define READ_SHORT() \
    (frame->ip += 2, (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))
// Reads the next byte from the bytecode, treats the resulting number as an
// index and looks up the corresponding value in the chunk's constant table.
#define READ_CONSTANT() \
    (frame->closure->function->chunk.constants.values[READ_BYTE()])
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

        disassemble_instruction(&frame->closure->function->chunk,
            (int)(frame->ip - frame->closure->function->chunk.code));
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
                push(frame->slots[slot]);
                break;
            }
            case OP_SET_LOCAL: {
                uint8_t slot = READ_BYTE();
                frame->slots[slot] = peek(0);
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
                frame->ip += offset;
                break;
            }
            case OP_JUMP_FALSE: {
                uint16_t offset = READ_SHORT();

                if (is_falsey(peek(0)))
                    frame->ip += offset;
                    
                break;
            }
            case OP_LOOP: {
                uint16_t offset = READ_SHORT();
                frame->ip -= offset;
                break;
            }
            case OP_CALL: {
                int args = READ_BYTE();

                if (!call_value(peek(args), args))
                    return INTERPRET_RUNTIME_ERROR;

                frame = &vm.frames[vm.frame_count - 1];
                break;
            }
            case OP_CLOSURE: {
                obj_func_t *function = AS_FUNCTION(READ_CONSTANT());
                obj_closure_t *closure = new_closure(function);

                push(OBJ_VAL(closure));
                break;
            }
            case OP_RETURN: {
                value_t result = pop();
                vm.frame_count--;

                if (vm.frame_count == 0) {
                    pop();
                    return INTERPRET_OK;
                }

                vm.stack_top = frame->slots;
                push(result);
                frame = &vm.frames[vm.frame_count - 1];
                break;
            }
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

    define_native("clock", clock_native);
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
    obj_func_t *func = compile(source);

    if (!func)
        return INTERPRET_COMPILE_ERROR;

    push(OBJ_VAL(func));

    obj_closure_t *closure = new_closure(func);

    pop();
    push(OBJ_VAL(closure));
    init_frame(func, 0);

    return run();
}