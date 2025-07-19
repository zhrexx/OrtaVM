#include "std.x"

__entry:
    mov 3 rax
    mov "./libx.so" rbx
    mov "ExampleExtern" rcx
    xcall
    print
    print "Hello"
