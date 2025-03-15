#include "std.x"

:__entry
    mov "gcc -O2 -static orta.c -o orta" rbx
    call _system
    mov "rm build.pre.x" rbx
    call _system

