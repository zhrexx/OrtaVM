#include "std.x"

externCallTest:
    call callExtern "./libx.so" "ExampleExtern"
    print
    ret

__entry:
    call externCallTest
    halt 1