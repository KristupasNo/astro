cmake_minimum_required(VERSION 3.16)
project(tlmframe VERSION 0.1.0 LANGUAGES C)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR "Use an out-of-source build: cmake -S . -B build")
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
add_compile_options(-Wall -Wextra -Werror)

add_library(frame STATIC src/frame.c)
target_include_directories(frame PUBLIC include)

add_executable(tlmframe src/main.c)
target_link_libraries(tlmframe PRIVATE frame)
target_compile_definitions(tlmframe PRIVATE TLMFRAME_VERSION="${PROJECT_VERSION}")

enable_testing()
add_executable(test_frame tests/test_frame.c)
target_link_libraries(test_frame PRIVATE frame)
add_test(NAME frame COMMAND test_frame)
add_test(NAME cli COMMAND "${CMAKE_COMMAND}"
    "-DTLMFRAME=$<TARGET_FILE:tlmframe>"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_cli.cmake")
