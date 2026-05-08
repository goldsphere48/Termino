@echo off
setlocal

set SOURCE=main.cpp lexer.cpp error.cpp isa_helper.cpp parser.cpp parser_helper.cpp
set BUILD_DIR=build
set OUT=compiler.exe

if not exist %BUILD_DIR% mkdir %BUILD_DIR%

clang %SOURCE% -o %BUILD_DIR%\%OUT% -O0 -g -std=c++20
