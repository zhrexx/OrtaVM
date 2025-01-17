@echo off
setlocal

set /p COMPILER="Enter the compiler (cl for MSVC, gcc for MinGW): "

set OUTPUT_DIR=build
set STACK_SIZE=8388608
set OPTIMIZATION=/O2
set STATIC_LINKING=/MT

if not exist "%OUTPUT_DIR%" mkdir "%OUTPUT_DIR%"

if /I "%COMPILER%"=="cl" (
    cl /Fe:%OUTPUT_DIR%\orta.exe orta.c /link /STACK:%STACK_SIZE% %OPTIMIZATION% %STATIC_LINKING%
    cl /Fe:%OUTPUT_DIR%\deovm.exe deovm.c /link /STACK:%STACK_SIZE% %OPTIMIZATION% %STATIC_LINKING%
) else if /I "%COMPILER%"=="gcc" (
    set FLAGS=-Wl,--stack,%STACK_SIZE% -static-libgcc -static-libstdc++ --static -O3
    x86_64-w64-mingw32-gcc -o %OUTPUT_DIR%\orta.exe orta.c %FLAGS%
    x86_64-w64-mingw32-gcc -o %OUTPUT_DIR%\deovm.exe deovm.c %FLAGS%
) else (
    echo Unknown compiler. Please use 'cl' for MSVC or 'gcc' for MinGW.
)

echo Compilation complete.
endlocal
