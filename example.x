#include "std.x"
#define u8 char

__entry:
    alloc u8 5 rax
    push 42
    @w rax 1 int
    @r rax 1 int
    print
    free rax
    
    add 10 10 
    print

    halt 10
