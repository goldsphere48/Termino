@echo off
setlocal

set SOURCE=src/main.cpp src/lexer.cpp src/error.cpp src/isa_helper.cpp src/parser.cpp src/parser_helper.cpp
set BUILD_DIR=build
set OUT=compiler.exe

if not exist %BUILD_DIR% mkdir %BUILD_DIR%

clang %SOURCE% -o %BUILD_DIR%\%OUT% -O0 -g -std=c++20 -Iinclude
