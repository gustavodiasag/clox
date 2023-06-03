#include <stdio.h>

#include "common.h"
#include "debug.h"
#include "back-end/vm.h"

VM vm;

void init_vm()
{

}

void free_vm()
{

}

InterpretResult interpret(Chunk *chunk)
{
    vm.chunk = chunk;
    vm.ip = vm.chunk->code;

    return run();
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
            disassemble_instruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
        #endif

        uint8_t instruction;

        switch(instruction = READ_BYTE()) {
           case OP_CONSTANT: {
                Value constant = READ_CONSTANT();
                print_value(constant);
                printf("\n");
                break;
            }
            case OP_RETURN:
                return INTERPRET_OK;
        }
    }
#undef READ_BYTE
#undef READ_CONSTANT
}