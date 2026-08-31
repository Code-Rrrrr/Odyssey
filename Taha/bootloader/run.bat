@echo off

set FILE=%1

if "%FILE%"=="" (
    echo Usage: run filename
    exit /b
)

qemu-system-x86_64 -m 16M -drive format=raw,file=%FILE%.bin -debugcon stdio