project(clox)

add_library(source
    main.c
    chunk.c
    debug.c
    memory.c
    value.c
    back-end/vm.c
)

target_include_directories(source
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)