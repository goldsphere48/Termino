@echo off
setlocal

cd /d %~dp0

set BUILD_DIR=build

if not exist %BUILD_DIR% mkdir %BUILD_DIR%

cd %BUILD_DIR%
cmake -G "Ninja" -DCMAKE_BUILD_TYPE=Debug .. && cmake --build . 
