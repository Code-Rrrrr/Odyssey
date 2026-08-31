@echo off

set FILE=%1

if "%FILE%"=="" (
    echo Usage: makerun filename
    exit /b
)

nasm -f bin %FILE%.asm -l %FILE%.lst -o %FILE%.bin

if %errorlevel% neq 0 (
    exit /b
)

qemu-system-x86_64 -m 16M -drive format=raw,file=%FILE%.bin -debugcon stdio