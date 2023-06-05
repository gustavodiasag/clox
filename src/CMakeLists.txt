project(clox)

add_library(source
    backend/chunk.c
    backend/value.c
    backend/vm.c
    debug.c
    frontend/compiler.c
    frontend/scanner.c
    memory.c
)

target_include_directories(source
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)