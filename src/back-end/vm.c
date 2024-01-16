#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "back-end/vm.h"
#include "common.h"
#include "debug.h"
#include "front-end/compiler.h"
#include "memory.h"

#define GC_THRESHOLD    0x100000

Vm vm;

/// @brief Defines one of the native functions supported by the language.
/// @param argc number of arguments
/// @param argv pointer to the first argument
/// @return elapsed time since the program started running.
static Value clock_native(int argc, Value* argv)
{
    return NUM_VAL((double)clock() / CLOCKS_PER_SEC);
}

static void reset_stack()
{
    vm.stack_top = vm.stack;
    vm.frame_count = 0;
    vm.open_upvalues = NULL;
}

/// @brief Variadic function for runtime error reporting.
/// @param format specifies the output string format
/// @param params arguments to be printed according to the format
static void runtime_err(const char* format, ...)
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
        CallFrame* frame = &vm.frames[i];
        ObjFun* func = frame->closure->function;
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
static void define_native(const char* name, NativeFun function)
{
    push(OBJ_VAL(copy_str(name, (int)strlen(name))));
    push(OBJ_VAL(new_native(function)));

    table_set(&vm.globals, AS_STR(vm.stack[0]), vm.stack[1]);
}

/// @brief Looks at the stack without popping any value stored.
/// @param offset how far from the top of the stack to look
/// @return the value at the position specified
static Value peek(int offset)
{
    return vm.stack_top[-1 - offset];
}

/// @brief Initializes the next call frame on the stack.
/// @param func closure being called (all functions are closures)
/// @param args amount of arguments supported
/// @return status of successfully initialized frame
static bool init_frame(ObjClosure* closure, int args)
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
    CallFrame* frame = &vm.frames[vm.frame_count++];

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
static bool call_value(Value callee, int args)
{
    if (IS_OBJ(callee)) {
        switch (OBJ_TYPE(callee)) {
        case OBJ_BOUND_METHOD: {
            ObjBoundMethod* bound = AS_BOUND_METHOD(callee);
            // When a method is called, the top of the stack
            // contains all of the arguments, and then just
            // under those is the closure of the called method.
            // The receiver is then inserted into that slot.
            vm.stack_top[-args - 1] = bound->receiver;

            return init_frame(bound->method, args);
        }
        case OBJ_CLASS: {
            // If the value being called is a class, then it
            // is treated as a constructor call.
            ObjClass* class = AS_CLASS(callee);
            vm.stack_top[-args - 1] = OBJ_VAL(new_instance(class));
            
            Value initializer;
            // A class isn't required to have an initializer. If omitted,
            // the runtime returns the uninitialized instance, but if any
            // arguments were passed when an instance is initialized when
            // there's no `init` method, its a runtime error.   
            if (table_get(&class->methods, vm.init_string, &initializer)) {
                return init_frame(AS_CLOSURE(initializer), args);
            } else if (args != 0) {
                runtime_err("Expected 0 arguments but got %d.", args);
                return false;
            }
            return true;
        }
        case OBJ_CLOSURE:
            return init_frame(AS_CLOSURE(callee), args);
        case OBJ_NATIVE: {
            NativeFun native = AS_NATIVE(callee);
            Value result = native(args, vm.stack_top - args);

            vm.stack_top -= args + 1;
            push(result);

            return true;
        }
        default:
            // Non-callable object type.
            break;
        }
    }
    runtime_err("Only functions and classes can be called.");

    return false;
}

/// @brief Combines the virtual machine behavior for the `OP_GET_PROPERTY`
/// and the `OP_CALL` instructions.
/// @param class class from which the instance belongs to
/// @param name method name
/// @param args amount of arguments passed to the method
/// @return whether the invocation succeded or not
static bool invoke_from_class(ObjClass* class, ObjStr* name, int args)
{
    Value method;
    if (!table_get(&class->methods, name, &method)) {
        runtime_err("Undefined property '%s'.", name->chars);
        return false;
    }
    return init_frame(AS_CLOSURE(method), args);
}

/// @brief Retrieves the receiver object from the stack and invokes
/// the method.
/// @param name method name 
/// @param args amount of arguments passed to the method
/// @return whether the invocation succeded or not
static bool invoke(ObjStr* name, int args)
{
    Value receiver = peek(args);
    // The invoke operation assumes that the object from which
    // the method was called is an instance, so an invalid type
    // of value being used turns to be a runtime error.  
    if (!IS_INSTANCE(receiver)) {
        runtime_err("Only instances have methods.");
        return false;
    }
    ObjInst* instance = AS_INSTANCE(receiver);
    
    Value value;
    if (table_get(&instance->fields, name, &value)) {
        vm.stack_top[-args - 1] = value;
        return call_value(value, args);
    }
    return invoke_from_class(instance->class, name, args);
}

/// @brief Places the given method in the runtime stack.
/// @param class contains the method table
/// @param name method name
/// @return whether the method was found in the table
static bool bind_method(ObjClass* class, ObjStr* name)
{
    Value method;
    if (!table_get(&class->methods, name, &method)) {
        runtime_err("Undefined property '%s'.", name->chars);
        return false;
    }
    ObjBoundMethod* bound = new_bound_method(peek(0), AS_CLOSURE(method));
    pop();
    push(OBJ_VAL(bound));

    return true;
}

/// @brief Closes over a local variable from the surrounding function.
/// @param local pointer to the captured local's slot
/// @return pointer to the upvalue object created
static ObjUpvalue* capture_upvalue(Value* local)
{
    ObjUpvalue* prev_upvalue = NULL;
    ObjUpvalue* curr_upvalue = vm.open_upvalues;

    while (curr_upvalue && curr_upvalue->location > local) {
        prev_upvalue = curr_upvalue;
        curr_upvalue = curr_upvalue->next;
    }

    if (curr_upvalue && curr_upvalue->location == local) {
        return curr_upvalue;
    }
    ObjUpvalue* upvalue = new_upvalue(local);
    upvalue->next = curr_upvalue;

    if (!prev_upvalue) {
        vm.open_upvalues = upvalue;
    } else {
        prev_upvalue->next = upvalue;
    }

    return upvalue;
}

/// @brief Closes every open upvalue pointing to the given slot.
/// @param last pointer to a stack slot
static void close_upvalues(Value* last)
{
    while (vm.open_upvalues && vm.open_upvalues->location >= last) {
        ObjUpvalue* upvalue = vm.open_upvalues;
        upvalue->closed = *upvalue->location;
        upvalue->location = &upvalue->closed;

        vm.open_upvalues = upvalue->next;
    }
}

/// @brief Stores the closure for a method in the class's method table
/// @param name method
static void define_method(ObjStr* name)
{
    Value method = peek(0);
    ObjClass* class = AS_CLASS(peek(1));

    table_set(&class->methods, name, method);
    pop();
}

/// @brief Checks if the specified value has a false behaviour.
/// @param value
/// @return whether the value is true or not
static bool is_falsey(Value value)
{
    return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

/// @brief Concatenates two string objects.
static void concat()
{
    // Concatenating two strings requires allocating a new character
    // array on the heap, which can trigger garbage collection. To
    // maintain a reference to those objects so they can be reached
    // in the marking process, they are peeked instead of popped
    // from the stack.
    ObjStr* b = AS_STR(peek(0));
    ObjStr* a = AS_STR(peek(1));

    int len = a->length + b->length;
    char* chars = ALLOCATE(char, len + 1);

    memcpy(chars, a->chars, a->length);
    memcpy(chars + a->length, b->chars, b->length);
    chars[len] = '\0';

    ObjStr* result = take_str(chars, len);

    pop();
    pop();
    push(OBJ_VAL(result));
}

static InterpretResult run()
{
    CallFrame* frame = &vm.frames[vm.frame_count - 1];
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
#define BINARY_OP(value_type, op)                    \
    do {                                             \
        if (!IS_NUM(peek(0)) || !IS_NUM(peek(1))) {  \
            runtime_err("Operands must be numbers"); \
            return INTERPRET_RUNTIME_ERROR;          \
        }                                            \
        double b = AS_NUM(pop());                    \
        double a = AS_NUM(pop());                    \
        push(value_type(a op b));                    \
    } while (false); // Ensures that all statements are within the same scope.

    while (true) {
#ifdef DEBUG_TRACE_EXECUTION
        printf("          ");

        for (Value* v = vm.stack; v < vm.stack_top; v++) {
            printf("[ ");
            print_value(*v);
            printf(" ]");
        }
        printf("\n");

        disassemble_instruction(
            &frame->closure->function->chunk,
            (int)(frame->ip - frame->closure->function->chunk.code));
#endif
        uint8_t instruction;

        switch (instruction = READ_BYTE()) {
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
            ObjStr* name = READ_STR();

            table_set(&vm.globals, name, peek(0));
            pop();
            break;
        }
        case OP_SET_GLOBAL: {
            ObjStr* name = READ_STR();

            if (table_set(&vm.globals, name, peek(0))) {
                table_delete(&vm.globals, name);
                runtime_err("Undefined variable '%s'.", name->chars);

                return INTERPRET_RUNTIME_ERROR;
            }
            break;
        }
        case OP_GET_GLOBAL: {
            ObjStr* name = READ_STR();
            Value value;

            if (!table_get(&vm.globals, name, &value)) {
                runtime_err("Undefined variable '%s'.", name->chars);
                return INTERPRET_RUNTIME_ERROR;
            }
            push(value);
            break;
        }
        case OP_GET_UPVALUE: {
            // Index into the current function's upvalue array.
            uint8_t slot = READ_BYTE();
            // Looks up the correspondent upvalue and dereference
            // its location pointer to read the value in that slot.
            push(*frame->closure->upvalues[slot]->location);
            break;
        }
        case OP_SET_UPVALUE: {
            uint8_t slot = READ_BYTE();
            // Takes the value at the top of the stack and store it
            // into the slot pointed to by the chosen upvalue.
            *frame->closure->upvalues[slot]->location = peek(0);
            break;
        }
        case OP_GET_PROPERTY: {
            // Only instances are allowed to have fields. So there's
            // the need to check that the value is an instance before
            // accessing any fields on it.
            if (!IS_INSTANCE(peek(0))) {
                runtime_err("Only instances have properties.");

                return INTERPRET_RUNTIME_ERROR;
            }
            ObjInst* instance = AS_INSTANCE(peek(0));
            ObjStr* name = READ_STR();

            Value value;

            if (table_get(&instance->fields, name, &value)) {
                pop(); /* instance */
                push(value);
                break;
            }
            // Fields take priority over methods, shadowing them
            // if the instance doesn't have a field with the
            // property name, then it might refer to a method.
            if (!bind_method(instance->class, name)) {
                // If it couldn't be found, that means a runtime error.
                return INTERPRET_RUNTIME_ERROR;
            }
            break;
        }
        case OP_SET_PROPERTY: {
            // Storing data on a field of a value that is not an
            // instance is considered an error.
            if (!IS_INSTANCE(peek(1))) {
                runtime_err("Only instances have fields.");

                return INTERPRET_RUNTIME_ERROR;
            }
            // At this point, the top of the stack has the instance
            // whose field is being sest and above that, the value
            // to be stored. The operand is read and the field name
            // string is determined. With that, the value on top of
            // the stack is stored into the instance's field table.
            ObjInst* instance = AS_INSTANCE(peek(1));
            table_set(&instance->fields, READ_STR(), peek(0));

            Value value = pop();
            pop();
            push(value);
            break;
        }
        case OP_GET_SUPER: {
            ObjStr* name = READ_STR();
            ObjClass* super = AS_CLASS(pop());

            if (!bind_method(super, name)) {
                return INTERPRET_RUNTIME_ERROR;
            }
            break;
        }
        case OP_EQUAL: {
            Value b = pop();
            Value a = pop();

            push(BOOL_VAL(values_equal(a, b)));
            break;
        }
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

            if (is_falsey(peek(0))) {
                frame->ip += offset;
            }
            break;
        }
        case OP_LOOP: {
            uint16_t offset = READ_SHORT();
            frame->ip -= offset;
            break;
        }
        case OP_CALL: {
            int args = READ_BYTE();
            
            if (!call_value(peek(args), args)) {
                return INTERPRET_RUNTIME_ERROR;
            }
            frame = &vm.frames[vm.frame_count - 1];
            break;
        }
        case OP_INVOKE: {
            ObjStr* method = READ_STR();
            int args = READ_BYTE();

            if (!invoke(method, args)) {
                return INTERPRET_RUNTIME_ERROR;
            }
            frame = &vm.frames[vm.frame_count - 1];
            break;
        }
        case OP_SUPER_INVOKE: {
            ObjStr* method = READ_STR();
            int args = READ_BYTE();
            ObjClass* super = AS_CLASS(pop());

            if (!invoke_from_class(super, method, args)) {
                return INTERPRET_RUNTIME_ERROR;
            }
            frame = &vm.frames[vm.frame_count - 1];
            break;
        }
        case OP_CLOSURE: {
            ObjFun* function = AS_FUNC(READ_CONSTANT());
            ObjClosure* closure = new_closure(function);
            push(OBJ_VAL(closure));
            
            for (int i = 0; i < closure->upvalue_count; i++) {
                uint8_t is_local = READ_BYTE();
                uint8_t index = READ_BYTE();

                if (is_local) {
                    // Here, `capture_value` takes a pointer to the local's slot
                    // in the surrounding function's stack window. That window
                    // begins at `frame->slots` which points to slot zero.
                    // Adding `index` offsets that to the right slot.
                    closure->upvalues[i] = capture_upvalue(frame->slots + index);
                } else {
                    // At the moment a function's declaration is being executed,
                    // the current function is the surrounding one. So, the
                    // upvalue from the enclosing function is directly read from
                    // `frame`.
                    closure->upvalues[i] = frame->closure->upvalues[index];
                }
            }
            break;
        }
        case OP_CLOSE_UPVALUE: {
            close_upvalues(vm.stack_top - 1);
            pop();
            break;
        }
        case OP_CLASS: {
            push(OBJ_VAL(new_class(READ_STR())));
            break;
        }
        case OP_INHERIT: {
            Value super = peek(1);
            // An object that is not a class cannot be inherited from.
            if (!IS_CLASS(super)) {
                runtime_err("Superclass must be a class.");
                return INTERPRET_RUNTIME_ERROR;
            }
            ObjClass* sub = AS_CLASS(peek(0));
            table_add_all(&AS_CLASS(super)->methods, &sub->methods);
            // Pop subclass. 
            pop();
            break;
        }
        case OP_RETURN: {
            Value result = pop();
            // Closes variables defined in the context of the function's scope,
            // such as it's parameters or locals declared inside its body.
            close_upvalues(frame->slots);
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
        case OP_METHOD:
            define_method(READ_STR());
            break;
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
    vm.bytes_allocated = 0;
    vm.next_gc = GC_THRESHOLD;
    vm.gray_count = 0;
    vm.gray_capacity = 0;
    vm.gray_stack = NULL;

    init_table(&vm.globals);
    init_table(&vm.strings);

    vm.init_string = NULL;
    vm.init_string = copy_str("init", 4);

    define_native("clock", clock_native);
}

void free_vm()
{
    free_table(&vm.globals);
    free_table(&vm.strings);
    vm.init_string = NULL;
    free_objs();
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

InterpretResult interpret(const char* source)
{
    ObjFun* func = compile(source);

    if (!func) {
        return INTERPRET_COMPILE_ERROR;
    }
    push(OBJ_VAL(func));

    ObjClosure* closure = new_closure(func);

    pop();
    push(OBJ_VAL(closure));
    init_frame(closure, 0);

    return run();
}