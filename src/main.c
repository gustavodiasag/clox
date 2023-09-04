#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "back-end/chunk.h"
#include "back-end/vm.h"
#include "common.h"
#include "debug.h"

void repl()
{
    char line[1024];

    while (true) {
        printf("> ");

        if (!fgets(line, sizeof(line), stdin)) {
            printf("\n");
            break;
        }

        interpret(line);
    }
}

char* read_file(const char* path)
{
    FILE* file = fopen(path, "rb");

    if (!file) {
        fprintf(stderr, "Could not open file \"%s\".\n", path);
        exit(74);
    }

    fseek(file, 0L, SEEK_END);
    size_t file_size = ftell(file);
    rewind(file);

    char* buffer = (char*)malloc(file_size + 1);

    if (!buffer) {
        fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
        exit(74);
    }

    size_t bytes_read = fread(buffer, sizeof(char), file_size, file);

    if (bytes_read < file_size) {
        fprintf(stderr, "Could not read file \"%s\".\n", path);
        exit(74);
    }

    buffer[bytes_read] = '\0';
    fclose(file);

    return buffer;
}

void run_file(const char* path)
{
    char* source = read_file(path);
    interpret_result_t result = interpret(source);
    free(source);

    // TODO: Handle errors using the GNU Library Reference Manual conventions.
    if (result == INTERPRET_COMPILE_ERROR)
        exit(65);

    if (result == INTERPRET_RUNTIME_ERROR)
        exit(70);
}

int main(int argc, const char* argv[])
{
    init_vm();

    if (argc == 1)
        repl();
    else if (argc == 2)
        run_file(argv[1]);
    else {
        fprintf(stderr, "Usage: clox [path]\n");
        exit(64);
    }

    free_vm();
    return 0;
}