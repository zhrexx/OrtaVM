#include "std.x"

externCallTest:
    call callExtern "./libx.so" "ExampleExtern"
    print
    ret

.abc:
    print "Hello, World"
    ret

__entry:
    call externCallTest
    call chooseLib "./libx.so" "./libx.dll"
    print

    halt 1
