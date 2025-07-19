#include "std.x"

externCallTest:
    mov 3 rax
    mov "./libx.so" rbx
    mov "ExampleExtern" rcx
    xcall
    print
    ret

test:
    print
    print
    ret

__entry:
    call test "Hello, World" 644
    ;call externCallTest
