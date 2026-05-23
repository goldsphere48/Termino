cmake_minimum_required(VERSION 3.20)

project(compiler
    LANGUAGES CXX
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(compiler
   src/main.cpp
   src/lexer.cpp
   src/error.cpp
   src/isa_helper.cpp
   src/parser.cpp
   src/parser_helper.cpp
   src/semantic.cpp
)

target_include_directories(compiler PRIVATE include ../shared)

target_compile_features(compiler PRIVATE cxx_std_20)

execute_process(
    COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${CMAKE_BINARY_DIR}/compile_commands.json
        ${CMAKE_SOURCE_DIR}/compile_commands.json
)
