#include "std.x"

__entry:
    push "Hello, World using 'print' instruction"
    print

    mov 2 rax
    mov "echo Hello, World using 'xcall'" rbx
    xcall

    alloc 25
    mov "Hello, World using memory" rax
    pop rbx ; memory
    mov 0 rcx
    write

    mov 25 rcx
    mov 1 rax
    printmem
