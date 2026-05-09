cmake_minimum_required(VERSION 3.20)

project(compiler
    LANGUAGES CXX
)

add_executable(compiler
   src/main.cpp
   src/lexer.cpp
   src/error.cpp
   src/isa_helper.cpp
   src/parser.cpp
   src/parser_helper.cpp
)

target_include_directories(compiler PRIVATE include)

target_compile_features(compiler PRIVATE cxx_std_20)
