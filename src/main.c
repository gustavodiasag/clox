#include "common.h"
#include "chunk.h"
#include "debug.h"
#include "back-end/vm.h"

int main(int argc, const char *argv[])
{
    init_vm();

    Chunk chunk;
    init_chunk(&chunk);

    int constant = add_constant(&chunk, 1.2);
    write_chunk(&chunk, OP_CONSTANT, 123);
    write_chunk(&chunk, constant, 123);

    write_chunk(&chunk, OP_RETURN, 123);
    disassemble_chunk(&chunk, "test_chunk");

    interpret(&chunk);
    
    free_vm();
    free_chunk(&chunk);
    return 0;
}