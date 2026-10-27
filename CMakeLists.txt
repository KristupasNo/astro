cmake_minimum_required(VERSION 3.16)
project(tlmframe LANGUAGES C)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR "Use an out-of-source build: cmake -S . -B build")
endif()

find_package(Git REQUIRED)
execute_process(
    COMMAND "${GIT_EXECUTABLE}" describe --tags --always --dirty
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    RESULT_VARIABLE git_status
    OUTPUT_VARIABLE TLMFRAME_VERSION
    ERROR_VARIABLE git_error
    OUTPUT_STRIP_TRAILING_WHITESPACE)
if(NOT git_status EQUAL 0)
    message(FATAL_ERROR "Versioning requires a Git checkout with a commit: ${git_error}")
endif()

execute_process(
    COMMAND "${GIT_EXECUTABLE}" rev-parse HEAD
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    RESULT_VARIABLE git_status
    OUTPUT_VARIABLE TLMFRAME_COMMIT_HASH
    ERROR_VARIABLE git_error
    OUTPUT_STRIP_TRAILING_WHITESPACE)
if(NOT git_status EQUAL 0)
    message(FATAL_ERROR "Cannot read the Git commit: ${git_error}")
endif()

if(DEFINED ENV{SOURCE_DATE_EPOCH})
    set(SOURCE_DATE_EPOCH "$ENV{SOURCE_DATE_EPOCH}")
else()
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" log -1 --format=%ct
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        RESULT_VARIABLE git_status
        OUTPUT_VARIABLE SOURCE_DATE_EPOCH
        ERROR_VARIABLE git_error
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(NOT git_status EQUAL 0)
        message(FATAL_ERROR "Cannot read the Git commit timestamp: ${git_error}")
    endif()
endif()
if(NOT SOURCE_DATE_EPOCH MATCHES "^[0-9]+$")
    message(FATAL_ERROR "SOURCE_DATE_EPOCH must be a non-negative Unix timestamp")
endif()

# The launcher preserves the epoch when compilation happens after configuration.
set(CMAKE_C_COMPILER_LAUNCHER "${CMAKE_COMMAND}" -E env
    "SOURCE_DATE_EPOCH=${SOURCE_DATE_EPOCH}")
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
add_compile_options(-Wall -Wextra -Werror -Wdate-time
    "-ffile-prefix-map=${CMAKE_CURRENT_SOURCE_DIR}=/usr/src/tlmframe"
    "-ffile-prefix-map=${CMAKE_CURRENT_BINARY_DIR}=/usr/src/tlmframe/build")

configure_file(include/version.h.in generated/version.h @ONLY ESCAPE_QUOTES)

add_library(frame STATIC src/frame.c)
target_include_directories(frame PUBLIC include)

add_executable(tlmframe src/main.c)
target_link_libraries(tlmframe PRIVATE frame)
target_include_directories(tlmframe PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated")

enable_testing()
add_executable(test_frame tests/test_frame.c)
target_link_libraries(test_frame PRIVATE frame)
add_test(NAME frame COMMAND test_frame)
add_test(NAME cli COMMAND "${CMAKE_COMMAND}"
    "-DTLMFRAME=$<TARGET_FILE:tlmframe>"
    "-DTLMFRAME_EMULATOR=${CMAKE_CROSSCOMPILING_EMULATOR}"
    "-DTLMFRAME_VERSION=${TLMFRAME_VERSION}"
    "-DTLMFRAME_COMMIT_HASH=${TLMFRAME_COMMIT_HASH}"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_cli.cmake")
