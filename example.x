#include "std.x"

__entry:
    var a
    push 10
    setvar a
    getvar a
    print

    push "Hello, World"
    print

    alloc 10
    free
