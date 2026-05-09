@echo off
setlocal

%~dp0build/Debug/compiler.exe %~dp0examples/test.tasm
