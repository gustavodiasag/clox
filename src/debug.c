#include <stdio.h>

#include "debug.h"
#include "back-end/value.h"

/// @brief Prints the compound structure stored at the given offset.
/// @param name instruction description
/// @param chunk contains the instructions to be checked
/// @param offset used to access the specified instruction 
/// @return the offset of the next instruction
static int constant_instruction(const char *name, chunk_t *chunk, int offset)
{
    uint8_t constant = chunk->code[offset + 1];
    printf("%-16s %4d '", name, constant);
    print_value(chunk->constants.values[constant]);
    printf("'\n");
    // Takes two bytes, one for the opcode and one for the operand.
    return offset + 2;
}

/// @brief Prints the simple structure stored at the given offset.
/// @param name instruction description
/// @param offset used to access the specified instruction
/// @return the offset of the next instruction
static int simple_instruction(const char *name, int offset)
{
    printf("%s\n", name);
    return offset + 1;
}

/// @brief Prints the stack slot corresponding to that instruction operand.
/// @param name instruction description
/// @param chunk contains the instructions to be checked
/// @param offset used to access the specified instruction
/// @return the offset of the next instruction
static int byte_instruction(const char *name, chunk_t *chunk, int offset)
{
    uint8_t slot = chunk->code[offset + 1];
    printf("%-16s %4d\n", name, slot);

    return offset + 2;
}

/// @brief Disassembles an instruction with a 16-bit operand.
/// @param name instruction description
/// @param sign 
/// @param chunk contains the instruction to be checked 
/// @return the offset od the next instruction
static int jump_instruction(const char *name, int sign, chunk_t *chunk, int offset)
{
    uint16_t jump = (uint16_t)(chunk->code[offset + 1] << 8);
    jump |= chunk->code[offset + 2];

    printf("%-16s %4d -> %d\n", name, offset, offset + 3 + sign * jump);

    return offset + 3;
}

/// @brief Prints all the instructions in a chunk.
/// @param chunk stores the bytecode to be analysed
void disassemble_chunk(chunk_t *chunk)
{
    for (int offset = 0; offset < chunk->count;) {
        offset = disassemble_instruction(chunk, offset);
    }
}

/// @brief Disassembles the instruction at the given offset.
/// @param chunk contains the instruction to be checked
/// @param offset used to access a specific instruction
/// @return the offset of the next instruction
int disassemble_instruction(chunk_t *chunk, int offset)
{
    printf("%04d ", offset);

    if (offset > 0 && chunk->lines[offset] == chunk->lines[offset - 1])
        printf("   | ");
    else
        printf("%4d ", chunk->lines[offset]);

    uint8_t instruction = chunk->code[offset];

    switch (instruction) {
        case OP_CONSTANT:
            return constant_instruction("OP_CONSTANT", chunk, offset);
        case OP_NIL:
            return simple_instruction("OP_NIL", offset);
        case OP_TRUE:
            return simple_instruction("OP_TRUE", offset);
        case OP_FALSE:
            return simple_instruction("OP_FALSE", offset);
        case OP_EQUAL:
            return simple_instruction("OP_EQUAL", offset);
        case OP_POP:
            return simple_instruction("OP_POP", offset);
        case OP_GET_LOCAL:
            return byte_instruction("OP_GET_LOCAL", chunk, offset);
        case OP_SET_LOCAL:
            return byte_instruction("OP_SET_LOCAL", chunk, offset);
        case OP_GLOBAL:
            return constant_instruction("OP_GLOBAL", chunk, offset);
        case OP_SET_GLOBAL:
            return constant_instruction("OP_SET_GLOBAL", chunk, offset);
        case OP_GET_GLOBAL:
            return constant_instruction("OP_GET_GLOBAL", chunk, offset);
        case OP_GREATER:
            return simple_instruction("OP_GREATER", offset);
        case OP_LESS:
            return simple_instruction("OP_LESS", offset);
        case OP_ADD:
            return simple_instruction("OP_ADD", offset);
        case OP_SUBTRACT:
            return simple_instruction("OP_SUBTRACT", offset);
        case OP_MULTIPLY:
            return simple_instruction("OP_MULTIPLY", offset);
        case OP_DIVIDE:
            return simple_instruction("OP_DIVIDE", offset);
        case OP_NOT:
            return simple_instruction("OP_NOT", offset);
        case OP_NEGATE:
            return simple_instruction("OP_NEGATE", offset);
        case OP_PRINT:
            return simple_instruction("OP_PRINT", offset);
        case OP_JUMP:
            return jump_instruction("OP_JUMP", 1, chunk, offset);
        case OP_JUMP_FALSE:
            return jump_instruction("OP_JUMP_FALSE", 1, chunk, offset);
        case OP_RETURN:
            return simple_instruction("OP_RETURN", offset);
        default:
            printf("Unknown opcode %d", instruction);
            return offset + 1;
    }
}