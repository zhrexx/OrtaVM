#include "std.x"

__entry:
    togglelocalscope
    var a
    push 10
    setvar a
    getvar a
    print

    push "Hello, World"
    print

    alloc 10
    free
    togglelocalscope 
    getvar a 
    print
