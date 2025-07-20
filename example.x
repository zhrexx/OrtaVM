#include "std.x"

externCallTest:
    call callExtern "./libx.so" "ExampleExtern"
    print
    ret

__entry:
    call externCallTest
    call chooseLib "./libx.so" "./libx.dll"
    print

    halt 1
