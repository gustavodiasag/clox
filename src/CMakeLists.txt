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

if(DEBUG)
    add_compile_definitions(DEBUG_PRINT_CODE DEBUG_TRACE_EXECUTION)
endif()

if(LOG_GC)
    add_compile_definitions(DEBUG_LOG_GC DEBUG_STRESS_GC)
endif()

if(OPTIMIZE)
    add_compile_definitions(NAN_BOXING)
endif()

target_include_directories(source
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)