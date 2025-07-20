#include "std.x"

externCallTest:
    call callExtern "./libx.so" "ExampleExtern"
    print
    ret

__entry:
    call externCallTest
    push "platform"
    ovm stack
    print
    halt 1