#include "std.x"

:__entry
;    push 1000
;:loop
;    dup
;    push 1
;    sub
;    dup
;    push 0
;    eq
;    jmpif exit_loop
;    jmp loop
;:exit_loop
    push 65
    mov "char" rbx
    call cast
    print
    
    call cregs
