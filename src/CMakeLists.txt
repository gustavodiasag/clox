project(clox)

add_library(source
    backend/chunk.c
    backend/vm.c
    debug.c
    main.c
    memory.c
    value.c
)

target_include_directories(source
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)