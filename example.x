;#include "std.x"

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
    
;alloc 4
;pop rbx ; memory 
;mov 0 rcx
;mov 100 rax
;write

;mov 2 rax
;mov 100 rcx
;printmem 
    
    push "Hello, World"
    print
    ;call cregs
