cmake_minimum_required(VERSION 3.16.3)

project(clox)

add_subdirectory(src)
add_subdirectory(include)

add_executable(${PROJECT_NAME} src/main.c)

target_link_libraries(${PROJECT_NAME} PRIVATE source)