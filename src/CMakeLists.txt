project(clox)

add_library(source
    back-end/chunk.c
    back-end/garbage_collector.c
    back-end/object.c
    back-end/table.c
    back-end/value.c
    back-end/vm.c
    debug.c
    front-end/compiler.c
    front-end/scanner.c
    memory.c
)

target_include_directories(source
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)