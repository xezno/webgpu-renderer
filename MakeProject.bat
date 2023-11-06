@echo off
IF EXIST build(rmdir /s /q build)
IF NOT EXIST build (mkdir build)
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
