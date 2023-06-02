#include "common.h"
#include "chunk.h"
#include "debug.h"

int main(int argc, const char *argv[])
{
    Chunk chunk;
    init_chunk(&chunk);
    write_chunk(&chunk, OP_RETURN);
    disassemble_chunk(&chunk, "test_chunk");
    free_chunk(&chunk);
    return 0;
}