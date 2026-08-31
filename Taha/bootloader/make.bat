@echo off

set FILE=%1

if "%FILE%"=="" (
    echo Usage: make filename
    exit /b
)

nasm -f bin %FILE%.asm -l %FILE%.lst -o %FILE%.bin

if %errorlevel% neq 0 (
    exit /b
)
